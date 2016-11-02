
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

RoscoNode          *rosco_node_new              (DskURL               *master_url,
                                                 const RoscoNodeFuncs *funcs, 
                                                 void                 *funcs_data,
                                                 RoscoDestroyFunc      funcs_data_destroy);

// register service implementations
typedef void (*RoscoClosure) (RoscoMessage *output,
                              void         *closure_data);
typedef void (*RoscoServiceFunc)  (RoscoService     *service,
                                   RoscoMessage     *input,
                                   RoscoClosure      response_callback,
                                   void             *closure_data);
                                   

RoscoService       *rosco_node_register_service (RoscoNode            *node,
                                                 const char           *service_name,
                                                 RoscoMessageType     *input,
                                                 RoscoMessageType     *output,
                                                 RoscoServiceFunc      func,
                                                 void                  func_data,
                                                 RoscoDestroyFunc      func_data_destroy);
void                rosco_service_unregister    (RoscoService         *service);

