#include "dsk/dsk.h"
#include "rosco-node.h"

typedef void (*RoscoNodeExecuteCallback) (DskError *error,
                               DskXml *response,
                               void *handler_data);

struct RoscoNode
{
  DskURL *master_url;
};

void rosco_node_execute_on_master (RoscoNode                 *node,
                                   const char                *method,
                                   unsigned                   n_params,
                                   DskXmlrpcValue           **params,
                                   RoscoNodeExecuteCallback   callback,
                                   void                      *callback_data)
{
  DskClientStreamOptions cs_options = DSK_CLIENT_STREAM_OPTIONS_INIT;
  DskOctetSink *sink;
  DskOctetSource *source;
  cs_options.hostname = node->master_url->host;
  cs_options.port = node->master_url->port;
  //TODO: set Connection: close
  if (!dsk_client_stream_new (&cs_options, NULL, &sink, &source, &error))
    {
      RoscoNodeExecuteResult result;
      result.type = ROSCO_NODE_EXECUTE_RESULT_FAILED;
      result.info.failed.error = error;
      callback (&result, callback_data);
      dsk_error_unref (error);
      return;
    }
  DskHttpClientStreamOptions hcs_options = DSK_HTTP_CLIENT_STREAM_OPTIONS_INIT;
  hcs_options.max_pipelined_requests = 0;
  DskHttpClientStream *cs = dsk_http_client_stream_new (source, sink, &hcs_options);
  DskHttpClientStreamRequestOptions request_options = DSK_HTTP_CLIENT_STREAM_REQUEST_OPTIONS_INIT;
  DskBuffer xml_buffer = DSK_BUFFER_INIT;
  DskXml *xml = dsk_xmlrpc_make_method_request (method_name, n_params, params);
  dsk_xml_to_buffer (xml, &xml_buffer);
  request_options.post_data_length = xml_buffer.size;
  char *xml_string = dsk_buffer_empty_to_string (&xml_buffer);
  request_options.post_data_slab = xml_string;
  request_options.content_type = "text/xml";
  request_options.funcs = &rosco_node_http_client_request_funcs;
  HttpClientUserData *user_data = rosco_xmlrpc_http_client_user_data_new (callback, callback_data);
  request_options.user_data = user_data;
  DskHttpClientStreamTransfer *xfer = dsk_http_client_stream_request (cs, &request_options, &error);
  dsk_free (xml_string);
  dsk_xml_unref (xml);
  dsk_http_client_stream_shutdown (cs);
  if (xfer == NULL)
    {
      RoscoNodeExecuteResult result;
      result.type = ROSCO_NODE_EXECUTE_RESULT_FAILED;
      result.info.failed.error = error;
      callback (&result, callback_data);
      dsk_free (request_options.user_data);
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

RoscoNode          *rosco_node_new              (RoscoURL             *master_url,
                                                 const RoscoNodeFuncs *funcs, 
                                                 void                 *funcs_data,
                                                 RoscoDestroyFunc      funcs_data_destroy)
{
  ...
}
