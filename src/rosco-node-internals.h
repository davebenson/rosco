
typedef enum {
  ROSCO_NODE_EXECUTE_RESULT_SUCCESS,
  ROSCO_NODE_EXECUTE_RESULT_TIMED_OUT,
  ROSCO_NODE_EXECUTE_RESULT_FAILED,
  ROSCO_NODE_EXECUTE_RESULT_FAULTED
} RoscoNodeExecuteResultType;
typedef struct {
  RoscoNodeExecuteResultType result;
  union {
    struct {
      DskXml *response;
      unsigned n_params;
      DskXmlrpcValue **params;
    } success;

    // low-level failure
    struct {
      DskError *error;
    } failed;

    // XMLRPC call gave error response
    struct {
      char *message;
      uint32_t code;
      DskXmlrpcValue *error_value;		// always a struct containing the other values
    } faulted
  } info;
} RoscoNodeExecuteResult;
 

typedef void (*RoscoNodeExecuteCallback)(RoscoNodeExecuteResult *result,
                                         void                   *callback_data);


void rosco_node_execute_on_master (RoscoNode                 *node,
                                   const char                *method,
                                   unsigned                   n_params,
                                   const DskXmlrpcValue     **params,
                                   RoscoNodeExecuteCallback   callback,
                                   void                      *callback_data);

/* --- actual known master-node APIs --- */
void rosco_node_master_execute_shutdown (RoscoNode                 *node,
                                         const char                *message,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
void rosco_node_master_execute_get_uri  (RoscoNode                 *node,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
void rosco_node_master_execute_get_pid  (RoscoNode                 *node,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
void rosco_node_master_execute_delete_param     (RoscoNode                 *node,
                                         const char                *key,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
void rosco_node_master_execute_set_param(RoscoNode                 *node,
                                         const char                *key,
                                         DskXmlrpcValue            *value,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
void rosco_node_master_execute_get_param(RoscoNode                 *node,
                                         const char                *key,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);

void rosco_node_master_execute_subscribe_param(RoscoNode                 *node,
                                         const char                *key,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
                     
void rosco_node_master_execute_unsubscribe_param(RoscoNode                 *node,
                                         const char                *key,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
void rosco_node_master_execute_has_param(RoscoNode                 *node,
                                         const char                *key,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
void rosco_node_master_execute_get_param_names(RoscoNode                 *node,
                                         const char                *key,
			                 RoscoNodeExecuteCallback   callback,
			                 void                      *callback_data);
