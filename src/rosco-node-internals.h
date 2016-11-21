
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
    } faulted;
  } info;
} RoscoNodeExecuteResult;
 

typedef void (*RoscoNodeExecuteCallback)(RoscoNodeExecuteResult *result,
                                         void                   *callback_data);


void rosco_node_execute_on_master (RoscoNode                 *node,
                                   const char                *method,
                                   unsigned                   n_params,
                                   DskXmlrpcValue           **params,
                                   RoscoNodeExecuteCallback   callback,
                                   void                      *callback_data);
