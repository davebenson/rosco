

typedef enum {
  DSK_XMLRPC_INT32,
  DSK_XMLRPC_BOOLEAN
  DSK_XMLRPC_STRING,
  DSK_XMLRPC_DOUBLE,
  DSK_XMLRPC_TIMESTAMP,
  DSK_XMLRPC_BINARY,
  DSK_XMLRPC_STRUCT,
  DSK_XMLRPC_ARRAY
} DskXmlrpcValueType;

typedef struct DskXmlrpcValue DskXmlrpcValue;

struct DskXmlrpcValue {
  DskXmlrpcValueType type;
  union {
    int32_t v_int32;
    dsk_boolean v_boolean;
    char *v_string;
    double v_double;
    struct { uint32_t nanoseconds; uint64_t seconds; } v_timestamp;
    struct {
      size_t n_members;
      DskXmlrpcMember *members;
    } v_struct;
    struct {
      size_t n_values;
      DskXmlrpcValue **values;
    } v_array;
  };
};

DskXml *dsk_xmlrpc_make_method_request (const char *method_name,
                                        size_t      n_params,
                                        const DskXmlrpcValue **params);
DskXml *dsk_xmlrpc_make_method_response (size_t      n_params,
                                        const DskXmlrpcValue **params);
DskXml *dsk_xmlrpc_make_fault_response  (int32_t     fault_code,
                                         const char *fault_string);
