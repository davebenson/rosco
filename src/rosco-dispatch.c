/* NOTE: this may not work very well on windows, where i'm
   not sure that "SOCKETs" are allocated nicely like
   file-descriptors are */
/* TODO:
 *  * epoll() implementation
 *  * kqueue() implementation
 *  * windows port (yeah, right, volunteers are DEFINITELY needed for this one...)
 */
/* TODO:
 *  use RoscoMemPoolFixed instead of recycling lists
 */
#include <assert.h>
#include <alloca.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/poll.h>
#define USE_POLL              1

/* windows annoyances:  use select, use a full-fledges map for fds */
#ifdef WIN32
# include <winsock.h>
# define USE_POLL              0
# define HAVE_SMALL_FDS            0
#endif
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>             /* for pipe(), needed for signal handling */
#include "rosco-common.h"
#include "rosco-fd.h"
#include "rosco-dispatch.h"
#include "rosco-rbtree-macros.h"
#include "rosco-list-macros.h"

#define DEBUG_DISPATCH_INTERNALS  0
#define DEBUG_DISPATCH            0

#ifndef HAVE_SMALL_FDS
# define HAVE_SMALL_FDS           1
#endif

typedef struct _Callback Callback;
struct _Callback
{
  RoscoFDFunc func;
  void *data;
};

typedef struct _FDMap FDMap;
struct _FDMap
{
  int notify_desired_index;     /* -1 if not an known fd */
  int change_index;             /* -1 if no prior change */
  int closed_since_notify_started;
};

#if !HAVE_SMALL_FDS
typedef struct _FDMapNode FDMapNode;
struct _FDMapNode
{
  RoscoFileDescriptor fd;
  FDMapNode *left, *right, *parent;
  rosco_boolean is_red;
  FDMap map;
};
#endif


typedef struct _RealDispatch RealDispatch;
struct _RealDispatch
{
  RoscoDispatch base;
  Callback *callbacks;          /* parallels notifies_desired */
  size_t notifies_desired_alloced;
  size_t changes_alloced;
#if HAVE_SMALL_FDS
  FDMap *fd_map;                /* map indexed by fd */
  size_t fd_map_size;           /* number of elements of fd_map */
#else
  FDMapNode *fd_map_tree;       /* map indexed by fd */
#endif
  rosco_boolean dispatching;

  RoscoDispatchTimer *timer_tree;
  RoscoDispatchTimer *recycled_timeouts;

  RoscoDispatchSignal *signal_tree;
  int signal_pipe_fds[2];

#if ROSCO_HAS_EPOLL
  int epoll_fd;
  unsigned max_epoll_events;
  struct epoll_event *epoll_events;
  RoscoFileDescriptorNotify *epoll_rosco_events;
#endif

  RoscoDispatchChild *child_tree;
  RoscoDispatchSignal *child_sig_handler;
  RoscoDispatchIdle *idle_child_handler;

  RoscoDispatchIdle *first_idle, *last_idle;
  RoscoDispatchIdle *recycled_idles;
};

struct _RoscoDispatchTimer
{
  RealDispatch *dispatch;

  /* the actual timeout time */
  unsigned long timeout_secs;
  unsigned timeout_usecs;

  /* red-black tree stuff */
  RoscoDispatchTimer *left, *right, *parent;
  unsigned is_red : 1;
  unsigned expired : 1;

  /* user callback */
  RoscoTimerFunc func;
  void *func_data;
};

struct _RoscoDispatchIdle
{
  RealDispatch *dispatch;

  RoscoDispatchIdle *prev, *next;

  /* user callback */
  RoscoIdleFunc func;
  void *func_data;
};

struct _RoscoDispatchSignal
{
  RealDispatch *dispatch;
  int signal;
  unsigned is_notifying : 1;
  unsigned destroyed_while_notifying : 1;
  unsigned is_red : 1;
  RoscoSignalHandler func;
  void *func_data;

  RoscoDispatchSignal *left, *right, *parent;
};

struct _RoscoDispatchChild
{
  RealDispatch *dispatch;
  int process_id;
  unsigned is_notifying : 1;
  unsigned is_red : 1;
  RoscoChildHandler func;
  void *func_data;

  RoscoDispatchChild *left, *right, *parent;
};

/* only one dispatch may assume responsibility for signal-handling */
static RoscoDispatch *global_signal_dispatch;
static int          global_signal_dispatch_fd;

/* Define the tree of timers, as per rosco-rbtree-macros.h */
#define TIMERS_COMPARE(a,b, rv) \
  if (a->timeout_secs < b->timeout_secs) rv = -1; \
  else if (a->timeout_secs > b->timeout_secs) rv = 1; \
  else if (a->timeout_usecs < b->timeout_usecs) rv = -1; \
  else if (a->timeout_usecs > b->timeout_usecs) rv = 1; \
  else if (a < b) rv = -1; \
  else if (a > b) rv = 1; \
  else rv = 0;
#define GET_TIMER_TREE(d) \
  (d)->timer_tree, RoscoDispatchTimer *, \
  ROSCO_STD_GET_IS_RED, ROSCO_STD_SET_IS_RED, \
  parent, left, right, \
  TIMERS_COMPARE

#if !HAVE_SMALL_FDS
#define FD_MAP_NODES_COMPARE(a,b, rv) \
  if (a->fd < b->fd) rv = -1; \
  else if (a->fd > b->fd) rv = 1; \
  else rv = 0;
#define GET_FD_MAP_TREE(d) \
  (d)->fd_map_tree, FDMapNode *, \
  TIMER_GET_IS_RED, TIMER_SET_IS_RED, \
  parent, left, right, \
  FD_MAP_NODES_COMPARE
#define COMPARE_FD_TO_FD_MAP_NODE(a,b, rv) \
  if (a < b->fd) rv = -1; \
  else if (a > b->fd) rv = 1; \
  else rv = 0;
#endif

/* declare the idle-handler list */
#define GET_IDLE_LIST(d) \
  RoscoDispatchIdle *, d->first_idle, d->last_idle, prev, next

/* signals */
#define COMPARE_SIGNO_TO_SIGNAL(a,b, rv) \
  rv = a < b->signal ? -1 : a > b->signal ? 1 : 0;
#define COMPARE_SIGNALS(a,b, rv) \
  COMPARE_SIGNO_TO_SIGNAL(a->signal, b, rv)
#define GET_SIGNAL_TREE(d) \
  (d)->signal_tree, RoscoDispatchSignal *, \
  ROSCO_STD_GET_IS_RED, ROSCO_STD_SET_IS_RED, \
  parent, left, right, \
  COMPARE_SIGNALS

/* children */
#define COMPARE_PID_TO_CHILD(a,b, rv) \
  rv = a < b->process_id ? -1 : a > b->process_id ? 1 : 0;
#define COMPARE_PROCESSES(a,b, rv) \
  COMPARE_PID_TO_CHILD(a->process_id, b, rv)
#define GET_CHILD_TREE(d) \
  (d)->child_tree, RoscoDispatchChild *, \
  ROSCO_STD_GET_IS_RED, ROSCO_STD_SET_IS_RED, \
  parent, left, right, \
  COMPARE_PROCESSES

/* Create or destroy a Dispatch */
RoscoDispatch *rosco_dispatch_new (void)
{
  RealDispatch *rv = ROSCO_NEW (RealDispatch);
  struct timeval tv;
  rv->base.n_changes = 0;
  rv->notifies_desired_alloced = 8;
  rv->base.notifies_desired = ROSCO_NEW_ARRAY (RoscoFileDescriptorNotify, rv->notifies_desired_alloced);
  rv->base.n_notifies_desired = 0;
  rv->callbacks = ROSCO_NEW_ARRAY (Callback, rv->notifies_desired_alloced);
  rv->changes_alloced = 8;
  rv->base.changes = ROSCO_NEW_ARRAY (RoscoFileDescriptorNotifyChange, rv->changes_alloced);
#if HAVE_SMALL_FDS
  rv->fd_map_size = 16;
  rv->fd_map = ROSCO_NEW_ARRAY (FDMap, rv->fd_map_size);
  memset (rv->fd_map, 255, sizeof (FDMap) * rv->fd_map_size);
#else
  rv->fd_map_tree = NULL;
#endif
  rv->timer_tree = NULL;
  rv->first_idle = rv->last_idle = NULL;
  rv->base.has_idle = ROSCO_FALSE;
  rv->base.has_timeout = ROSCO_FALSE;
  rv->recycled_idles = NULL;
  rv->recycled_timeouts = NULL;
  rv->dispatching = ROSCO_FALSE;

  /* need to handle SIGPIPE more gracefully than default */
  signal (SIGPIPE, SIG_IGN);

  rv->signal_tree = NULL;
  rv->signal_pipe_fds[0] = rv->signal_pipe_fds[1] = -1;

  rv->child_tree = NULL;
  rv->child_sig_handler = NULL;
  rv->idle_child_handler = NULL;

  gettimeofday (&tv, NULL);
  rv->base.last_dispatch_secs = tv.tv_sec;
  rv->base.last_dispatch_usecs = tv.tv_usec;

#if ROSCO_HAS_EPOLL
  /* maybe this should default to the NRFILE or something. */
  rv->epoll_fd = epoll_create (1024);
  rv->max_epoll_events = 64;
  rv->epoll_events = ROSCO_NEW (struct epoll_event, d->max_epoll_events);
  rv->epoll_rosco_events = ROSCO_NEW (RoscoFileDescriptorNotify, d->max_epoll_events);
#endif

  return &rv->base;
}

#if !HAVE_SMALL_FDS
static void
free_fd_tree_recursive (FDMapNode          *node)
{
  if (node)
    {
      free_fd_tree_recursive (allocator, node->left);
      free_fd_tree_recursive (allocator, node->right);
      rosco_free (node);
    }
}
#endif
void free_timer_tree_recursive (RoscoDispatchTimer *node)
{
  if (node)
    {
      free_timer_tree_recursive (node->left);
      free_timer_tree_recursive (node->right);
      rosco_free (node);
    }
}

/* XXX: leaking timer_tree seemingly? */
void
rosco_dispatch_free(RoscoDispatch *dispatch)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  while (d->recycled_timeouts != NULL)
    {
      RoscoDispatchTimer *t = d->recycled_timeouts;
      d->recycled_timeouts = t->right;
      rosco_free (t);
    }
  while (d->recycled_idles != NULL)
    {
      RoscoDispatchIdle *i = d->recycled_idles;
      d->recycled_idles = i->next;
      rosco_free (i);
    }
  while (d->first_idle)
    {
      RoscoDispatchIdle *i = d->first_idle;
      d->first_idle = i->next;
      rosco_free (i);
    }
  d->last_idle = NULL;

  rosco_free (d->base.notifies_desired);
  rosco_free (d->base.changes);
  rosco_free (d->callbacks);

#if HAVE_SMALL_FDS
  rosco_free (d->fd_map);
#else
  free_fd_tree_recursive (d->fd_map_tree);
#endif
  free_timer_tree_recursive (d->timer_tree);
  rosco_free (d);
}

/* TODO: perhaps thread-private dispatches make more sense? */
static RoscoDispatch *def = NULL;
RoscoDispatch  *rosco_dispatch_default (void)
{
  if (def == NULL)
    def = rosco_dispatch_new ();
  return def;
}

#if HAVE_SMALL_FDS
static void
enlarge_fd_map (RealDispatch *d,
                unsigned      fd)
{
  size_t new_size = d->fd_map_size * 2;
  FDMap *new_map;
  while (fd >= new_size)
    new_size *= 2;
  new_map = ROSCO_NEW_ARRAY (FDMap, new_size);
  memcpy (new_map, d->fd_map, d->fd_map_size * sizeof (FDMap));
  memset (new_map + d->fd_map_size,
          255,
          sizeof (FDMap) * (new_size - d->fd_map_size));
  rosco_free (d->fd_map);
  d->fd_map = new_map;
  d->fd_map_size = new_size;
}

static inline void
ensure_fd_map_big_enough (RealDispatch *d,
                          unsigned      fd)
{
  if (fd >= d->fd_map_size)
    enlarge_fd_map (d, fd);
}
#endif

static unsigned
allocate_notifies_desired_index (RealDispatch *d)
{
  unsigned rv = d->base.n_notifies_desired++;
  if (rv == d->notifies_desired_alloced)
    {
      unsigned new_size = d->notifies_desired_alloced * 2;
      RoscoFileDescriptorNotify *n = ROSCO_NEW_ARRAY (RoscoFileDescriptorNotify, new_size);
      Callback *c = ROSCO_NEW_ARRAY (Callback, new_size);
      memcpy (n, d->base.notifies_desired, d->notifies_desired_alloced * sizeof (RoscoFileDescriptorNotify));
      rosco_free (d->base.notifies_desired);
      memcpy (c, d->callbacks, d->notifies_desired_alloced * sizeof (Callback));
      rosco_free (d->callbacks);
      d->base.notifies_desired = n;
      d->callbacks = c;
      d->notifies_desired_alloced = new_size;
    }
#if DEBUG_DISPATCH_INTERNALS
  fprintf (stderr, "allocate_notifies_desired_index: returning %u\n", rv);
#endif
  return rv;
}
static unsigned
allocate_change_index (RealDispatch *d)
{
  unsigned rv = d->base.n_changes++;
  if (rv == d->changes_alloced)
    {
      unsigned new_size = d->changes_alloced * 2;
      RoscoFileDescriptorNotifyChange *n = ROSCO_NEW_ARRAY (RoscoFileDescriptorNotifyChange, new_size);
      memcpy (n, d->base.changes, d->changes_alloced * sizeof (RoscoFileDescriptorNotifyChange));
      rosco_free (d->base.changes);
      d->base.changes = n;
      d->changes_alloced = new_size;
    }
  return rv;
}

static inline FDMap *
get_fd_map (RealDispatch *d, RoscoFileDescriptor fd)
{
#if HAVE_SMALL_FDS
  if ((unsigned)fd >= d->fd_map_size)
    return NULL;
  else
    return d->fd_map + fd;
#else
  FDMapNode *node;
  ROSCO_RBTREE_LOOKUP_COMPARATOR (GET_FD_MAP_TREE (d), fd, COMPARE_FD_TO_FD_MAP_NODE, node);
  return node ? &node->map : NULL;
#endif
}
static inline FDMap *
force_fd_map (RealDispatch *d, RoscoFileDescriptor fd)
{
#if HAVE_SMALL_FDS
  ensure_fd_map_big_enough (d, fd);
  return d->fd_map + fd;
#else
  {
    FDMap *fm = get_fd_map (d, fd);
    RoscoAllocator *allocator = d->allocator;
    if (fm == NULL)
      {
        FDMapNode *node = ROSCO_NEW (FDMapNode);
        FDMapNode *conflict;
        node->fd = fd;
        memset (&node->map, 255, sizeof (FDMap));
        ROSCO_RBTREE_INSERT (GET_FD_MAP_TREE (d), node, conflict);
        assert (conflict == NULL);
        fm = &node->map;
      }
    return fm;
  }
#endif
}

static void
deallocate_change_index (RealDispatch *d,
                         FDMap        *fm)
{
  unsigned ch_ind = fm->change_index;
  unsigned from = d->base.n_changes - 1;
  RoscoFileDescriptor from_fd;
  fm->change_index = -1;
  rosco_assert (ch_ind < d->base.n_changes);
  if (ch_ind == from)
    {
      d->base.n_changes--;
      return;
    }
  from_fd = d->base.changes[ch_ind].fd;
  get_fd_map (d, from_fd)->change_index = ch_ind;
  d->base.changes[ch_ind] = d->base.changes[from];
  d->base.n_changes--;
}

static void
deallocate_notify_desired_index (RealDispatch *d,
                                 RoscoFileDescriptor  fd,
                                 FDMap        *fm)
{
  unsigned nd_ind = fm->notify_desired_index;
  unsigned from = d->base.n_notifies_desired - 1;
  RoscoFileDescriptor from_fd;
  (void) fd;
#if DEBUG_DISPATCH_INTERNALS
  fprintf (stderr, "deallocate_notify_desired_index: fd=%d, nd_ind=%u\n",fd,nd_ind);
#endif
  fm->notify_desired_index = -1;
  if (nd_ind == from)
    {
      d->base.n_notifies_desired--;
      return;
    }
  from_fd = d->base.notifies_desired[from].fd;
  get_fd_map (d, from_fd)->notify_desired_index = nd_ind;
  d->base.notifies_desired[nd_ind] = d->base.notifies_desired[from];
  d->callbacks[nd_ind] = d->callbacks[from];
  d->base.n_notifies_desired--;
}

/* Registering file-descriptors to watch. */
void
rosco_dispatch_watch_fd (RoscoDispatch *dispatch,
                       int          fd,
                       unsigned     events,
                       RoscoFDFunc    callback,
                       void        *callback_data)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  unsigned f = fd;              /* avoid tiring compiler warnings: "comparison of signed versus unsigned" */
  unsigned nd_ind, change_ind;
  unsigned old_events;
  FDMap *fm;
#if DEBUG_DISPATCH
  fprintf (stderr, "dispatch: watch_fd: %d, %s%s\n",
           fd,
           (events&ROSCO_EVENT_READABLE)?"r":"",
           (events&ROSCO_EVENT_WRITABLE)?"w":"");
#endif
  if (events == 0)
    {
      callback = NULL;
      callback_data = NULL;
    }
  else
    rosco_assert (callback != NULL);
  fm = force_fd_map (d, f);

  /* XXX: should we set fm->map.closed_since_notify_started=0 ??? */

  if (fm->notify_desired_index == -1)
    {
      if (callback != NULL)
        nd_ind = fm->notify_desired_index = allocate_notifies_desired_index (d);
      old_events = 0;
    }
  else
    {
      old_events = dispatch->notifies_desired[fm->notify_desired_index].events;
      if (callback == NULL)
        deallocate_notify_desired_index (d, fd, fm);
      else
        nd_ind = fm->notify_desired_index;
    }
  if (callback == NULL)
    {
      if (fm->change_index == -1)
        {
          change_ind = fm->change_index = allocate_change_index (d);
          dispatch->changes[change_ind].old_events = old_events;
        }
      else
        change_ind = fm->change_index;
      d->base.changes[change_ind].fd = f;
      d->base.changes[change_ind].events = 0;
      return;
    }
  assert (callback != NULL && events != 0);
  if (fm->change_index == -1)
    {
      change_ind = fm->change_index = allocate_change_index (d);
      dispatch->changes[change_ind].old_events = old_events;
    }
  else
    change_ind = fm->change_index;

  d->base.changes[change_ind].fd = fd;
  d->base.changes[change_ind].events = events;
  d->base.notifies_desired[nd_ind].fd = fd;
  d->base.notifies_desired[nd_ind].events = events;
  d->callbacks[nd_ind].func = callback;
  d->callbacks[nd_ind].data = callback_data;
}

void
rosco_dispatch_close_fd (RoscoDispatch *dispatch,
                       int          fd)
{
  rosco_dispatch_fd_closed (dispatch, fd);
  close (fd);
}

void
rosco_dispatch_fd_closed(RoscoDispatch             *dispatch,
                       RoscoFileDescriptor        fd)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  FDMap *fm;
#if DEBUG_DISPATCH
  fprintf (stderr, "dispatch: fd %d closed\n", fd);
#endif
  fm = force_fd_map (d, fd);
  fm->closed_since_notify_started = 1;
  if (fm->change_index != -1)
    deallocate_change_index (d, fm);
  if (fm->notify_desired_index != -1)
    deallocate_notify_desired_index (d, fd, fm);
}

static void
free_timer (RoscoDispatchTimer *timer)
{
  RealDispatch *d = timer->dispatch;
  timer->right = d->recycled_timeouts;
  d->recycled_timeouts = timer;
}

void
rosco_dispatch_dispatch (RoscoDispatch             *dispatch,
                       size_t                   n_notifies,
                       RoscoFileDescriptorNotify *notifies)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  unsigned fd_max;
  unsigned i;
  struct timeval tv;

  /* Update the cached time-of-day. (this must be done before any
   * callbacks are run.) */
  gettimeofday (&tv, NULL);
  dispatch->last_dispatch_secs = tv.tv_sec;
  dispatch->last_dispatch_usecs = tv.tv_usec;

  /* If a notify comes in with a new file-descriptor (?? can it happen),
     that's bigger than our allocation size,
     we need to enlarge the fd_map. */
  fd_max = 0;
  for (i = 0; i < n_notifies; i++)
    if (fd_max < (unsigned) notifies[i].fd)
      fd_max = notifies[i].fd;
  rosco_assert (!d->dispatching);
  d->dispatching = ROSCO_TRUE;
  ensure_fd_map_big_enough (d, fd_max);
  for (i = 0; i < n_notifies; i++)
    d->fd_map[notifies[i].fd].closed_since_notify_started = 0;
  for (i = 0; i < n_notifies; i++)
    {
      unsigned fd = notifies[i].fd;
      if (!d->fd_map[fd].closed_since_notify_started
       && d->fd_map[fd].notify_desired_index != -1)
        {
          unsigned nd_ind = d->fd_map[fd].notify_desired_index;
          unsigned events = d->base.notifies_desired[nd_ind].events & notifies[i].events;
          if (events != 0)
            d->callbacks[nd_ind].func (fd, events, d->callbacks[nd_ind].data);
        }
    }

  /* clear changes */
  for (i = 0; i < dispatch->n_changes; i++)
    d->fd_map[dispatch->changes[i].fd].change_index = -1;
  dispatch->n_changes = 0;

  /* handle idle functions */
  while (d->first_idle != NULL)
    {
      RoscoDispatchIdle *idle = d->first_idle;
      RoscoIdleFunc func = idle->func;
      void *data = idle->func_data;
      ROSCO_LIST_REMOVE_FIRST (GET_IDLE_LIST (d));

      idle->func = NULL;                /* set to NULL to render remove_idle a no-op */
      func (data);

      idle->next = d->recycled_idles;
      d->recycled_idles = idle;
    }
  d->base.has_idle = ROSCO_FALSE;

  /* handle timers */
  RoscoDispatchTimer *expired = NULL;
  while (d->timer_tree != NULL)
    {
      RoscoDispatchTimer *min_timer;
      ROSCO_RBTREE_FIRST (GET_TIMER_TREE (d), min_timer);
      if (min_timer->timeout_secs < (unsigned long) tv.tv_sec
       || (min_timer->timeout_secs == (unsigned long) tv.tv_sec
        && min_timer->timeout_usecs <= (unsigned) tv.tv_usec))
        {
          ROSCO_RBTREE_REMOVE (GET_TIMER_TREE (d), min_timer);
          min_timer->left = expired;
          min_timer->expired = ROSCO_TRUE;
          expired = min_timer;
        }
      else
        {
          d->base.has_timeout = 1;
          d->base.timeout_secs = min_timer->timeout_secs;
          d->base.timeout_usecs = min_timer->timeout_usecs;
          break;
        }
    }
  while (expired != NULL)
    {
      RoscoDispatchTimer *timer = expired;
      expired = timer->left;
      if (timer->func != NULL)
        timer->func (timer->func_data);
      free_timer (timer);
    }
  if (d->timer_tree == NULL)
    d->base.has_timeout = 0;
  d->dispatching = ROSCO_FALSE;
}

void
rosco_dispatch_clear_changes (RoscoDispatch *dispatch)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  unsigned i;
  for (i = 0; i < dispatch->n_changes; i++)
    {
      FDMap *fm = get_fd_map (d, dispatch->changes[i].fd);
      assert (fm->change_index == (int) i);
      fm->change_index = -1;
    }
  dispatch->n_changes = 0;
}

#if ROSCO_HAS_EPOLL
static inline unsigned
events_to_epoll_events (unsigned ev)
{
  return ((ev & ROSCO_EVENT_READABLE) ? (EPOLLIN|EPOLLHUP) : 0)
       | ((ev & ROSCO_EVENT_WRITABLE) ? (EPOLLOUT) : 0);
}
static inline unsigned
epoll_events_to_events (unsigned ev)
{
  return ((ev & (EPOLLIN|EPOLLHUP)) ? ROSCO_EVENT_READABLE : 0)
       | ((ev & EPOLLOUT) ? ROSCO_EVENT_WRITABLE : 0);
}
#else
static inline unsigned
events_to_pollfd_events (unsigned ev)
{
  return  ((ev & ROSCO_EVENT_READABLE) ? POLLIN : 0)
       |  ((ev & ROSCO_EVENT_WRITABLE) ? POLLOUT : 0)
       ;
}
static inline unsigned
pollfd_events_to_events (unsigned ev)
{
  return  ((ev & (POLLIN|POLLHUP)) ? ROSCO_EVENT_READABLE : 0)
       |  ((ev & POLLOUT) ? ROSCO_EVENT_WRITABLE : 0)
       ;
}
#endif

void
rosco_dispatch_run (RoscoDispatch *dispatch)
{
  struct pollfd *fds;
  void *to_free = NULL, *to_free2 = NULL;
  size_t n_events;
  unsigned i;
  int timeout;
  RoscoFileDescriptorNotify *events;
  RealDispatch *d = (RealDispatch *) dispatch;

  /* compute timeout */
  if (dispatch->has_idle)
    timeout = 0;
  else if (!dispatch->has_timeout)
    timeout = -1;
  else
    {
      struct timeval tv;
      gettimeofday (&tv, NULL);
      if (dispatch->timeout_secs < (unsigned long) tv.tv_sec
       || (dispatch->timeout_secs == (unsigned long) tv.tv_sec
        && dispatch->timeout_usecs <= (unsigned) tv.tv_usec))
        timeout = 0;
      else
        {
          int du = dispatch->timeout_usecs - tv.tv_usec;
          int ds = dispatch->timeout_secs - tv.tv_sec;
          if (du < 0)
            {
              du += 1000000;
              ds -= 1;
            }
          if (ds > INT_MAX / 1000)
            timeout = INT_MAX / 1000 * 1000;
          else
            /* Round up, so that we ensure that something can run
               if they just wait the full duration */
            timeout = ds * 1000 + (du + 999) / 1000;
        }
    }

#if ROSCO_HAS_EPOLL
  int epoll_fd = d->epoll_fd;
  for (i = 0; i < dispatch->n_changes; i++)
    {
      RoscoFileDescriptorNotifyChange *c = &dispatch->changes[i];
      int op = c->old_events == 0 ? EPOLL_CTL_ADD
             : c->events == 0 ? EPOLL_CTL_DEL
             : EPOLL_CTL_MOD;
      struct epoll_event e;
      e.events = events_to_epoll_events (c->events);
      e.data.fd = c->fd;
      if (epoll_ctl (epoll_fd, op, c->fd, &e) < 0)
        {
          rosco_warning ("epoll_ctl %d op=0x%x events=0x%x failed: %s", c->fd, op, e.events, strerror (errno));
        }
    }
  
  int epoll_n_events = epoll_wait (epoll_fd, d->epoll_events, d->max_epoll_events, timeout);
  if (epoll_n_events < 0)
    {
      rosco_warning ("epoll_wait: %s", strerror (errno));
      epoll_n_events = 0;
    }
  else
    {
      if (epoll_n_events == d->max_epoll_events)
        {
          d->max_epoll_events *= 2;
          d->epoll_events = ROSCO_RENEW (struct epoll_event, d->epoll_events, d->max_epoll_events);
          d->epoll_rosco_events = ROSCO_RENEW (RoscoFileDescriptorNotify, d->epoll_events, d->max_epoll_events);
        }
    }
  n_events = epoll_n_events;
  events = d->epoll_rosco_events;
  for (i = 0; i < n_events; i++)
    {
      events[i].fd = d->epoll_events[i].data.fd;
      events[i].events = epoll_events_to_events (d->epoll_events[i].events);
    }
#else  /* use poll(2) */
  if (dispatch->n_notifies_desired < 128)
    fds = alloca (sizeof (struct pollfd) * dispatch->n_notifies_desired);
  else
    to_free = fds = ROSCO_NEW_ARRAY (struct pollfd, dispatch->n_notifies_desired);
  for (i = 0; i < dispatch->n_notifies_desired; i++)
    {
      fds[i].fd = dispatch->notifies_desired[i].fd;
      fds[i].events = events_to_pollfd_events (dispatch->notifies_desired[i].events);
      fds[i].revents = 0;
    }
  if (poll (fds, dispatch->n_notifies_desired, timeout) < 0)
    {
      if (errno == EINTR)
        return;   /* probably a signal interrupted the poll-- let the user have control */

      /* i don't really know what would plausibly cause this */
      fprintf (stderr, "error polling: %s\n", strerror (errno));
      return;
    }
  n_events = 0;
  for (i = 0; i < dispatch->n_notifies_desired; i++)
    if (fds[i].revents)
      n_events++;
  if (n_events < 128)
    events = alloca (sizeof (RoscoFileDescriptorNotify) * n_events);
  else
    to_free2 = events = ROSCO_NEW_ARRAY (RoscoFileDescriptorNotify, n_events);
  n_events = 0;
  for (i = 0; i < dispatch->n_notifies_desired; i++)
    if (fds[i].revents)
      {
        events[n_events].fd = fds[i].fd;
        events[n_events].events = pollfd_events_to_events (fds[i].revents);

        /* note that we may actually wind up with fewer events
           now that we actually call pollfd_events_to_events() */
        if (events[n_events].events != 0)
          n_events++;
      }
#endif
  rosco_dispatch_clear_changes (dispatch);
  rosco_dispatch_dispatch (dispatch, n_events, events);
  if (to_free)
    rosco_free (to_free);
  if (to_free2)
    rosco_free (to_free2);
}

RoscoDispatchTimer *
rosco_dispatch_add_timer(RoscoDispatch *dispatch,
                              unsigned            timeout_secs,
                              unsigned            timeout_usecs,
                              RoscoTimerFunc func,
                              void               *func_data)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  RoscoDispatchTimer *rv;
  RoscoDispatchTimer *at;
  RoscoDispatchTimer *conflict;
  rosco_assert (func != NULL);
  if (d->recycled_timeouts != NULL)
    {
      rv = d->recycled_timeouts;
      d->recycled_timeouts = rv->right;
    }
  else
    {
      rv = ROSCO_NEW (RoscoDispatchTimer);
    }
  rv->timeout_secs = timeout_secs;
  rv->timeout_usecs = timeout_usecs;
  rv->func = func;
  rv->func_data = func_data;
  rv->dispatch = d;
  rv->expired = ROSCO_FALSE;
  ROSCO_RBTREE_INSERT (GET_TIMER_TREE (d), rv, conflict);
  (void) conflict;              /* suppress warnings */
  
  /* is this the first element in the tree */
  for (at = rv; at != NULL; at = at->parent)
    if (at->parent && at->parent->right == at)
      break;
  if (at == NULL)               /* yes, so set the public members */
    {
      dispatch->has_timeout = 1;
      dispatch->timeout_secs = rv->timeout_secs;
      dispatch->timeout_usecs = rv->timeout_usecs;
    }
  return rv;
}

RoscoDispatchTimer *
rosco_dispatch_add_timer_millis (RoscoDispatch         *dispatch,
                               uint64_t             millis,
                               RoscoTimerFunc func,
                               void                *func_data)
{
  unsigned tsec = dispatch->last_dispatch_secs;
  unsigned tusec = dispatch->last_dispatch_usecs;
  tusec += 1000 * (millis % 1000);
  tsec += millis / 1000;
  if (tusec >= 1000*1000)
    {
      tusec -= 1000*1000;
      tsec += 1;
    }
  return rosco_dispatch_add_timer (dispatch, tsec, tusec, func, func_data);
}
void  rosco_dispatch_adjust_timer    (RoscoDispatchTimer *timer,
                                    unsigned           timeout_secs,
                                    unsigned           timeout_usecs)
{
  RoscoDispatchTimer *conflict;
  RealDispatch *d = timer->dispatch;
  rosco_boolean was_first = ROSCO_FALSE;
  if (d->first_idle == NULL)
    {
      RoscoDispatchTimer *first;
      ROSCO_RBTREE_FIRST (GET_TIMER_TREE (d), first);
      if (first == timer)
        was_first = ROSCO_TRUE;
    }
  rosco_assert (timer->func != NULL);
  ROSCO_RBTREE_REMOVE (GET_TIMER_TREE (d), timer);
  timer->timeout_secs = timeout_secs;
  timer->timeout_usecs = timeout_usecs;
  ROSCO_RBTREE_INSERT (GET_TIMER_TREE (d), timer, conflict);
  rosco_assert (conflict == NULL);

  if (d->first_idle == NULL)
    {
      RoscoDispatchTimer *first;
      ROSCO_RBTREE_FIRST (GET_TIMER_TREE (d), first);
      if (was_first || first == timer)
        {
          d->base.timeout_secs = first->timeout_secs;
          d->base.timeout_usecs = first->timeout_usecs;
        }
    }
}

void  rosco_dispatch_adjust_timer_millis (RoscoDispatchTimer *timer,
                                        uint64_t          milliseconds)
{
  unsigned tsec = timer->dispatch->base.last_dispatch_secs;
  unsigned tusec = timer->dispatch->base.last_dispatch_usecs;
  tusec += milliseconds % 1000 * 1000;
  tsec += milliseconds / 1000;
  if (tusec >= 1000*1000)
    {
      tusec -= 1000*1000;
      tsec += 1;
    }
  rosco_dispatch_adjust_timer (timer, tsec, tusec);
}

void  rosco_dispatch_remove_timer (RoscoDispatchTimer *timer)
{
  rosco_boolean may_be_first;
  RealDispatch *d = timer->dispatch;

  if (timer->expired)
    {
      timer->func = NULL;
      return;
    }

  may_be_first = d->base.timeout_usecs == timer->timeout_usecs
              && d->base.timeout_secs == timer->timeout_secs;

  ROSCO_RBTREE_REMOVE (GET_TIMER_TREE (d), timer);

  if (may_be_first)
    {
      if (d->timer_tree == NULL)
        d->base.has_timeout = 0;
      else
        {
          RoscoDispatchTimer *min;
          ROSCO_RBTREE_FIRST (GET_TIMER_TREE (d), min);
          d->base.timeout_secs = min->timeout_secs;
          d->base.timeout_usecs = min->timeout_usecs;
        }
    }
  free_timer (timer);
}
RoscoDispatchIdle *
rosco_dispatch_add_idle (RoscoDispatch        *dispatch,
                       RoscoIdleFunc func,
                       void               *func_data)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  RoscoDispatchIdle *rv;
  if (d->recycled_idles != NULL)
    {
      rv = d->recycled_idles;
      d->recycled_idles = rv->next;
    }
  else
    {
      rv = ROSCO_NEW (RoscoDispatchIdle);
    }
  ROSCO_LIST_APPEND (GET_IDLE_LIST (d), rv);
  rv->func = func;
  rv->func_data = func_data;
  rv->dispatch = d;

  d->base.has_idle = ROSCO_TRUE;

  return rv;
}

void
rosco_dispatch_remove_idle (RoscoDispatchIdle *idle)
{
  if (idle->func != NULL)
    {
      RealDispatch *d = idle->dispatch;
      ROSCO_LIST_REMOVE (GET_IDLE_LIST (d), idle);
      idle->next = d->recycled_idles;
      d->recycled_idles = idle;
      if (d->first_idle == NULL)
        d->base.has_idle = ROSCO_FALSE;
    }
}

static void
generic_signal_handler (int sig)
{
  uint8_t b = sig;
  int rv;
  rosco_assert (sig < 256);
retry_write:
  rv = write (global_signal_dispatch_fd, &b, 1);
  if (rv < 0)
    {
      if (errno == EINTR)
        goto retry_write;
      if (errno == EAGAIN)
        {
          /* TODO: we should do a second strategy here */
        }
      else
        {
          /* should be give a warning or something?
             i have no idea what error we might get */
          const char message1[] = "unexpected error doing signal notification (";
          const char message2[] = ")\n";
          const char *se = strerror (errno);
          write (STDERR_FILENO, message1, sizeof (message1)-1);
          write (STDERR_FILENO, se, strlen (se));
          write (STDERR_FILENO, message2, sizeof (message2)-1);
        }
    }
}

static void
handle_signal_pipe_fd_readable  (RoscoFileDescriptor   fd,
                                 unsigned       events,
                                 void          *callback_data)
{
  RealDispatch *d = callback_data;
  uint8_t buf[1024];
  uint8_t emitted[32];
  int bytes_read;
  ROSCO_UNUSED (events);
retry_read:
  bytes_read = read (fd, buf, sizeof (buf));
  if (bytes_read < 0)
    {
      if (errno == EINTR)
        goto retry_read;
      if (errno == EAGAIN)
        return;
      rosco_warning ("error reading from signal-pipe: %s", strerror (errno));
    }
  else
    {
      unsigned i;
      memset (emitted, 0, 32);
      for (i = 0; i < (unsigned)bytes_read; i++)
        {
          RoscoDispatchSignal *handler;
          /* Guard against repeated emissions of the same signal
             (which is usually a waste of time since signal-handlers
             must guard against dropped repeated sgnals anyway) */
          if (emitted[buf[i]>>3] & (1 << (buf[i] & 7)))
            continue;
          emitted[buf[i]>>3] |= (1 << (buf[i] & 7));

          ROSCO_RBTREE_LOOKUP_COMPARATOR (GET_SIGNAL_TREE (d),
                                        buf[i], COMPARE_SIGNO_TO_SIGNAL,
                                        handler);
          if (handler != NULL)
            {
              handler->is_notifying = 1;
              handler->func (handler->func_data);
              handler->is_notifying = 0;
              if (handler->destroyed_while_notifying)
                rosco_free (handler);
            }
        }
    }
}

RoscoDispatchSignal *
rosco_dispatch_add_signal    (RoscoDispatch     *dispatch,
                            int              signal_number,
                            RoscoSignalHandler func,
                            void            *func_data)
{
  RoscoDispatchSignal *rv;
  RoscoDispatchSignal *conflict;
  RealDispatch *d = (RealDispatch *) dispatch;
  struct sigaction action, oaction;

  /* we restrict signals so they fit in a byte,
     since we communicate the signal-number
     across a stream and we don't want them to get broken up. */
  rosco_return_val_if_fail (signal_number < 256,
                          "currently, only signals <256 can be trapped",
                          NULL);
  if (global_signal_dispatch == NULL)
    {
      global_signal_dispatch = dispatch;
retry_pipe:
      if (pipe (d->signal_pipe_fds) < 0)
        {
          if (errno == EINTR)
            goto retry_pipe;
          rosco_die ("error creating pipe for synchronous signal handling");
        }
      rosco_fd_set_nonblocking (d->signal_pipe_fds[0]);
      rosco_fd_set_nonblocking (d->signal_pipe_fds[1]);
      global_signal_dispatch_fd = d->signal_pipe_fds[1];
      rosco_dispatch_watch_fd (dispatch, d->signal_pipe_fds[0], ROSCO_EVENT_READABLE,
                             handle_signal_pipe_fd_readable, d);
    }
  else if (global_signal_dispatch != dispatch)
    {
      rosco_die ("only one dispatcher may be used to handle signals");
    }
  ROSCO_RBTREE_LOOKUP_COMPARATOR (GET_SIGNAL_TREE (d),
                                signal_number, COMPARE_SIGNO_TO_SIGNAL,
                                rv);
  rosco_return_val_if_fail (rv == NULL, "signal already trapped", NULL);
  memset (&action, 0, sizeof (action));
  action.sa_handler = generic_signal_handler;
  action.sa_flags = SA_NOCLDSTOP;
retry_sigaction:
  if (sigaction (signal_number, &action, &oaction) != 0)
    {
      if (errno == EINTR)
        goto retry_sigaction;
      rosco_warning ("error running sigaction (signal %u): %s",
                   signal_number, strerror (errno));
      return NULL;
    }

  rv = ROSCO_NEW (RoscoDispatchSignal);
  rv->dispatch = d;
  rv->is_notifying = rv->destroyed_while_notifying = 0;
  rv->signal = signal_number;
  rv->func = func;
  rv->func_data = func_data;
  ROSCO_RBTREE_INSERT (GET_SIGNAL_TREE (d), rv, conflict);
  rosco_assert (conflict == NULL);

  return rv;
}
void  rosco_dispatch_remove_signal (RoscoDispatchSignal *sig)
{
  RealDispatch *d = sig->dispatch;
  rosco_assert (!sig->destroyed_while_notifying);

  /* untrap signal handler */
  signal (sig->signal, SIG_DFL);

  /* remove signal handler from tree */
  ROSCO_RBTREE_REMOVE (GET_SIGNAL_TREE (d), sig);

  if (sig->is_notifying)
    sig->destroyed_while_notifying = 1;
  else
    rosco_free (sig);
}

static void
do_waitpid_repeatedly (void *data)
{
  RealDispatch *d = data;
  pid_t pid;
  int status;
  while ((pid=waitpid (-1, &status, WNOHANG)) != -1)
    {
      RoscoDispatchChild *child;
      if (pid == 0)
        break;
      ROSCO_RBTREE_LOOKUP_COMPARATOR (GET_CHILD_TREE (d),
                                    pid, COMPARE_PID_TO_CHILD,
                                    child);
      if (child != NULL)
        {
          RoscoDispatchChildInfo child_info;
          ROSCO_RBTREE_REMOVE (GET_CHILD_TREE (d), child);
          child_info.process_id = pid;
          if (WIFSIGNALED (status))
            {
              child_info.killed = ROSCO_TRUE;
              child_info.value = WTERMSIG (status);
            }
          else
            {
              child_info.killed = ROSCO_FALSE;
              child_info.value = WEXITSTATUS (status);
            }
          child->is_notifying = ROSCO_TRUE;
          child->func (&child_info, child->func_data);
          rosco_free (child);
        }
    }
}
static void
do_waitpid_repeatedly_idle (void *data)
{
  RealDispatch *d = data;
  d->idle_child_handler = NULL;
  do_waitpid_repeatedly (d);
}

RoscoDispatchChild *
rosco_dispatch_add_child    (RoscoDispatch       *dispatch,
                           int                process_id,
                           RoscoChildHandler    func,
                           void              *func_data)
{
  RealDispatch *d = (RealDispatch *) dispatch;
  RoscoDispatchChild *conflict;
  RoscoDispatchChild *child;
  if (d->child_sig_handler == NULL)
    d->child_sig_handler = rosco_dispatch_add_signal (dispatch,
                                                    SIGCHLD,
                                                    do_waitpid_repeatedly,
                                                    d);
  if (d->idle_child_handler == NULL)
    d->idle_child_handler = rosco_dispatch_add_idle (dispatch,
                                                   do_waitpid_repeatedly_idle,
                                                   d);
  

  /* add child to child tree */
  child = ROSCO_NEW (RoscoDispatchChild);
  child->dispatch = d;
  child->process_id = process_id;
  child->func = func;
  child->func_data = func_data;
  child->is_notifying = ROSCO_FALSE;
  ROSCO_RBTREE_INSERT (GET_CHILD_TREE (d), child, conflict);
  rosco_return_val_if_fail (conflict == NULL, "process-id trapped twice", NULL);
  return child;
}

void
rosco_dispatch_remove_child (RoscoDispatchChild  *handler)
{
  RealDispatch *d = handler->dispatch;
  if (handler->is_notifying)
    return;
  ROSCO_RBTREE_REMOVE (GET_CHILD_TREE (d), handler);
  rosco_free (handler);
}


void
rosco_dispatch_destroy_default (void)
{
  if (def)
    {
      RoscoDispatch *to_kill = def;
      def = NULL;
      rosco_dispatch_free (to_kill);
    }
}
