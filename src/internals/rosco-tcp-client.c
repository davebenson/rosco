
static rosco_bool



RoscoTcpClient *rosco_tcp_client_new                      (const char          *hostname_or_ip,
                                                           uint16_t             port,
                                                           RoscoTcpClientFuncs *funcs,
                                                           void                *func_data,
                                                           RoscoError         **error)
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
  if (host_type == ROSCO_TCP_CLIENT_HOST_TYPE_NAME)
    {
      ...
    }
  
  if (host_type == ROSCO_TCP_CLIENT_HOST_TYPE_IPV4)
    {
      ...
    }
  else if (host_type == ROSCO_TCP_CLIENT_HOST_TYPE_IPV6)
    {
      assert(0);                //TODO
    }
  else
    assert(0);                // SHOULD NOT HAPPEN
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
