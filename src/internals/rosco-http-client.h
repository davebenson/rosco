

// maintains a pool of connections
typedef struct _RoscoHttpClient RoscoHttpClient;

RoscoHttpClient *rosco_http_client_new (void);

struct _RoscoHttpRequest {
  char *host;
  uint16_t port;

  char *method;
  char *path;

  char **headers;               // key-value pairs

  unsigned has_body : 1;
  
  size_t body_length;
  uint8_t *body_data;

  unsigned ref_count;
  RoscoHttpRequestState state;
};


struct _RosHttpResponse {
  RosHttpStatusCode status_code;
  char *reason;
  char **headers;
  size_t body_length;
  uint8_t *body_data;
};

RoscoHttpClient * rosco_http_client_default (void);

void rosco_http_client_make_request (RoscoHttpClient *client,
                                     RoscoHttpRequest *request,
                                     RoscoHttpResponseHandler handler,
                                     void             *handler_data);
