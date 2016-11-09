
typedef struct RoscoNode RoscoNode;
typedef struct RoscoNodeFuncs {
  void (*disconnected)      (RoscoNode  *node, 
                             RoscoError *error,
                             rosco_bool  will_retry,
                             void       *funcs_data);
  void (*connected)         (RoscoNode  *node, 
                             void       *funcs_data);
  void (*connect_failed)    (RoscoNode  *node, 
                             RoscoError *error,
                             rosco_bool  will_retry,
                             void       *funcs_data);
  void (*close_started)     (RoscoNode  *node);
  void (*close_succeeded)   (RoscoNode  *node);
  void (*close_failed)      (RoscoNode  *node,
                             RoscoError *error);
} RoscoNodeFuncs;

DskURL             *rosco_master_url            (void);

RoscoNode          *rosco_node_new              (DskURL               *master_url,
						 const char           *name,
                                                 const RoscoNodeFuncs *funcs, 
                                                 void                 *funcs_data,
                                                 RoscoDestroyFunc      funcs_data_destroy);
void               rosco_node_close             (RoscoNode            *node,

// register service implementations
typedef void (*RoscoClosure) (RoscoMessage *output,
                              void         *closure_data);
typedef void (*RoscoServiceFunc)  (RoscoService     *service,
                                   RoscoMessage     *input,
                                   RoscoClosure      response_callback,
                                   void             *closure_data);
                                   
RoscoPublisher    * rosco_node_advertise_topic     (RoscoNode            *node,
                                                    const char           *topic,
                                                    RoscoMessageType     *type);

void                rosco_publisher_publish        (RoscoPublisher       *publisher,
                                                    RoscoMessage         *message);
void                rosco_publisher_close          (RoscoPublisher       *publisher);

RoscoSubscriber    *rosco_node_subscribe_topic     (RoscoNode            *node,
                                                    const char           *topic,
                                                    RoscoMessageType     *type,
                                                    RoscoClosure          handler,
                                                    void                 *handler_data,
                                                    RoscoDestroyFunc      handler_data_destroy);
void                rosco_subscriber_close         (RoscoSubscriber      *subscriber);

RoscoService       *rosco_node_advertise_service(RoscoNode            *node,
                                                 const char           *service_name,
                                                 RoscoMessageType     *input,
                                                 RoscoMessageType     *output,
                                                 RoscoServiceFunc      func,
                                                 void                  func_data,
                                                 RoscoDestroyFunc      func_data_destroy);
void                rosco_service_unregister    (RoscoService         *service);


