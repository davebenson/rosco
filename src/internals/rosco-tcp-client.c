
g       #include <netdb.h>
static rosco_bool



RoscoTcpClient *rosco_tcp_client_new                      (const char          *hostname_or_ip,
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
          client->state = ROSCO_TCP_CLIENT_FAILED;
          if (client->funcs.failed != NULL)
            {
              RoscoError *error = rosco_error_new (ROSCO_ERROR_NAME_LOOKUP_FAILED, "host looking for '%s' failed", client->host.name);
              client->funcs.failed (client, error);
              rosco_error_unref (error);
            }
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
	  RoscoError *error = rosco_error_new (ROSCO_ERROR_BAD_ADDRESS_FAMILY, "only ipv4 and ipv6 supported");
          if (client->funcs.failed != NULL)
	    client->funcs.failed (client, error);
	  rosco_error_unref (error);
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
      ...
    }

#if 0
  // set nonblocking
#endif

  // do connect
  if (connect (client->fd, &addr, sizeof (addr)) < 0)
    {
      ...
    }
}

void
rosco_tcp_client_outgoing_buffer_changed  (RoscoTcpClient *client)
{
...
}

void
rosco_tcp_client_incoming_buffer_changed  (RoscoTcpClient *client)
{
...
}

void
rosco_tcp_client_close (RoscoTcpClient *client)
{
...
}

void
rosco_tcp_client_ref (RoscoTcpClient *client)
{
...
}
void
rosco_tcp_client_unref (RoscoTcpClient *client)
{
...
}
