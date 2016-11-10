#include "dsk/dsk.h"

typedef void (*XmlrpcHandler) (DskError *error,
                               DskXml *response,
                               void *handler_data);

static void
do_xmlrpc (DskUrl *url,
           const char *method,
           DskXml *input,
           XmlrpcHandler handler,
           void *handler_data)
{
  DskClientStreamOptions client_stream_options = DSK_CLIENT_STREAM_OPTIONS_INIT;
  DskOctetSink *sink;
  DskOctetSource *source;
  DskError *error = NULL;
  if (!dsk_client_stream_new (&client_stream_options, NULL, &sink, &source, &error))
    {
      handler (error, NULL, handler_data);
      dsk_error_unref (error);
      return;
    }

  DskHttpClientStreamOptions http_client_stream_options = DSK_HTTP_CLIENT_STREAM_OPTIONS_INIT;
  DskHttpClientStream *http_client = dsk_http_client_stream_new (sink, source, &http_client_stream_options);
  DskHttpClientStreamRequestOptions req_options = DSK_HTTP_CLIENT_STREAM_REQUEST_OPTIONS_INIT;
  req_options.post_data_len = ...
  req_options.post_data_slab = ...
  req_options.
  dsk_http_client_stream_request (...);
  dsk_object_unref (sink);
  dsk_object_unref (source);
  dsk_object_unref (http_client);
}

DskHttpClientStream *
dsk_http_client_stream_new     (DskOctetSink        *sink,
                                DskOctetSource      *source,
                                const DskHttpClientStreamOptions *options);
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
