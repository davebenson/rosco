#include "rosco-http-client.h"
#include "rosco-tcp-client.h"

#define ROSCO_LOG_CATEGORY "rosco-http-client"

struct _RoscoHttpClient_Connection {
  RoscoTcpClient *tcp;

  RoscoHttpClient_HostPool *host_pool;

  // for kept-alive connections only
  RoscoHttpClient_Connection *keptalive_prev, *keptalive_next;
};

struct _RoscoHttpClient_HostPool {
  char *hostname;
  unsigned port;
  RoscoHttpClient_Connection *keptalive_connection_list;

  RoscoHttpClient_HostPool *left, *right, *parent;
  unsigned is_red:1;
};

struct _RoscoHttpClientRequest {
  RoscoHttpClient_Connection *connection;
};

struct _RoscoHttpClient {
  RoscoHttpClient_HostPool *hosts;
};

static void
rosco_http_client__tcp_client__connected (RoscoTcpClient *client)
{
  ROSCO_DEBUG("connected to server");
}

static void
rosco_http_client__tcp_client__new_data_available (RoscoTcpClient *client)
{
  // scan header
  ...

  // parse contents
  ...
}

static void
rosco_http_client__tcp_client__outgoing_drained (RoscoTcpClient *client)
{
  // TODO: for now there's no real contention support
}

static void
rosco_http_client__tcp_client__failed (RoscoTcpClient *client, RoscoError *error)
{
  ...
}

static void
rosco_http_client__tcp_client__closed (RoscoTcpClient *client)
{
  ...
}

static void
rosco_http_client__tcp_client__destroyed (void *client)
{
}

void rosco_http_client_make_request (RoscoHttpClient *client,
                                     RoscoHttpRequest *request,
                                     RoscoHttpResponseHandler handler,
                                     void             *handler_data,
                                     RoscoDestroyFunc  handler_data_destroy)
{
  RoscoHttpClient_HostPool exemplar;
  exemplar.hostname = request->host;
  exemplar.port = request->port;
  RoscoHttpClient_HostPool *host_pool;
  DSK_RBTREE_LOOKUP (GET_HOST_POOL_TREE (client), exemplar, host_pool);
  RoscoHttpClient_Connection *conn = NULL;
  if (out != NULL)
    {
      conn = out->keptalive_connection_list;
      if (conn != NULL)
        {
          .. remove from list
        }
    }

  if (conn == NULL)
    {
      if (host_pool == NULL)
        {
          // create host-pool
          host_pool = ROSCO_NEW (RoscoHttpClient_HostPool);
          host_pool->hostname = rosco_strdup (request->host);
          host_pool->port = request->port;
          host_pool->keptalive_connection_list = NULL;
          RoscoHttpClient_HostPool *conflict;
          DSK_RBTREE_INSERT (GET_HOST_POOL_TREE (client), host_pool, conflict);
          assert (conflict == NULL);
        }

      // create connection
      conn = ROSCO_NEW (RoscoHttpClient_Connection);
      RoscoError *error = NULL;
      conn->tcp = rosco_tcp_client_new (request->host, request->port, &tcp_client_funcs, conn, &error);
      conn->host_pool = host_pool;
    }

  // Send request via TCP client.
  rosco_http_request_to_buffer (request, &tcp->outgoing);
  if (request->has_body)
    {
      rosco_buffer_printf (&tcp->outgoing, "Content-Length: %zu\n", request->body_length);
      rosco_buffer_append (&tcp->outgoing, (const uint8_t *) "\n", 1);
      rosco_buffer_append (&tcp->outgoing, request->body_data, request->body_length);
    }
  else
    {
      rosco_buffer_append (&tcp->outgoing, (const uint8_t *) "\n", 1);
    }
}
