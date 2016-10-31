#include "rosco-http-client.h"
#include "rosco-tcp-client.h"

#define ROSCO_LOG_CATEGORY "rosco-http-client"

typedef enum
{
  ROSCO_HTTP_CLIENT_CONNECTION_STATE_CONNECTING,
  ROSCO_HTTP_CLIENT_CONNECTION_STATE_IDLE,
  ROSCO_HTTP_CLIENT_CONNECTION_STATE_AWAITING_RESPONSE_HEADER,
  ROSCO_HTTP_CLIENT_CONNECTION_STATE_AWAITING_RESPONSE_BODY,
} RoscoHttpClientConnectionState;

struct _RoscoHttpClient_Connection {
  RoscoTcpClient *tcp_client;
  RoscoHttpClient_HostPool *host_pool;          // owner
  RoscoHttpClientConnectionState state;

  // handler and http parsing state
  ...

  // for kept-alive connections only
  RoscoHttpClient_Connection *keptalive_prev, *keptalive_next;
};

struct _RoscoHttpClient_HostPool {
  RoscoHttpClient *http_client;         // owner
  char *hostname;
  unsigned port;
  RoscoHttpClient_Connection *keptalive_connection_list;

  RoscoDispatchTimer *keptalive_reaper;

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
  RoscoHttpClient_Connection *connection = client->func_data;
  ROSCO_DEBUG("connected to server");
}

static rosco_bool
can_keepalive (RoscoHttpClient_Connection *connection)
{
...
}

static void
handle_keepalive_timer (void *func_data)
{
  RoscoHttpClient_HostPool *host_pool = func_data;
  ..
}

static void
do_keepalive (RoscoHttpClient_Connection *connection)
{
  connection->state = ROSCO_HTTP_CLIENT_CONNECTION_STATE_IDLE;
  DSK_QUEUE_ENQUEUE (HOST_POOL_GET_KEPTALIVE_CONNECTION_QUEUE (connection->host_pool), connection);
  if (connection->host_pool->keptalive_reaper == NULL)
    {
      connection->host_pool->keptalive_reaper = rosco_dispatch_add_timer_millis (rosco_dispatch_default (), 
                                                                                 CLIENT_KEEPALIVE_MILLISECONDS,
                                                                                 handle_keepalive_timer,
                                                                                 connection->host_pool);
    }
}

static void
destroy_connection (RoscoHttpClient_Connection *connection)
{
  connection->state = ROSCO_HTTP_CLIENT_CONNECTION_STATE_CLOSED;
  rosco_tcp_client_close (connection->tcp_client);
  ...
  rosco_free (connection);
}

static void
rosco_http_client__tcp_client__new_data_available (RoscoTcpClient *client)
{
  RoscoHttpClient_Connection *connection = client->func_data;
restart_processing:
  switch (connection->state)
    {
    case ROSCO_HTTP_CLIENT_CONNECTION_STATE_AWAITING_RESPONSE_HEADER: {
      // scan header
      size_t n_bytes;
      RoscoError *error = NULL;
      switch (rosco_http_scan_header (&client->incoming, &n_bytes, &error))
        {
        case ROSCO_HTTP_SCAN_STATUS_SUCCESS:
          break;
        case ROSCO_HTTP_SCAN_STATUS_INCOMPLETE:
          return;
        case ROSCO_HTTP_SCAN_STATUS_ERROR:
          fail_connection (ROSCO_ERROR..., "unable to parse HTTP header: %s", error->message);
          rosco_error_unref (error);
          return;
        }

      // parse header
      ...

      if (expecting_body (...))
        {
          connection->state = ROSCO_HTTP_CLIENT_CONNECTION_STATE_AWAITING_RESPONSE_BODY;
          goto restart_processing;
        }
      else
        {
          connection->handler (..., connection->handler_data);
          if (can_keepalive (connection))
            do_keepalive (connection);
          else
            destroy_connection (connection);
          ... go back to keptalive unless close
        }
      break;
    }
  case ROSCO_HTTP_CLIENT_CONNECTION_STATE_AWAITING_RESPONSE_BODY:
    {
      ...
    }
  default:
    {
      if (client->incoming.size > 0)
        {
          fail_connection (ROSCO_ERROR_UNEXPECTED_DATA, "HTTP client not expecting incoming data in keepalive state");
          return;
        }
      break;
  }
}

static void
rosco_http_client__tcp_client__outgoing_drained (RoscoTcpClient *client)
{
  RoscoHttpClient_Connection *connection = client->func_data;
  (void) connection;
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
  if (host_pool != NULL)
    {
      DSK_QUEUE_DEQUEUE(HOST_POOL_GET_KEPTALIVE_CONNECTION_QUEUE (host_pool), conn);
    }
  if (host_pool == NULL)
    {
      host_pool = ROSCO_NEW (RoscoHttpClient_HostPool);
      host_pool->hostname = rosco_strdup (request->host);
      host_pool->port = request->port;
      host_pool->keptalive_connection_list = NULL;
      host_pool->keptalive_reaper = NULL;
      RoscoHttpClient_HostPool *conflict;
      DSK_RBTREE_INSERT (GET_HOST_POOL_TREE (client), host_pool, conflict);
      assert (conflict == NULL);
    }
  assert (host_pool != NULL);
  if (conn == NULL)
    {
      conn = DSK_NEW (RoscoHttpClient_Connection);
      conn->host_pool = host_pool;
      conn->state = ROSCO_HTTP_CLIENT_CONNECTION_STATE_CONNECTING;
      conn->keptalive_prev = conn->keptalive_next = NULL;
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
