
/* --- Defining our XMLRPC services --- */
typedef struct RoscoXmlrpcServer RoscoXmlrpcServer;
typedef struct RoscoXmlrpcClosure RoscoXmlrpcClosure;
typedef void (*RoscoXmlrpcMethodFunc) (size_t                  n_params,
                                       const DskXmlrpcValue  **params,
                                       RoscoXmlrpcClosure     *closure,
                                       void                   *method_data);
RoscoXmlrpcServer *rosco_xmlrpc_server_new (void);
void rosco_register_xmlrpc_method  (RoscoXmlrpcServer         *server,
                                    const char                *method_name,
                                    RoscoXmlrpcMethodFunc  method_func,
                                    void                      *method_data,
                                    DskDestroyNotify           method_data_destroy);
void rosco_xmlrpc_closure_success  (RoscoXmlrpcClosure    *closure,
                                    size_t                     n_params,
                                    const DskXmlrpcValue     **params);
void rosco_xmlrpc_closure_fault    (RoscoXmlrpcClosure    *closure,
                                    uint32_t                   code,
                                    const char                *message);

void rosco_xmlrpc_closure_add_to_server (RoscoXmlrpcServer *xmlrpc_server,
                                         const char        *path,
                                         DskHttpServer     *server);
