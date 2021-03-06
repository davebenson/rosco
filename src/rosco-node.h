
typedef struct RoscoURL RoscoURL;
RoscoURL      *rosco_url_new           (const char   *str,
                                        RoscoError  **error);
RoscoURL      *rosco_url_new_append    (RoscoURL     *url,
                                        const char   *format,
                                        ...);
void           rosco_url_ref           (RoscoURL     *url);
void           rosco_url_unref         (RoscoURL     *url);

typedef struct RoscoNode RoscoNode;
typedef struct RoscoNodeFuncs {
  void (*disconnected)  (RoscoNode  *node, 
                         RoscoError *error,
                         rosco_bool  will_retry,
                         void       *funcs_data);
  void (*connected)     (RoscoNode  *node, 
                         void       *funcs_data);
  void (*connect_failed)(RoscoNode  *node, 
                         RoscoError *error,
                         rosco_bool  will_retry,
                         void       *funcs_data);
} RoscoNodeFuncs;

RoscoNode          *rosco_node_new              (RoscoURL             *master_url,
                                                 const RoscoNodeFuncs *funcs, 
                                                 void                 *funcs_data,
                                                 RoscoDestroyFunc      funcs_data_destroy);

// register service implementations
typedef void (*RoscoServiceFunc)  (RoscoService     *service,
                                   RoscoMessage     *input,
                                   RoscoClosure      response_callback,
                                   void             *response_callback_data);
                                   

RoscoService       *rosco_node_register_service (RoscoNode            *node,
                                                 const char           *service_name,
                                                 RoscoMessageType     *input,           //???
                                                 RoscoMessageType     *output,          //???
                                                 RoscoServiceFunc      func,
                                                 void                  func_data,
                                                 RoscoDestroyFunc      func_data_destroy);
void                rosco_service_unregister    (RoscoService         *service);

