

typedef enum {
  ROSCO_TCP_CLIENT_INIT,
  ROSCO_TCP_CLIENT_NAME_LOOKUP,
  ROSCO_TCP_CLIENT_CONNECTING,
  ROSCO_TCP_CLIENT_CONNECTED,
  ROSCO_TCP_CLIENT_CLOSED,
  ROSCO_TCP_CLIENT_FAILED
} RoscoTcpClientState;

typedef enum {
  ROSCO_TCP_CLIENT_HOST_TYPE_IPV4,
  ROSCO_TCP_CLIENT_HOST_TYPE_IPV6,
  ROSCO_TCP_CLIENT_HOST_TYPE_NAME
} RoscoTcpClientHostType;

typedef struct _RoscoTcpClient RoscoTcpClient;
typedef struct _RoscoTcpClientFuncs RoscoTcpClientFuncs;

struct _RoscoTcpClientFuncs {
  void (*connected) (RoscoTcpClient *client);
  void (*new_data_available) (RoscoTcpClient *client);
  void (*outgoing_drained) (RoscoTcpClient *client);
  void (*failed) (RoscoTcpClient *client, RoscoError *error);
  void (*closed) (RoscoTcpClient *client);

  // called at the end
  void (*destroyed) (RoscoTcpClient *client);
};

struct _RoscoTcpClient {
  // after modifying, you must call rosco_tcp_client_{incoming,outgoing}_buffer_changed
  RoscoBuffer incoming;
  RoscoBuffer outgoing;
  
  // read-only
  int fd;
  RoscoTcpClientState state;
  RoscoTcpClientFuncs funcs;
  void *func_data;

  RoscoTcpClientState state;
  RoscoTcpClientHostType host_type;
  union {
    uint8_t ipv4[4];
    uint8_t ipv6[16];
    char *name;
  } host;

  uint16_t port;
};

RoscoTcpClient *rosco_tcp_client_new                      (const char          *hostname_or_ip,
                                                           uint16_t             port,
                                                           RoscoTcpClientFuncs *funcs,
                                                           void                *func_data,
                                                           RoscoDestroyFunc     destroy);
void            rosco_tcp_client_connect                  (RoscoTcpClient *client);
void            rosco_tcp_client_outgoing_buffer_changed  (RoscoTcpClient *client);
void            rosco_tcp_client_incoming_buffer_changed  (RoscoTcpClient *client);
void            rosco_tcp_client_close                    (RoscoTcpClient *client);

void            rosco_tcp_client_ref                      (RoscoTcpClient *client);
void            rosco_tcp_client_unref                    (RoscoTcpClient *client);
