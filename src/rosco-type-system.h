#ifndef __ROSCO_TYPE_SYSTEM_H_
#define __ROSCO_TYPE_SYSTEM_H_

typedef struct RoscoType RoscoType;
typedef struct RoscoMessageType RoscoMessageType;
typedef struct RoscoMessage RoscoMessage;
typedef struct RoscoArrayType RoscoArrayType;
typedef struct RoscoArray RoscoArray;
typedef struct RoscoMessageTypeField RoscoMessageTypeField;
typedef struct RoscoServiceType RoscoServiceType;

// private
typedef struct RoscoTypeContextRecGuard RoscoTypeContextRecGuard;

#include "dsk/dsk.h"
#include <unistd.h>


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
  ROSCO_BUILTIN_TYPE_FLOAT32,
  ROSCO_BUILTIN_TYPE_FLOAT64,
  ROSCO_BUILTIN_TYPE_STRING,
  ROSCO_BUILTIN_TYPE_TIME,
  ROSCO_BUILTIN_TYPE_DURATION,
  ROSCO_BUILTIN_TYPE_MESSAGE,
  ROSCO_BUILTIN_TYPE_ARRAY,
} RoscoBuiltinType;

typedef dsk_boolean (*RoscoTypeSerializeFunc)   (RoscoType         *type,
                                                 const void        *ptr_value,
                                                 DskBuffer         *out,
                                                 DskError         **error);
typedef dsk_boolean (*RoscoTypeDeserializeFunc) (RoscoType         *type,
                                                 DskBuffer         *in_out,
                                                 void              *ptr_value_out,
                                                 DskError         **error);

struct RoscoType {
  RoscoBuiltinType type;
  unsigned is_static : 1;
  unsigned pass_by_ref : 1;
  char *cname;
  char *name;
  char *base_name;
  char *func_prefix_name;
  size_t sizeof_ctype;
  size_t alignof_ctype;

  // this is either "cname"         if !pass_by_ref
  // or             "const cname *" if pass_by_ref
  char *c_input_arg_type;

  dsk_boolean (*serialize)(RoscoType *type,
                          const void *ptr_value,
                          DskBuffer *out,
                          DskError **error);
  dsk_boolean (*deserialize)(RoscoType *type,
                            DskBuffer *in,
                            void        *ptr_value_out,
                            DskError **error);
  void (*destruct) (RoscoType *type,
                    void      *ptr_value);

  // derived types
  RoscoArrayType *vararray_type;
  unsigned n_fixed_array_types;
  RoscoArrayType **fixed_array_types;
};

struct RoscoMessageType {
  RoscoType base;
  unsigned n_fields;
  RoscoMessageTypeField *fields;
  size_t sizeof_message;
};

struct RoscoArrayType {
  RoscoType base;
  RoscoType *element_type;
  ssize_t length;                       // or -1 for variable length
};

struct RoscoArray
{
  size_t count;
  void *data;
};

struct RoscoMessageTypeField
{
  char *name;
  RoscoType *type;
  size_t offset;
};

struct RoscoMessage
{
  RoscoMessageType *message_type;
};


typedef struct RoscoTypeContextType RoscoTypeContextType;
typedef struct RoscoTypeContextServiceType RoscoTypeContextServiceType;
typedef struct RoscoTypeContext RoscoTypeContext;

struct RoscoTypeContextType {
  RoscoType *type;
  RoscoTypeContextType *left, *right, *parent;
  dsk_boolean is_red;
};

struct RoscoServiceType
{
  dsk_boolean is_static;
  char *name;
  char *cname;
  RoscoMessageType *input;
  RoscoMessageType *output;
};

struct RoscoTypeContextServiceType {
  RoscoServiceType *service_type;
  RoscoTypeContextServiceType *left, *right, *parent;
  dsk_boolean is_red;
};

struct RoscoTypeContext
{
  size_t n_dirs;
  char **dirs;
  RoscoTypeContextType *types_by_name;
  RoscoTypeContextServiceType *services_by_name;
  RoscoTypeContextRecGuard *recursion_guards;
};

RoscoTypeContext    *rosco_type_context_new     (unsigned             n_message_dirs,
                                                 char               **message_dirs);
RoscoType           *rosco_type_context_get     (RoscoTypeContext    *context,
                                                 const char          *name,
                                                 ssize_t              opt_name_len,
                                                 DskError           **error);
RoscoServiceType    *rosco_type_context_get_service(RoscoTypeContext    *context,
                                                 const char          *name,
                                                 ssize_t              opt_name_len,
                                                 DskError           **error);
void                 rosco_type_context_destroy (RoscoTypeContext    *context);

RoscoType *rosco_bool__get_type(void);
RoscoType *rosco_uint8__get_type(void);
RoscoType *rosco_int8__get_type(void);
RoscoType *rosco_uint16__get_type(void);
RoscoType *rosco_int16__get_type(void);
RoscoType *rosco_uint32__get_type(void);
RoscoType *rosco_int32__get_type(void);
RoscoType *rosco_uint64__get_type(void);
RoscoType *rosco_int64__get_type(void);
RoscoType *rosco_float32__get_type(void);
RoscoType *rosco_float64__get_type(void);
RoscoType *rosco_string__get_type(void);
RoscoType *rosco_time__get_type(void);
RoscoType *rosco_duration__get_type(void);

#endif
