
typedef enum
{
  ROSCO_BUILTIN_TYPE_BOOL,
  ROSCO_BUILTIN_TYPE_INT8,
  ROSCO_BUILTIN_TYPE_UINT8,
  ROSCO_BUILTIN_TYPE_INT16,
  ROSCO_BUILTIN_TYPE_UINT16,
  ROSCO_BUILTIN_TYPE_INT32,
  ROSCO_BUILTIN_TYPE_UINT32,
  ROSCO_BUILTIN_TYPE_INT64,
  ROSCO_BUILTIN_TYPE_UINT64,
  ROSCO_BUILTIN_TYPE_TIME,
  ROSCO_BUILTIN_TYPE_DURATION,
  ROSCO_BUILTIN_TYPE_MESSAGE,
  ROSCO_BUILTIN_TYPE_ARRAY,
} RoscoBuiltinType;

struct RoscoType {
  RoscoBuiltinType type;
  char *cname;
  char *name;
  size_t sizeof_ctype;
  size_t alignof_ctype;

  rosco_bool (*serialize)(RoscoType *type,
                          const void *ptr_value,
                          RoscoBuffer *out,
                          RoscoError **error);
  rosco_bool (*deserialize)(RoscoType *type,
                            RoscoBufferIterator *in_out,
                            void        *ptr_value_out,
                            RoscoError **error);

  RoscoType *vararray_type;
  unsigned n_fixed_array_types;
  RoscoType **fixed_array_types;
};

struct RoscoMessageType {
  RoscoType base;
  unsigned n_fields;
  RoscoMessageTypeField *fields;
};

struct RoscoArrayType {
  RoscoType base;
  RoscoType *element_type;
  ssize_t length;                       // or -1 for variable length
};

typedef struct _RoscoMessageTypeField RoscoMessageTypeField;
struct _RoscoMessageTypeField
{
  char *name;
  RoscoType type;
  void *descriptor;
  size_t offset;
};

typedef struct _RoscoMessage RoscoMessage;
struct _RoscoMessage
{
  RoscoMessageType *message_type;
  unsigned ref_count;
};


typedef struct RoscoMessageContextType RoscoMessageContextType;
typedef struct RoscoMessageContext RoscoMessageContext;

struct RoscoMessageContextType {
  RoscoType *type;
  RoscoMessageContextType *left, *right, *parent;
  rosco_bool is_red;
};

struct _RoscoMessageContext
{
  size_t n_dirs;
  char **dirs;
  RoscoMessageContextType *types_by_name;
};

RoscoMessageContext *rosco_message_context_new     (unsigned             n_dirs,
                                                    char               **dirs);
RoscoType           *rosco_message_context_get     (RoscoMessageContext *message,
                                                    const char          *name,
                                                    RoscoError         **error);
void                 rosco_message_context_destroy (RoscoMessageContext *context);

