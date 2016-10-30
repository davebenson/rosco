
typedef void (*XmlrpcHandler) (RoscoMutableXML_Document *response,
                               void *handler_data);

static void
do_xmlrpc (RoscoURL *base_url,
           const char *method,
           RoscoMutableXML_Document *input,
           XmlrpcHandler handler,
           void *handler_data)
{
  RoscoHttpRequest *request = rosco_http_request_new ("POST", ...);
  rosco_http_request_add_header (request, "Content-Type", "text/xml");
  RoscoBinaryData *data = rosco_mutable_xml_serialize (input);
  rosco_http_request_set_body (request, data);
  rosco_http_client_make_request (rosco_http_client_default (),
                                  request,
                                  handle_http_response,
                                  call,
                                  NULL);
}

RoscoNode          *rosco_node_new              (RoscoURL             *master_url,
                                                 const RoscoNodeFuncs *funcs, 
                                                 void                 *funcs_data,
                                                 RoscoDestroyFunc      funcs_data_destroy)
{
  ...
}
