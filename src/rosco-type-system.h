#ifndef __ROSCO_TYPE_SYSTEM_H_
#define __ROSCO_TYPE_SYSTEM_H_

typedef struct RoscoType RoscoType;
typedef struct RoscoMessageType RoscoMessageType;
typedef struct RoscoMessage RoscoMessage;
typedef struct RoscoArrayType RoscoArrayType;
typedef struct RoscoArray RoscoArray;
typedef struct RoscoMessageTypeField RoscoMessageTypeField;
typedef struct RoscoServiceType RoscoServiceType;

#include "dsk/dsk.h"
#include <unistd.h>

typedef uint8_t rosco_bool;

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
  char *cname;
  char *name;
  size_t sizeof_ctype;
  size_t alignof_ctype;

  dsk_boolean (*serialize)(RoscoType *type,
                          const void *ptr_value,
                          DskBuffer *out,
                          DskError **error);
  dsk_boolean (*deserialize)(RoscoType *type,
                            DskBuffer *in,
                            void        *ptr_value_out,
                            DskError **error);

  // derived types
  RoscoArrayType *vararray_type;
  unsigned n_fixed_array_types;
  RoscoArrayType **fixed_array_types;
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

struct RoscoArray
{
  size_t count;
  void *data;
};

struct RoscoMessageTypeField
{
  char *name;
  RoscoType type;
  void *descriptor;
  size_t offset;
};

struct RoscoMessage
{
  RoscoMessageType *message_type;
  unsigned ref_count;
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
  char *name;
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
};

RoscoTypeContext    *rosco_type_context_new     (unsigned             n_message_dirs,
                                                 char               **message_dirs);
RoscoType           *rosco_type_context_get     (RoscoTypeContext    *context,
                                                 const char          *name,
                                                 DskError           **error);
RoscoServiceType    *rosco_type_context_get_service(RoscoTypeContext    *context,
                                                 const char          *name,
                                                 DskError           **error);
void                 rosco_type_context_destroy (RoscoTypeContext    *context);


#endif
