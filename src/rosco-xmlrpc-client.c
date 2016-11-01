
typedef struct XmlrpcCall {
  Rosco_XMLRPC_Handler handler;
  Rosco_XMLRPC_ErrorHandler error_handler;
  void *data;
  Rosco_XMLRPC_Destroy destroy;
} XmlrpcCall;

static void
destroy_call (XmlrpcCall *call)
{
  if (call->destroy != NULL)
    call->destroy (call->data);
  rosco_free (call);
}

static void
handle_http_response (RoscoHttpResponse *response,
                      void *data)
{
  XmlrpcCall *call = data;
  RoscoError *error = NULL;
  RoscoMutableXML_Document *doc = rosco_mutable_xml_document_parse (response->body_length, response->body_data, &error);
  if (doc == NULL)
    {
      if (call->error_handler)
        call->error_handler (error, call->data;
      destroy_call (call);
      return;
    }
  call->handler (doc, call->data);
  rosco_xmlrpc_document_free (doc);
  destroy_call (call);
}

void rosco_xmlrpc_start (RoscoURL    *url,
                         RoscoMutableXML *xml,  // postdata
                         Rosco_XMLRPC_Handler handler,
                         Rosco_XMLRPC_ErrorHandler error_handler,
                         void *data,
                         Rosco_XMLRPC_Destroy destroy)
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
