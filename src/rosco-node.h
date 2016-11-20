#ifndef __ROSCO_NODE_H_
#define __ROSCO_NODE_H_

#include "rosco-type-system.h"

typedef struct RoscoPublisher RoscoPublisher;
typedef struct RoscoSubscriber RoscoSubscriber;
typedef struct RoscoService RoscoService;
typedef struct RoscoServiceClient RoscoServiceClient;
typedef struct RoscoNode RoscoNode;

typedef struct RoscoNodeFuncs {
  void (*disconnected)      (RoscoNode   *node, 
                             DskError    *error,
                             dsk_boolean  will_retry,
                             void        *funcs_data);
  void (*connected)         (RoscoNode   *node, 
                             void        *funcs_data);
  void (*connect_failed)    (RoscoNode   *node, 
                             DskError    *error,
                             dsk_boolean  will_retry,
                             void        *funcs_data);
  void (*close_started)     (RoscoNode   *node);
  void (*close_succeeded)   (RoscoNode   *node);
  void (*close_failed)      (RoscoNode   *node,
                             DskError    *error);
} RoscoNodeFuncs;

DskUrl             *rosco_master_url            (void);

RoscoNode          *rosco_node_new              (DskUrl               *master_url,
						 const char           *name,
                                                 const RoscoNodeFuncs *funcs, 
                                                 void                 *funcs_data,
                                                 DskDestroyNotify      funcs_data_destroy);

typedef void (*RoscoNodeCloseHandler)(DskError *error, void *handler_data);
void               rosco_node_close             (RoscoNode            *node,
                                                 RoscoNodeCloseHandler handler,
                                                 void                 *handler_data);

//           === Topics ===
//
// publishers and subscribers are both owned by the node,
// so you only need to destroy them if you are managing pub/subs dynamically.
// Most simple nodes will just have a fairly fixed set of pub/subs.
//

// Topic Publisher API
RoscoPublisher    * rosco_publisher_new            (RoscoNode            *node,
                                                    const char           *topic,
                                                    RoscoMessageType     *type);
void                rosco_publisher_destroy        (RoscoPublisher       *publisher);
void                rosco_publisher_publish        (RoscoPublisher       *publisher,
                                                    RoscoMessage         *message);


// Topic Subscriber API
typedef void (*RoscoSubscriptionHandler) (RoscoMessage *output,
                                          void         *closure_data);

RoscoSubscriber *   rosco_subscriber_new           (RoscoNode            *node,
                                                    const char           *topic,
                                                    RoscoMessageType     *type,
                                                    RoscoSubscriptionHandler   handler,
                                                    void                 *handler_data,
                                                    DskDestroyNotify      handler_data_destroy);
void                rosco_subscriber_close         (RoscoSubscriber      *subscriber);



//                   === Services ===

// Service implementations.
typedef void (*RoscoClosure) (DskError     *error,
                              RoscoMessage *output,
                              void         *closure_data);
typedef void (*RoscoServiceFunc)  (RoscoService     *service,
                                   RoscoMessage     *input,
                                   RoscoClosure      response_callback,
                                   void             *closure_data);
                                   
RoscoService       *rosco_node_advertise_service (RoscoNode            *node,
                                                  const char           *service_name,
                                                  RoscoMessageType     *input,
                                                  RoscoMessageType     *output,
                                                  RoscoServiceFunc      func,
                                                  void                 *func_data,
                                                  DskDestroyNotify      func_data_destroy);
void                rosco_service_unregister     (RoscoService         *service);


// Invoking services.
RoscoServiceClient *rosco_node_service_client_new(RoscoNode            *node,
                                                  const char           *service_name);
void                rosco_service_client_call    (RoscoServiceClient   *service_client,
                                                  RoscoMessage         *input,
                                                  RoscoClosure          closure,
                                                  void                 *closure_data,
                                                  DskDestroyNotify      func_data_destroy);
void                rosco_service_client_unregister (RoscoService         *service);



RoscoService       *rosco_node_advertise_service(RoscoNode            *node,
                                                 const char           *service_name,
                                                 RoscoMessageType     *input,
                                                 RoscoMessageType     *output,
                                                 RoscoServiceFunc      func,
                                                 void                 *func_data,
                                                 DskDestroyNotify      func_data_destroy);
void                rosco_service_unregister    (RoscoService         *service);



#endif
