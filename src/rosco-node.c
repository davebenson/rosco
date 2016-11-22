#include "dsk/dsk.h"
#include "rosco-node.h"
#include "rosco-node-internals.h"

typedef enum {
  ROSCO_NODE_REGISTRATION_INIT,
  ROSCO_NODE_REGISTRATION_PENDING,
  ROSCO_NODE_REGISTRATION_SUCCESS,
  ROSCO_NODE_REGISTRATION_FAILED_WAITING_TO_RETRY
} RoscoNodeRegistrationState;

typedef struct RoscoNodeClass RoscoNodeClass;
struct RoscoNodeClass
{
  DskObjectClass base_class;
};
struct RoscoNode
{
  DskObject base_instance;
  DskUrl *master_url;

  char *name;
  const RoscoNodeFuncs *funcs;
  void *funcs_data;
  DskDestroyNotify funcs_data_destroy;

  RoscoNodeRegistrationState registration_state;
  DskDispatchTimer *registration_retry_timer;
};

static void
rosco_node_init (RoscoNode *nodoe)
{
}

static void
rosco_node_finalize (RoscoNode *node)
{
}

DSK_OBJECT_CLASS_DEFINE_CACHE_DATA(RoscoNode);
RoscoNodeClass rosco_node_class = {
  DSK_OBJECT_CLASS_DEFINE(RoscoNode, &dsk_object_class, rosco_node_init, rosco_node_finalize),
};

static DskXmlParserConfig *parser_config = NULL;

typedef struct RoscoXmlrpcHttpClientUserData RoscoXmlrpcHttpClientUserData;
struct RoscoXmlrpcHttpClientUserData {
  RoscoNodeExecuteCallback   callback;
  void                      *callback_data;
  dsk_boolean                invoked;
};
static RoscoXmlrpcHttpClientUserData *
rosco_xmlrpc_http_client_user_data_new (RoscoNodeExecuteCallback callback,
                                        void *callback_data)
{
  RoscoXmlrpcHttpClientUserData *rv = DSK_NEW (RoscoXmlrpcHttpClientUserData);
  rv->callback = callback;
  rv->callback_data = callback_data;
  rv->invoked = DSK_FALSE;
  return rv;
}

static inline void
rosco_xmlrpc_http_client_user_data_invoke_fail (RoscoXmlrpcHttpClientUserData *ud, DskError *error)
{
  dsk_assert (!ud->invoked);
  RoscoNodeExecuteResult result;
  result.result = ROSCO_NODE_EXECUTE_RESULT_FAILED;
  result.info.failed.error = error;
  ud->invoked = DSK_TRUE;
  ud->callback (&result, ud->callback_data);
}

static inline void
rosco_xmlrpc_http_client_user_data_invoke_fault (RoscoXmlrpcHttpClientUserData *ud,
                                                 const char *message,
                                                 uint32_t code,
                                                 DskXmlrpcValue *error_value)
{
  dsk_assert (!ud->invoked);
  RoscoNodeExecuteResult result;
  result.result = ROSCO_NODE_EXECUTE_RESULT_FAULTED;
  result.info.faulted.message = (char *) message;
  result.info.faulted.code = code;
  result.info.faulted.error_value = error_value;
  ud->invoked = DSK_TRUE;
  ud->callback (&result, ud->callback_data);
}

static inline void
rosco_xmlrpc_http_client_user_data_invoke_success (RoscoXmlrpcHttpClientUserData *ud,
                                                 DskXml *response,
                                                 size_t n_params,
                                                 DskXmlrpcValue **params)
{
  dsk_assert (!ud->invoked);
  RoscoNodeExecuteResult result;
  result.result = ROSCO_NODE_EXECUTE_RESULT_SUCCESS;
  result.info.success.response = response;
  result.info.success.n_params = n_params;
  result.info.success.params = params;
  ud->invoked = DSK_TRUE;
  ud->callback (&result, ud->callback_data);
}

static void
rosco_xmlrpc_http_client__handle_response         (DskHttpClientStreamTransfer *transfer)
{
}

static void 
rosco_xmlrpc_http_client__handle_content_complete (DskHttpClientStreamTransfer *transfer)
{
  RoscoXmlrpcHttpClientUserData *user_data = transfer->user_data;
  DskBuffer *content = &transfer->content->buffer;
  DskXmlParser *parser = dsk_xml_parser_new (parser_config, "XMLRPC URL");
  DskError *error = NULL;
  while (content->size > 0)
    {
      DskBufferFragment *f = content->first_frag;
      if (!dsk_xml_parser_feed (parser, f->buf_length, f->buf + f->buf_start, &error))
        {
	  rosco_xmlrpc_http_client_user_data_invoke_fail (user_data, error);
	  dsk_error_unref (error);
	  dsk_xml_parser_free (parser);
	  return;
        }
      dsk_buffer_discard (content, f->buf_length);
    }
  DskXml *body = dsk_xml_parser_pop (parser, NULL);
  if (body == NULL)
    {
      rosco_xmlrpc_http_client_user_data_invoke_fail (user_data, error);
      dsk_error_unref (error);
      dsk_xml_parser_free (parser);
      return;
    }
  dsk_xml_parser_free (parser);
  DskXmlrpcMethodResponseInfo method_response_info;
  if (!dsk_xmlrpc_parse_method_response (body, &method_response_info, &error))
    {
      rosco_xmlrpc_http_client_user_data_invoke_fail (user_data, error);
      dsk_error_unref (error);
      dsk_xml_unref (body);
      return;
    }
  if (method_response_info.faulted)
    {
      rosco_xmlrpc_http_client_user_data_invoke_fault (user_data,
                                                       method_response_info.fault_message,
                                                       method_response_info.fault_code,
                                                       NULL   /* TODO */
                                                      );
    }
  else
    {
      rosco_xmlrpc_http_client_user_data_invoke_success (user_data,
                                                         NULL,	/* TODO */
                                                         method_response_info.n_params,
                                                         method_response_info.params);
    }
  dsk_xmlrpc_method_response_info_clear (&method_response_info);
  dsk_xml_unref (body);
}

static void 
rosco_xmlrpc_http_client__handle_error            (DskHttpClientStreamTransfer *transfer)
{
  RoscoXmlrpcHttpClientUserData *user_data = transfer->user_data;
  if (!user_data->invoked)
    rosco_xmlrpc_http_client_user_data_invoke_fail (user_data, transfer->owner->latest_error);
}

static void 
rosco_xmlrpc_http_client__destroy                 (DskHttpClientStreamTransfer *transfer)
{
  RoscoXmlrpcHttpClientUserData *user_data = transfer->user_data;
  dsk_assert (user_data->invoked);
  dsk_free (user_data);
}

static DskHttpClientStreamFuncs rosco_node_http_client_request_funcs = {
  rosco_xmlrpc_http_client__handle_response,
  rosco_xmlrpc_http_client__handle_content_complete,
  rosco_xmlrpc_http_client__handle_error,
  rosco_xmlrpc_http_client__destroy
};

void rosco_node_execute_on_master (RoscoNode                 *node,
                                   const char                *method,
                                   unsigned                   n_params,
                                   const DskXmlrpcValue     **params,
                                   RoscoNodeExecuteCallback   callback,
                                   void                      *callback_data)
{
  DskClientStreamOptions cs_options = DSK_CLIENT_STREAM_OPTIONS_INIT;
  DskOctetSink *sink;
  DskOctetSource *source;
  DskError *error = NULL;
  cs_options.hostname = node->master_url->host;
  cs_options.port = node->master_url->port;
  //TODO: set Connection: close
  if (!dsk_client_stream_new (&cs_options, NULL, &sink, &source, &error))
    {
      RoscoNodeExecuteResult result;
      result.result = ROSCO_NODE_EXECUTE_RESULT_FAILED;
      result.info.failed.error = error;
      callback (&result, callback_data);
      dsk_error_unref (error);
      return;
    }
  DskHttpClientStreamOptions hcs_options = DSK_HTTP_CLIENT_STREAM_OPTIONS_INIT;
  hcs_options.max_pipelined_requests = 0;
  DskHttpClientStream *cs = dsk_http_client_stream_new (sink, source, &hcs_options);
  DskHttpRequestOptions req_header_options = DSK_HTTP_REQUEST_OPTIONS_INIT;
  DskHttpClientStreamRequestOptions request_options = DSK_HTTP_CLIENT_STREAM_REQUEST_OPTIONS_INIT;
  request_options.request_options = &req_header_options;
  req_header_options.method = DSK_HTTP_METHOD_POST;
  DskBuffer xml_buffer = DSK_BUFFER_INIT;
  DskXml *xml = dsk_xmlrpc_make_method_request (method, n_params, params);
  dsk_xml_to_buffer (xml, &xml_buffer);
  request_options.post_data_length = xml_buffer.size;
  char *xml_string = dsk_buffer_empty_to_string (&xml_buffer);
  request_options.post_data_slab = xml_string;
  request_options.content_type = "text/xml";
  request_options.funcs = &rosco_node_http_client_request_funcs;
  RoscoXmlrpcHttpClientUserData *user_data = rosco_xmlrpc_http_client_user_data_new (callback, callback_data);
  request_options.user_data = user_data;
  DskHttpClientStreamTransfer *xfer = dsk_http_client_stream_request (cs, &request_options, &error);
  dsk_free (xml_string);
  dsk_xml_unref (xml);
  dsk_http_client_stream_shutdown (cs);
  if (xfer == NULL)
    {
      rosco_xmlrpc_http_client_user_data_invoke_fail (user_data, error);
      dsk_free (request_options.user_data);
      dsk_error_unref (error);
      error = NULL;
    }

  dsk_object_unref (sink);
  dsk_object_unref (source);
  dsk_object_unref (cs);
}

RoscoNode          *
rosco_node_new              (DskUrl               *master_url,
                             const char           *name,
                             const RoscoNodeFuncs *funcs, 
                             void                 *funcs_data,
                             DskDestroyNotify      funcs_data_destroy)
{
  if (parser_config == NULL)
    {
      // Perform one-time initialization.
      parser_config = dsk_xml_parser_config_new_simple (0, NULL);
    }
  RoscoNode *rv = dsk_object_new (&rosco_node_class);
  rv->master_url = dsk_object_ref (master_url);
  rv->name = dsk_strdup (name);
  rv->funcs = funcs;
  rv->funcs_data = funcs_data;
  rv->funcs_data_destroy = funcs_data_destroy;

  // register node
  rv->registration_state = ROSCO_NODE_REGISTRATION_PENDING;
  rosco_node_execute_on_master (rv, method_name, nparams, params,...);


  return rv;
}
