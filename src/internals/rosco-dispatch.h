#ifndef __ROSCO_DISPATCH_H_
#define __ROSCO_DISPATCH_H_

typedef struct _RoscoDispatch RoscoDispatch;
typedef struct _RoscoDispatchTimer RoscoDispatchTimer;
typedef struct _RoscoDispatchIdle RoscoDispatchIdle;
typedef struct _RoscoDispatchSignal RoscoDispatchSignal;
typedef struct _RoscoDispatchChild RoscoDispatchChild;

typedef enum
{
  ROSCO_EVENT_READABLE = (1<<0),
  ROSCO_EVENT_WRITABLE = (1<<1)
} Rosco_Events;

/* Create or destroy a Dispatch */
RoscoDispatch        *rosco_dispatch_new (void);
void                rosco_dispatch_free(RoscoDispatch *dispatch);

RoscoDispatch        *rosco_dispatch_default (void);


/* Registering file-descriptors to watch. */
typedef void (*RoscoFDFunc)  (RoscoFileDescriptor fd,
                            unsigned          events,
                            void             *callback_data);
void  rosco_dispatch_watch_fd (RoscoDispatch        *dispatch,
                             RoscoFileDescriptor   fd,
                             unsigned            events,
                             RoscoFDFunc           callback,
                             void               *callback_data);
void  rosco_dispatch_close_fd (RoscoDispatch        *dispatch,
                             RoscoFileDescriptor   fd);
void  rosco_dispatch_fd_closed(RoscoDispatch        *dispatch,
                             RoscoFileDescriptor   fd);

/* Timers */
typedef void (*RoscoTimerFunc) (void              *func_data);
RoscoDispatchTimer *
      rosco_dispatch_add_timer       (RoscoDispatch       *dispatch,
                                    unsigned           timeout_secs,
                                    unsigned           timeout_usecs,
                                    RoscoTimerFunc       func,
                                    void              *func_data);
void  rosco_dispatch_adjust_timer    (RoscoDispatchTimer *timer,
                                    unsigned           timeout_secs,
                                    unsigned           timeout_usecs);

/* if possible, these are keyed off a clock that ignores clock-resets */
RoscoDispatchTimer *
      rosco_dispatch_add_timer_millis(RoscoDispatch       *dispatch,
                                    uint64_t           milliseconds,
                                    RoscoTimerFunc       func,
                                    void              *func_data);
void  rosco_dispatch_adjust_timer_millis (RoscoDispatchTimer *timer,
                                        uint64_t          milliseconds);

void  rosco_dispatch_remove_timer (RoscoDispatchTimer *);


/* querying the timer (UNIMPLEMENTED) */

rosco_boolean  rosco_dispatch_timer_is_wallclock (RoscoDispatchTimer *);
uint64_t     rosco_dispatch_timer_get_expiration_millis (RoscoDispatchTimer *);


/* see effective-timers.txt
   
 */
rosco_boolean  rosco_supports_effective_timers (void);

/* Idle functions */
typedef void (*RoscoIdleFunc)   (void               *func_data);
RoscoDispatchIdle *
      rosco_dispatch_add_idle (RoscoDispatch *dispatch,
                                    RoscoIdleFunc func,
                                    void               *func_data);
void  rosco_dispatch_remove_idle (RoscoDispatchIdle *);

/* Signal handling */
typedef void (*RoscoSignalHandler) (void *func_data);
RoscoDispatchSignal *
      rosco_dispatch_add_signal    (RoscoDispatch     *dispatch,
                                  int              signal_number,
                                  RoscoSignalHandler func,
                                  void            *func_data);
void  rosco_dispatch_remove_signal (RoscoDispatchSignal *signal);

/* Process termination */
typedef struct _RoscoDispatchChildInfo RoscoDispatchChildInfo;
struct _RoscoDispatchChildInfo
{
  int process_id;
  rosco_boolean killed;           /* killed by signal */
  int value;                    /* exit status or signal number */
};
typedef void (*RoscoChildHandler) (RoscoDispatchChildInfo  *info,
                                 void                  *func_data);
RoscoDispatchChild *
      rosco_dispatch_add_child    (RoscoDispatch       *dispatch,
                                 int                process_id,
                                 RoscoChildHandler    func,
                                 void              *func_data);
void  rosco_dispatch_remove_child (RoscoDispatchChild  *handler);

/* --- API for use in standalone application --- */
/* Where you are happy just to run poll(2). */

/* rosco_dispatch_run() 
 * Run one main-loop iteration, using poll(2) (or some system-level event system).
 */
void  rosco_dispatch_run      (RoscoDispatch *dispatch);



/* --- API for those who want to embed a dispatch into their own main-loop --- */
typedef struct _RoscoFileDescriptorNotify RoscoFileDescriptorNotify;
struct _RoscoFileDescriptorNotify
{
  RoscoFileDescriptor fd;
  Rosco_Events events;
};

typedef struct _RoscoFileDescriptorNotifyChange RoscoFileDescriptorNotifyChange;
struct _RoscoFileDescriptorNotifyChange
{
  RoscoFileDescriptor fd;
  Rosco_Events old_events;
  Rosco_Events events;
};

void  rosco_dispatch_dispatch (RoscoDispatch             *dispatch,
                             size_t                   n_notifies,
                             RoscoFileDescriptorNotify *notifies);
void  rosco_dispatch_clear_changes (RoscoDispatch *);


struct _RoscoDispatch
{
  /* changes to the events you are interested in. */
  /* (this handles closed file-descriptors 
     in a manner agreeable to epoll(2) and kqueue(2)) */
  size_t n_changes;
  RoscoFileDescriptorNotifyChange *changes;

  /* the complete set of events you are interested in. */
  size_t n_notifies_desired;
  RoscoFileDescriptorNotify *notifies_desired;

  /* If TRUE, return immediately. */
  rosco_boolean has_idle;

  /* number of milliseconds to wait if no events occur */
  rosco_boolean has_timeout;
  unsigned long timeout_secs;
  unsigned timeout_usecs;

  unsigned long last_dispatch_secs;
  unsigned last_dispatch_usecs;

  /* private data follows (see RealDispatch structure in .c file) */
};

void rosco_dispatch_destroy_default (void);

#endif
