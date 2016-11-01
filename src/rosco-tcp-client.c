
#include <netdb.h>


RoscoTcpClient *
rosco_tcp_client_new                      (const char          *hostname_or_ip,
                                           uint16_t             port,
                                           RoscoTcpClientFuncs *funcs,
                                           void                *func_data)
{
  RoscoTcpClient *rv = ROSCO_NEW (RoscoTcpClient);
  rv->fd = -1;
  rv->state = ROSCO_TCP_CLIENT_INIT;
  rosco_buffer_init (&rv->incoming);
  rosco_buffer_init (&rv->outgoing);
  rv->funcs = *funcs;
  rv->func_data = func_data;
  if (try_parse_ipv4 (hostname_or_ip, rv->host.ipv4))
    rv->host_type = ROSCO_TCP_CLIENT_HOST_TYPE_IPV4;
  else if (try_parse_ipv6 (hostname_or_ip, rv->host.ipv6))
    rv->host_type = ROSCO_TCP_CLIENT_HOST_TYPE_IPV6;
  else
    {
      rv->host.name = rosco_strdup (hostname_or_ip);
      rv->host_type = ROSCO_TCP_CLIENT_HOST_TYPE_NAME;
    }
  return rv;
}

static void
client_failed (RoscoTcpClient *client, RoscoErrorCode code, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  RoscoError *error = rosco_error_new_valist (ROSCO_ERROR_BAD_ADDRESS_FAMILY, format, args);
  client->state = ROSCO_TCP_CLIENT_FAILED;
  if (client->funcs.failed != NULL)
    client->funcs.failed (client, error);
  rosco_error_unref (error);
}

static void
handle_client_connect_ready (RoscoFileDescriptor fd,
                             unsigned          events,
                             void             *callback_data)
{
  RoscoTcpClient *client = callback_data;
  assert (client->fd == fd);
  assert ((events & ROSCO_EVENT_WRITABLE) == ROSCO_EVENT_WRITABLE);
  assert (client->state == ROSCO_TCP_CLIENT_CONNECTING);
  int err;
  socklen_t err_size = sizeof (err);
  if (getsockopt (client->fd, SOL_SOCKET, SO_ERROR, &err, &err_size) < 0)
    assert (0);
  if (err == 0)
    {
      client->state = ROSCO_TCP_CLIENT_CONNECTED;
      maybe_do_write (client);
      rosco_dispatch_watch_fd (rosco_dispatch_default (), client->fd,
                               ROSCO_EVENT_READABLE | (client->outgoing.size > 0 ? ROSCO_EVENT_WRITABLE : 0),
                               handle_connected_client_io, client);
    }
  else
    {
}

void
rosco_tcp_client_connect (RoscoTcpClient *client)
{
  RoscoTcpClientHostType host_type = client->host_type;
  struct sockaddr_storage addr;
  memset (&addr, 0, sizeof (addr));
  int address_family = AF_INET;
  if (host_type == ROSCO_TCP_CLIENT_HOST_TYPE_NAME)
    {
      struct hostent *ent = gethostbyname(client->host.name);		// NOTE: blocks!!!
      if (ent == NULL)
        {
          client_failed (client, ROSCO_ERROR_NAME_LOOKUP_FAILED, "host looking for '%s' failed", client->host.name);
	  return;
        }
      if (ent->h_addrtype == AF_INET)
        {
          struct sockaddr_in *sa = (struct sockaddr_in *) addr;
          sa->sa_family = AF_INET;
          sa->sa_port = htons (client->port);
          memcpy (sa->sa_data, ent->h_addr_list[0], 4);
        }
      else if (ent->h_addrtype == AF_INET6)
        {
          struct sockaddr_in6 *sa = (struct sockaddr_in6 *) addr;
          sa->sa_family = AF_INET6;
          sa->sa_port = htons (client->port);
          memcpy (sa->sa_data, ent->h_addr_list[0], 16);
	  address_family = AF_INET6;
        }
      else
        {
          client_failed (client, ROSCO_ERROR_BAD_ADDRESS_FAMILY, "only ipv4 and ipv6 supported");
          return;
        }
    }
  else if (host_type == ROSCO_TCP_CLIENT_HOST_TYPE_IPV4)
    {
      struct sockaddr_in *sa = (struct sockaddr_in *) addr;
      sa->sa_family = AF_INET;
      sa->sa_port = htons (client->port);
      memcpy (sa->sa_data, client->host.ipv4, 4);
    }
  else if (host_type == ROSCO_TCP_CLIENT_HOST_TYPE_IPV6)
    {
      struct sockaddr_in6 *sa = (struct sockaddr_in6 *) addr;
      sa->sa_family = AF_INET6;
      sa->sa_port = htons (client->port);
      memcpy (sa->sa_data, client->host.ipv4, 16);
      address_family = AF_INET6;
    }
  else
    assert(0);                // SHOULD NOT HAPPEN

  // create socket
  client->fd = socket (address_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (client->fd < 0)
    {
      client_failed (client, ROSCO_ERROR_BAD_ADDRESS_FAMILY, "creating socket failed: %s", strerror (errno));
      return;
    }

#if 0
  // set nonblocking
#endif

  // do connect
  if (connect (client->fd, &addr, sizeof (addr)) < 0)
    {
      if (errno == EINPROGRESS)
        {
          rosco_dispatch_watch_fd (rosco_dispatch_default (), client->fd, ROSCO_EVENT_WRITABLE,
                                   handle_fd_connect, client);
        }
      else
        {
          client_failed (client, ROSCO_ERROR_CONNECT_FAILED,
                         "error connecting to remote host: %s", strerror (errno));
          return;
        }
    }
}

void
rosco_tcp_client_outgoing_buffer_changed  (RoscoTcpClient *client)
{
  if (!client->trapping_writable && client->incoming.size > 0)
    {
      rosco_dispatch_watch_fd (rosco_dispatch_default (),
                               client->fd,
                               ROSCO_EVENT_READABLE|ROSCO_EVENT_WRITABLE,
                               client);
      client->trapping_writable = 1;
    }
}

void
rosco_tcp_client_incoming_buffer_changed  (RoscoTcpClient *client)
{
  (void) client;
}

void
rosco_tcp_client_close (RoscoTcpClient *client)
{
  if (client->fd >= 0)
    {
      rosco_dispatch_close_fd (rosco_dispatch_default (), client->fd);
      client->fd = -1;
    }
}

void
rosco_tcp_client_ref (RoscoTcpClient *client)
{
  assert(client->ref_count > 0);
  ++(client->ref_count);
}
void
rosco_tcp_client_unref (RoscoTcpClient *client)
{
  assert(client->ref_count > 0);
  if (--(client->ref_count) == 0)
    {
      rosco_tcp_client_close (client);
      rosco_buffer_init (&client->incoming);
      rosco_buffer_init (&client->outgoing);
      if (client->host_type == ROSCO_TCP_CLIENT_HOST_TYPE_NAME)
        rosco_free (client->host.name);
      rosco_free (client);
    }
}
