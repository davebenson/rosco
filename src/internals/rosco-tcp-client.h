

typedef enum {
  ROSCO_TCP_CLIENT_INIT,
  ROSCO_TCP_CLIENT_NAME_LOOKUP,
  ROSCO_TCP_CLIENT_CONNECTING,
  ROSCO_TCP_CLIENT_CONNECTED,
  ROSCO_TCP_CLIENT_CLOSED,
  ROSCO_TCP_CLIENT_FAILED
} RoscoTcpClientState;

typedef struct _RoscoTcpClient RoscoTcpClient;
typedef struct _RoscoTcpClientFuncs RoscoTcpClientFuncs;

struct _RoscoTcpClientFuncs {
  void (*connected) (RoscoTcpClient *client);
  void (*new_data_available) (RoscoTcpClient *client);
  void (*outgoing_drained) (RoscoTcpClient *client);
  void (*failed) (RoscoTcpClient *client, RoscoError *error);
  void (*closed) (RoscoTcpClient *client);
};

struct _RoscoTcpClient {
  int fd;
  RoscoTcpClientState state;
  RoscoBuffer incoming;
  RoscoBuffer outgoing;
  
};

RoscoTcpClient *rosco_tcp_client_new                      (const char     *hostname_or_ip,
                                                           uint16_t        port,
                                                           RoscoTcpClientFuncs *funcs,
                                                           void           *func_data,
                                                           RoscoDestroyFunc destroy,
                                                           RoscoError    **error);
void            rosco_tcp_client_outgoing_buffer_changed  (RoscoTcpClient *client);
void            rosco_tcp_client_incoming_buffer_changed  (RoscoTcpClient *client);
void            rosco_tcp_client_close                    (RoscoTcpClient *client);

void            rosco_tcp_client_ref                      (RoscoTcpClient *client);
void            rosco_tcp_client_unref                    (RoscoTcpClient *client);
