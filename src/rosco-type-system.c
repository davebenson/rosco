#include "rosco.h"
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dsk/dsk-rbtree-macros.h"
#include "dsk/dsk-tmp-array-macros.h"

#define MAX_ALIGN DSK_MAX3(alignof(double), alignof(uint64_t), alignof(void*))

struct RoscoTypeContextRecGuard {
  RoscoTypeContextRecGuard *left,*right,*parent;
  dsk_boolean is_red;
  const char *str;
};
#define COMPARE_REC_GUARDS(a,b, rv) \
  rv = strcmp(a->str, b->str)
#define GET_RECURSION_GUARD_TREE(context) \
  (context)->recursion_guards, RoscoTypeContextRecGuard*, DSK_STD_GET_IS_RED, DSK_STD_SET_IS_RED, parent, left, right, COMPARE_REC_GUARDS
#define COMPARE_STR_AND_REC_GUARD_NODE(teststr, node, rv) \
  rv = strcmp (teststr, node->str)

#define UINT32_TO_UINT8LE_ARRAY(value) \
   (uint8_t)((value)>>0),              \
   (uint8_t)((value)>>8),              \
   (uint8_t)((value)>>16),             \
   (uint8_t)((value)>>24)

#define BILLION DSK_BILLION

#define COMPARE_TYPE_CONTEXT_TYPES(a,b, rv) \
  rv = strcmp(a->type->name, b->type->name)
#define GET_TYPE_TREE(ctx) \
  (ctx)->types_by_name, RoscoTypeContextType*, DSK_STD_GET_IS_RED, DSK_STD_SET_IS_RED, parent, left, right, COMPARE_TYPE_CONTEXT_TYPES

#define COMPARE_TYPE_CONTEXT_SERVICE_TYPES(a,b,rv) \
  rv = strcmp(a->service_type->name, b->service_type->name)
#define GET_SERVICE_TREE(ctx) \
  (ctx)->services_by_name, RoscoTypeContextServiceType*, DSK_STD_GET_IS_RED, DSK_STD_SET_IS_RED, parent, left, right, COMPARE_TYPE_CONTEXT_SERVICE_TYPES

#define DEFINE_ROSCO_TYPE(BUILTIN_BASE, c_type, name, func_prefix) \
{                                                     \
  ROSCO_BUILTIN_TYPE_##BUILTIN_BASE,                  \
  DSK_TRUE,		/* is_static */               \
  DSK_FALSE,		/* pass_by_ref */             \
  #c_type,                                            \
  #name,                                              \
  #name,                                              \
  #func_prefix,                                       \
  sizeof(c_type),                                     \
  alignof(c_type),                                    \
  #c_type,                                            \
  func_prefix ## __serialize_f,                       \
  func_prefix ## __deserialize_f,                     \
  func_prefix ## __destruct_f,                        \
  NULL,                                               \
  0,                                                  \
  NULL                                                \
}
#define DESERIALIZE_CHECK_MIN_SIZE(name, buffer, min_size, error)   \
  do{                                                               \
    if ((buffer)->size < (min_size))                                \
      {                                                             \
        dsk_set_error (error, "%s deserialize: end-of-data", name); \
        return DSK_FALSE;                                           \
      }                                                             \
  }while(0)

#define DECLARE_SERIALIZE_FUNC(func_name)      \
static dsk_boolean                             \
func_name (RoscoType         *type,            \
           const void        *ptr_value,       \
           DskBuffer         *out,             \
           DskError         **error)
#define DECLARE_DESERIALIZE_FUNC(func_name)    \
static dsk_boolean                             \
func_name (RoscoType         *type,            \
           DskBuffer         *in_out,          \
           void              *ptr_value_out,   \
           DskError         **error)
#define DECLARE_DESTRUCT_FUNC(func_name)       \
static void                                    \
func_name (RoscoType         *type,            \
           void              *ptr_value)

DECLARE_SERIALIZE_FUNC(rosco_bool__serialize_f)
{ 
  (void)type;
  return rosco_bool__serialize(* (rosco_bool *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_bool__deserialize_f)
{
  (void)type;
  return rosco_bool__deserialize(in_out, (rosco_bool *) ptr_value_out, error);
}

#define rosco_bool__destruct_f NULL

static RoscoType rosco_bool__type = DEFINE_ROSCO_TYPE(BOOL, rosco_bool, bool, rosco_bool);

RoscoType *rosco_bool__get_type()
{
  return &rosco_bool__type;
}

DECLARE_SERIALIZE_FUNC(rosco_uint8__serialize_f)
{
  (void) type;
  return rosco_uint8__serialize(* (uint8_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_uint8__deserialize_f)
{
  (void) type;
  return rosco_uint8__deserialize (in_out, ptr_value_out, error);
}
#define rosco_uint8__destruct_f NULL
static RoscoType rosco_uint8__type = DEFINE_ROSCO_TYPE(UINT8, uint8_t, uint8, rosco_uint8);
RoscoType *rosco_uint8__get_type()
{
  return &rosco_uint8__type;
}

DECLARE_SERIALIZE_FUNC(rosco_int8__serialize_f)
{
  (void) type;
  return rosco_int8__serialize(* (int8_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_int8__deserialize_f)
{
  (void) type;
  return rosco_int8__deserialize (in_out, ptr_value_out, error);
}
#define rosco_int8__destruct_f NULL
static RoscoType rosco_int8__type = DEFINE_ROSCO_TYPE(INT8, int8_t, int8, rosco_int8);
RoscoType *rosco_int8__get_type()
{
  return &rosco_int8__type;
}

DECLARE_SERIALIZE_FUNC(rosco_uint16__serialize_f)
{
  (void) type;
  return rosco_uint16__serialize(* (uint16_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_uint16__deserialize_f)
{
  (void) type;
  return rosco_uint16__deserialize (in_out, ptr_value_out, error);
}
#define rosco_uint16__destruct_f NULL
static RoscoType rosco_uint16__type = DEFINE_ROSCO_TYPE(UINT16, uint16_t, uint16, rosco_uint16);
RoscoType *rosco_uint16__get_type()
{
  return &rosco_uint16__type;
}

DECLARE_SERIALIZE_FUNC(rosco_int16__serialize_f)
{
  (void) type;
  return rosco_int16__serialize(* (int16_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_int16__deserialize_f)
{
  (void) type;
  return rosco_int16__deserialize (in_out, ptr_value_out, error);
}
#define rosco_int16__destruct_f NULL
static RoscoType rosco_int16__type = DEFINE_ROSCO_TYPE(INT16, int16_t, int16, rosco_int16);
RoscoType *rosco_int16__get_type()
{
  return &rosco_int16__type;
}

DECLARE_SERIALIZE_FUNC(rosco_uint32__serialize_f)
{
  (void) type;
  return rosco_uint32__serialize(* (uint32_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_uint32__deserialize_f)
{
  (void) type;
  return rosco_uint32__deserialize (in_out, ptr_value_out, error);
}
#define rosco_uint32__destruct_f NULL
static RoscoType rosco_uint32__type = DEFINE_ROSCO_TYPE(UINT32, uint32_t, uint32, rosco_uint32);
RoscoType *rosco_uint32__get_type()
{
  return &rosco_uint32__type;
}

DECLARE_SERIALIZE_FUNC(rosco_int32__serialize_f)
{
  (void) type;
  return rosco_int32__serialize(* (int32_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_int32__deserialize_f)
{
  (void) type;
  return rosco_int32__deserialize (in_out, ptr_value_out, error);
}
#define rosco_int32__destruct_f NULL
static RoscoType rosco_int32__type = DEFINE_ROSCO_TYPE(INT32, int32_t, int32, rosco_int32);
RoscoType *rosco_int32__get_type()
{
  return &rosco_int32__type;
}

DECLARE_SERIALIZE_FUNC(rosco_uint64__serialize_f)
{
  (void) type;
  return rosco_uint64__serialize(* (uint64_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_uint64__deserialize_f)
{
  (void) type;
  return rosco_uint64__deserialize (in_out, ptr_value_out, error);
}
#define rosco_uint64__destruct_f NULL
static RoscoType rosco_uint64__type = DEFINE_ROSCO_TYPE(UINT64, uint64_t, uint64, rosco_uint64);
RoscoType *rosco_uint64__get_type()
{
  return &rosco_uint64__type;
}

DECLARE_SERIALIZE_FUNC(rosco_int64__serialize_f)
{
  (void) type;
  return rosco_int64__serialize(* (int64_t *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_int64__deserialize_f)
{
  (void) type;
  return rosco_int64__deserialize (in_out, ptr_value_out, error);
}
#define rosco_int64__destruct_f NULL
static RoscoType rosco_int64__type = DEFINE_ROSCO_TYPE(INT64, int64_t, int64, rosco_int64);
RoscoType *rosco_int64__get_type()
{
  return &rosco_int64__type;
}

DECLARE_SERIALIZE_FUNC(rosco_float32__serialize_f)
{
  (void) type;
  return rosco_float32__serialize(* (float *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_float32__deserialize_f)
{
  (void) type;
  return rosco_float32__deserialize (in_out, ptr_value_out, error);
}
#define rosco_float32__destruct_f NULL
static RoscoType rosco_float32__type = DEFINE_ROSCO_TYPE(FLOAT32, float, float32, rosco_float32);
RoscoType *rosco_float32__get_type()
{
  return &rosco_float32__type;
}

DECLARE_SERIALIZE_FUNC(rosco_float64__serialize_f)
{
  (void) type;
  return rosco_float64__serialize(* (double *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_float64__deserialize_f)
{
  (void) type;
  return rosco_float64__deserialize (in_out, ptr_value_out, error);
}
#define rosco_float64__destruct_f NULL
static RoscoType rosco_float64__type = DEFINE_ROSCO_TYPE(FLOAT64, float, float64, rosco_float64);
RoscoType *rosco_float64__get_type()
{
  return &rosco_float64__type;
}


DECLARE_SERIALIZE_FUNC(rosco_duration__serialize_f)
{
  (void) type;
  return rosco_duration__serialize (* (RoscoDuration *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_duration__deserialize_f)
{
  (void) type;
  return rosco_duration__deserialize (in_out, ptr_value_out, error);
}
#define rosco_duration__destruct_f NULL
static RoscoType rosco_duration__type = DEFINE_ROSCO_TYPE(DURATION, RoscoDuration, duration, rosco_duration);
RoscoType *rosco_duration__get_type()
{
  return &rosco_duration__type;
}

DECLARE_SERIALIZE_FUNC(rosco_time__serialize_f)
{
  (void) type;
  return rosco_time__serialize (* (RoscoTime *) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_time__deserialize_f)
{
  (void) type;
  return rosco_time__deserialize (in_out, ptr_value_out, error);
}
#define rosco_time__destruct_f NULL
static RoscoType rosco_time__type = DEFINE_ROSCO_TYPE(TIME, RoscoTime, time, rosco_time);
RoscoType *rosco_time__get_type()
{
  return &rosco_time__type;
}
DECLARE_SERIALIZE_FUNC(rosco_string__serialize_f)
{
  (void) type;
  return rosco_string__serialize (* (char **) ptr_value, out, error);
}
DECLARE_DESERIALIZE_FUNC(rosco_string__deserialize_f)
{
  (void) type;
  return rosco_string__deserialize (in_out, ptr_value_out, error);
}
DECLARE_DESTRUCT_FUNC(rosco_string__destruct_f)
{
  (void) type;
  return rosco_string__destruct (* (char **) ptr_value);
}
static RoscoType rosco_string__type = DEFINE_ROSCO_TYPE(STRING, char *, string, rosco_string);
RoscoType *rosco_string__get_type()
{
  return &rosco_string__type;
}


static RoscoTypeContextType *
type_context_register_type (RoscoTypeContext *ctx, RoscoType *type)
{
  RoscoTypeContextType *node = DSK_NEW (RoscoTypeContextType);
  node->type = type;
  RoscoTypeContextType *conflict;
  DSK_RBTREE_INSERT (GET_TYPE_TREE (ctx), node, conflict);
  assert(conflict == NULL);
  return node;
}

RoscoTypeContext *
rosco_type_context_new     (unsigned             n_dirs,
                               char               **dirs)
{
  RoscoTypeContext *rv = DSK_NEW (RoscoTypeContext);
  rv->n_dirs = n_dirs;
  rv->dirs = DSK_NEW_ARRAY (n_dirs, char *);
  for (unsigned i = 0; i < n_dirs; i++)
    rv->dirs[i] = dsk_strdup (dirs[i]);
  rv->types_by_name = NULL;
  rv->services_by_name = NULL;

  // register base types
  type_context_register_type (rv, rosco_bool__get_type());
  type_context_register_type (rv, rosco_uint8__get_type());
  type_context_register_type (rv, rosco_int8__get_type());
  type_context_register_type (rv, rosco_uint16__get_type());
  type_context_register_type (rv, rosco_int16__get_type());
  type_context_register_type (rv, rosco_uint32__get_type());
  type_context_register_type (rv, rosco_int32__get_type());
  type_context_register_type (rv, rosco_uint64__get_type());
  type_context_register_type (rv, rosco_int64__get_type());
  type_context_register_type (rv, rosco_float32__get_type());
  type_context_register_type (rv, rosco_float64__get_type());
  type_context_register_type (rv, rosco_string__get_type());
  type_context_register_type (rv, rosco_time__get_type());
  type_context_register_type (rv, rosco_duration__get_type());

  return rv;
}

static dsk_boolean
array_serialize                (RoscoType *type,
		                const void *ptr_value,
		                DskBuffer *out,
		                DskError **error)
{
  dsk_assert (type->type == ROSCO_BUILTIN_TYPE_ARRAY);
  RoscoArrayType *arrtype = (RoscoArrayType *) type;
  RoscoArray arr = * (RoscoArray *) ptr_value;
  RoscoType *elt_type = arrtype->element_type;
  size_t elt_size = elt_type->sizeof_ctype;
  if (arrtype->length >= 0)
    {
      // fixed-length array
      dsk_assert (arr.count == (size_t) arrtype->length);
      // note: there's no header to serialize
    }
  else
    {
      // variable-length array. serialize the length first.
      unsigned length = arr.count;
      uint8_t prefix[4] = { UINT32_TO_UINT8LE_ARRAY(length) };
      dsk_buffer_append (out, 4, prefix);
    }
  
  // serialize elements
  void *at = arr.data;
  unsigned rem = arr.count;
  while (rem-- > 0)
    { 
      if (!elt_type->serialize (elt_type, at, out, error))
	return DSK_FALSE;
      at = (char *) at + elt_size;
    }
  return DSK_TRUE;
}

static dsk_boolean
array_deserialize (RoscoType   *type,
		   DskBuffer   *in,
		   void        *ptr_value_out,
		   DskError   **error)
{
  dsk_assert (type->type == ROSCO_BUILTIN_TYPE_ARRAY);
  RoscoArrayType *arrtype = (RoscoArrayType *) type;
  RoscoArray *arr = ptr_value_out;
  RoscoType *elt_type = arrtype->element_type;
  size_t elt_size = elt_type->sizeof_ctype;
  size_t n;

  // handle fixed-/variable-length arrays.
  if (arrtype->length >= 0)
    {
      n = arr->count = arrtype->length;
    }
  else
    {
      // variable-length array. serialize the length first.
      uint8_t len_enc[4];
      if (dsk_buffer_read (in, 4, len_enc) != 4)
        {
          dsk_set_error (error, "%s deserialize: end-of-data", type->name);
          return DSK_FALSE;
        }
      n = ((uint32_t)len_enc[0])
        | ((uint32_t)len_enc[1] << 8)
        | ((uint32_t)len_enc[2] << 16)
        | ((uint32_t)len_enc[3] << 24);
    }

  // deserialize the actual elements
  uint8_t *at = arr->data;
  for (unsigned i = 0; i < n; i++)
    {
      if (!elt_type->deserialize (elt_type,
				  in,
				  at,
				  error))
	{
	  return DSK_FALSE;
	}
      at += elt_size;
    }
  
  return DSK_TRUE;
}

static inline void
setup_basename (RoscoType *type)
{
  type->base_name = strchr (type->name, '/');
  if (type->base_name == NULL)
    type->base_name = type->name;
  else
    type->base_name += 1;
}

static RoscoType *
_rosco_type_get_array_type         (RoscoType *type, ssize_t length)
{
  if (length == -1)
    {
      if (type->vararray_type == NULL)
	{
	  RoscoArrayType *atype = DSK_NEW0 (RoscoArrayType);
	  atype->base.type = ROSCO_BUILTIN_TYPE_ARRAY;
	  atype->base.cname = "RoscoArray";
	  atype->base.name = dsk_strdup_printf("%s[]", type->name);
          setup_basename (&atype->base);
	  atype->base.func_prefix_name = NULL;
	  atype->base.sizeof_ctype = sizeof (RoscoArray);
	  atype->base.alignof_ctype = alignof (RoscoArray);
	  atype->base.serialize = array_serialize;
	  atype->base.deserialize = array_deserialize;
	  atype->element_type = type;
	  atype->length = -1;
	  type->vararray_type = atype;
	}
      return (RoscoType *) type->vararray_type;
    }
  else
    {
      if (length >= type->n_fixed_array_types)
        {
          type->fixed_array_types = DSK_RENEW (RoscoArrayType *, type->fixed_array_types, length + 1);
          while (type->n_fixed_array_types <= length)
            type->fixed_array_types[type->n_fixed_array_types++] = NULL;
        }
      if (type->fixed_array_types[length] == NULL)
        {
	  RoscoArrayType *atype = DSK_NEW0 (RoscoArrayType);
	  atype->base.type = ROSCO_BUILTIN_TYPE_ARRAY;
	  atype->base.cname = "RoscoArray";
	  atype->base.name = dsk_strdup_printf("%s[%u]", type->name, (unsigned) length);
          setup_basename (&atype->base);
	  atype->base.func_prefix_name = NULL;
	  atype->base.sizeof_ctype = sizeof (RoscoArray);
	  atype->base.alignof_ctype = alignof (RoscoArray);
	  atype->base.serialize = array_serialize;
	  atype->base.deserialize = array_deserialize;
	  atype->element_type = type;
	  atype->length = length;
	  type->fixed_array_types[length] = atype;
        }
      return (RoscoType *) type->fixed_array_types[length];
    }
}

static dsk_boolean
parse_message_fields_from_string (RoscoTypeContext *context, 
                                  size_t text_size, const uint8_t *text,
                                  const char *filename, unsigned start_line,
                                  size_t *n_fields_out, 
                                  RoscoMessageTypeField **fields_out,
                                  size_t *sizeof_message_out,
                                  size_t *alignof_message_out,
                                  DskError **error)
{
  const char *at = (const char *) text;
  const char *end = at + text_size;
  DSK_TMP_ARRAY_DECLARE (RoscoMessageTypeField, fields, 16);
  unsigned lineno = start_line;
  size_t cur_offset = sizeof (RoscoMessage);
  size_t cur_align = alignof (RoscoMessage);
  while (at < end)
    {
      const char *nl = memchr (at, '\n', end - at);
      const char *end_line = nl ? nl : end;
      while (at < end_line && dsk_ascii_isspace (*at))
        at++;
      if (at == end_line || *at == '#')
        {
          // may push at > end, but it doesn't matter.
          at = end_line + 1;
          lineno++;
          continue;
        }
        
      // scan the field name from the right.
      const char *end_field_name = end_line;
      while (end_field_name > at && dsk_ascii_isspace (*(end_field_name-1)))
        end_field_name--;
      const char *start_field_name = end_field_name;
      while (start_field_name > at && dsk_ascii_istoken (*(start_field_name-1)))
        start_field_name--;
      if (start_field_name == end_field_name)
        {
          dsk_set_error (error, "missing identifier %s:%u", filename, lineno);
          goto fail;
        }

      // parse fields types
      const char *end_type = start_field_name;
      while (end_type > at && dsk_ascii_isspace (*(end_type-1)))
        end_type--;
      fprintf(stderr, "calling rosco_type_context_get on %.*s\n", (int)(end_type-at),at);
      RoscoType *field_type = rosco_type_context_get (context, at, end_type - at, error);
      if (field_type == NULL)
        {
          fprintf(stderr, "FAIL!\n");
          goto fail;
        }
      
      // round cur_offset up to alignment
      cur_offset = DSK_ALIGN(cur_offset, field_type->alignof_ctype);
      cur_align = DSK_MAX(cur_align, field_type->alignof_ctype);

      RoscoMessageTypeField field;
      field.type = field_type;
      field.name = dsk_strcut (start_field_name, end_field_name);
      field.offset = cur_offset;
      DSK_TMP_ARRAY_APPEND (fields, field);

      cur_offset += field_type->sizeof_ctype;
      lineno++;

      at = end_line + 1;
    }

  // round cur_offset up to max_alignment
  cur_offset = DSK_ALIGN(cur_offset, MAX_ALIGN);

  *n_fields_out = fields.length;
  DSK_TMP_ARRAY_CLEAR_TO_ALLOCATION (fields, *fields_out);
  *sizeof_message_out = cur_offset;
  *alignof_message_out = cur_align;
  return DSK_TRUE;

fail:
  for (unsigned i = 0; i < fields.length; i++)
    dsk_free (fields.data[i].name);
  DSK_TMP_ARRAY_CLEAR (fields);
  return DSK_FALSE;
}

static dsk_boolean
message_serialize              (RoscoType *type,
		                const void *ptr_value,
		                DskBuffer *out,
		                DskError **error)
{
  dsk_assert (type->type == ROSCO_BUILTIN_TYPE_MESSAGE);
  RoscoMessageType *mtype = (RoscoMessageType *) type;
  RoscoMessage *msg = * (RoscoMessage **) ptr_value;
  if (msg == NULL)
    {
      dsk_set_error (error, "serialize message %s: message was NULL", type->name);
      return DSK_FALSE;
    }
  for (size_t i = 0; i < mtype->n_fields; i++)
    {
      RoscoType *ftype = mtype->fields[i].type;
      if (!ftype->serialize (ftype,
                             ((char *)msg) + mtype->fields[i].offset,
                             out,
                             error))
       {
         return DSK_FALSE;
       }
   }
  return DSK_TRUE;
}

static dsk_boolean
message_deserialize (RoscoType   *type,
		     DskBuffer   *in,
		     void        *ptr_value_out,
		     DskError   **error)
{
  dsk_assert (type->type == ROSCO_BUILTIN_TYPE_MESSAGE);
  RoscoMessageType *mtype = (RoscoMessageType *) type;
  RoscoMessage *msg = dsk_malloc (mtype->sizeof_message);
  msg->message_type = mtype;
  for (size_t i = 0; i < mtype->n_fields; i++)
    {
      RoscoType *ftype = mtype->fields[i].type;
      void *field_data = ((char*)msg) + mtype->fields[i].offset;
      if (!ftype->deserialize (ftype, in, field_data, error))
        {
          for (size_t j = 0; j < i; j++)
            { 
	      RoscoType *fjtype = mtype->fields[j].type;
	      void *j_field_data = ((char*)msg) + mtype->fields[j].offset;
              fjtype->destruct (fjtype, j_field_data);
            }
          dsk_free (msg);
          return DSK_FALSE;
        }
    }
  * (RoscoMessage **) ptr_value_out = msg;
  return DSK_TRUE;
}

static char *
name_to_cname (const char *name)
{
  size_t rv_len = 0;
  for (const char *at = name; *at; at++)
    if (*at == '/')
      rv_len += 2;
    else
      rv_len += 1;
  char *rv = dsk_malloc (rv_len + 1);	/* +1 for terminal NUL */
  char *rv_at = rv;
  const char *at = name;
  const char *ns_end = strrchr (name, '/');
  if (ns_end)
    ns_end++;
  else
    ns_end = name;
  while (at < ns_end)
    {
      if (*at == '/')
        {
          *rv_at++ = '_';
          *rv_at++ = '_';
        }
      else
	*rv_at++ = *at;
      at++;
    }
  strcpy (rv_at, at);
  return rv;
}

static char *
message_name_to_func_prefix (const char *name)
{
  size_t rv_len = 0;
  const char *ns_end = strrchr (name, '/');
  if (ns_end)
    ns_end++;
  else
    ns_end = name;
  for (const char *at = name; at < ns_end; at++)
    if (*at == '/')
      rv_len += 2;
    else
      rv_len += 1;
  for (const char *at = ns_end; *at; at++)
    if (dsk_ascii_isupper (*at))
      rv_len += 2;
    else 
      rv_len += 1;
  char *rv = dsk_malloc (rv_len + 1);	/* +1 for terminal NUL */
  char *rv_at = rv;
  const char *at = name;
  dsk_boolean word_start = DSK_TRUE;
  while (*at)
    {
      if (*at == '/')
        {
          *rv_at++ = '_';
          *rv_at++ = '_';
          word_start = DSK_TRUE;
        }
      else
        {
          if (dsk_ascii_isupper (*at))
            {
              if (!word_start)
                *rv_at++ = '_';
              *rv_at++ = *at + ('a' - 'A');
            }
          else
            *rv_at++ = *at;
          word_start = DSK_FALSE;
        }
      at++;
    }
  return rv;
}

static RoscoType *
_rosco_type_context_get        (RoscoTypeContext    *context,
                                const char          *normalized_name,
                                DskError            **error)
{
  char *base_type_free = NULL;
  RoscoTypeContextType *mctype;
  dsk_boolean added_rec_guard = DSK_FALSE;
  RoscoType *rv = NULL;
#define COMPARE_NORMALIZED_NAME(nname, t, rv) rv = strcmp(nname, t->type->name)
  DSK_RBTREE_LOOKUP_COMPARATOR (GET_TYPE_TREE (context), normalized_name, COMPARE_NORMALIZED_NAME, mctype);
  if (mctype != NULL)
    {
      return mctype->type;
    }
  
  const char *left_bracket = strchr (normalized_name, '[');
  const char *base_type;
  if (left_bracket == NULL)
    {
      base_type = normalized_name;
    }
  else
    {
      base_type_free = dsk_strcut (normalized_name, left_bracket);
      base_type = base_type_free;

      DSK_RBTREE_LOOKUP_COMPARATOR (GET_TYPE_TREE (context), base_type, COMPARE_NORMALIZED_NAME, mctype);
    }

  /* recursion guard */
  RoscoTypeContextRecGuard *guard;
  DSK_RBTREE_LOOKUP_COMPARATOR (GET_RECURSION_GUARD_TREE (context), base_type, COMPARE_STR_AND_REC_GUARD_NODE, guard);
  if (guard != NULL)
    {
      dsk_set_error (error, "recursion encountered parsing type %s", base_type);
      goto cleanup_and_return_rv;
    }
  RoscoTypeContextRecGuard guard_instance;
  guard = &guard_instance;
  guard->str = base_type;
  RoscoTypeContextRecGuard *rg_conflict;
  DSK_RBTREE_INSERT (GET_RECURSION_GUARD_TREE (context), guard, rg_conflict);
  dsk_assert (rg_conflict == NULL);

  /* Handle base_type */
  if (mctype == NULL)
    {
      for (unsigned i = 0; i < context->n_dirs; i++)
        {
          char *filename = dsk_strdup_printf ("%s/msg/%s.msg", context->dirs[i], base_type);
          size_t content_size;
          // read file, pass to message parser
          uint8_t *contents = dsk_file_get_contents (filename, &content_size, NULL);
          if (contents != NULL)
            {
              size_t n_fields;
              RoscoMessageTypeField *fields;
              size_t sizeof_message;
              size_t alignof_message;
              if (!parse_message_fields_from_string (context,
                                                     content_size, contents,
                                                     filename, 1,
                                                     &n_fields, &fields,
                                                     &sizeof_message,
                                                     &alignof_message,
                                                     error))
                { 
                  dsk_free (contents);
                  dsk_free (filename);
                  goto cleanup_and_return_rv;
                }

              RoscoMessageType *mt = DSK_NEW0 (RoscoMessageType);
              mt->base.type = ROSCO_BUILTIN_TYPE_MESSAGE;
              mt->base.cname = name_to_cname (normalized_name);
              mt->base.name = dsk_strdup (normalized_name);
              setup_basename (&mt->base);
              mt->base.func_prefix_name = message_name_to_func_prefix (normalized_name);
              mt->base.sizeof_ctype = sizeof_message;
              mt->base.alignof_ctype = alignof_message;
              mt->base.serialize = message_serialize;
              mt->base.deserialize = message_deserialize;
              mt->base.c_input_arg_type = dsk_strdup_printf ("const %s *", mt->base.cname);
              mt->n_fields = n_fields;
              mt->fields = fields;
              rv = (RoscoType *) mt;

              mctype = type_context_register_type (context, rv);
            }
          dsk_free (filename);
        }
      if (rv == NULL)
        {
          dsk_set_error (error, "type %s not found", base_type);
          goto cleanup_and_return_rv;
        }
    }
  dsk_assert (mctype != NULL);
  rv = mctype->type;

  // handle array subscripts
  while (left_bracket != NULL)
    {
      // find right-bracket and parse array size (if empty, make size=-1)
      const char *right_bracket = left_bracket + 1;
      ssize_t size = -1;
      if (dsk_ascii_isdigit (*right_bracket))
        {
          char *endnum;
          size = strtoul (right_bracket, &endnum, 0);
          if (right_bracket == endnum)
            {
              dsk_set_error (error, "error parsing array type: bad number '%.5s...'", right_bracket);
              goto cleanup_and_return_rv;
            }
          right_bracket = endnum;
        }
      if (*right_bracket == 0)
        {
	  dsk_set_error (error, "missing ']'");
	  goto cleanup_and_return_rv;
        }
      if (*right_bracket != ']')
	{
	  dsk_set_error (error, "unexpected character '%s' when expecting ']'", dsk_ascii_byte_name (*right_bracket));
	  goto cleanup_and_return_rv;
	}
       
      // make array type (or retrieve existing one)
      rv = _rosco_type_get_array_type (rv, size);

      // update left_bracket
      left_bracket = right_bracket + 1;
    }

cleanup_and_return_rv:
  dsk_free (base_type_free);
  if (added_rec_guard)
    DSK_RBTREE_REMOVE (GET_RECURSION_GUARD_TREE (context), guard);
  return rv;
}

// TODO: this fct should probably guard against
//       bad characters (eg "/../" in message names,
//       for security reasons.
static const char *normalize_type_name (const char *name, ssize_t len, char **free_out)
{
  if (len < 0)
    {
      if (strchr (name, ' ') == NULL)
        return name;
      len = strlen (name);
    }
  char *rv = dsk_malloc (len + 1);
  char *rv_at = rv;
  const char *name_at = name;
  unsigned rem = len;
  while (rem > 0)
    {
      if (*name_at != ' ')
        *rv_at++ = *name_at;
      name_at++;
      rem--;
    }
  *rv_at = 0;
  *free_out = rv;
  return rv;
}

RoscoType *
rosco_type_context_get     (RoscoTypeContext *context,
                            const char          *name,
                            ssize_t              opt_name_len,
                            DskError         **error)
{
  char *to_free = NULL;
  const char *nname = normalize_type_name (name, opt_name_len, &to_free);
  RoscoType *type = _rosco_type_context_get (context, nname, error);
  dsk_free (to_free);
  return type;
}

static size_t
count_newlines (size_t len, const uint8_t *data)
{
  size_t rem = len;
  size_t rv = 0;
  while (rem--)
    if (*data++ == (uint8_t) '\n')
      rv++;
  return rv;
}

static const uint8_t *
find_minusminusminus_sep (size_t len, const uint8_t *contents, const uint8_t **sep_end_out)
{
  const uint8_t *at = contents;
  const uint8_t *end = at + len;
  while (at < end - 3)
    {
      if (at == contents || *at++ == '\n')
        {
          /* skip leading whitespace;
           * if this skips a newline it doesn't matter. */
          while (at < end && dsk_ascii_isspace (*at))
            at++;
          if (at + 2 < end && at[0] == '-' && at[1] == '-' && at[2] == '-')
            {
              const uint8_t *sep_start = at;
              at += 3;
	      while (at < end && dsk_ascii_isspace (*at))
                {
		  if (*at == '\n')
		    break;
		  at++;
                }
              if (at == end)
                {
                  // we will treat a file ending with "--" (no newline)
                  // as though it had a newline, ir an empty (0-field) response.
                  *sep_end_out = end;
                  return sep_start;
                }
              if (*at == '\n') 
                {
                  *sep_end_out = at + 1;
                  return sep_start;
                }
              else
                {
                  dsk_warning ("in service file: unexpected character after --");
                  return NULL;
                }
            }
        }
    }
  return NULL;
}

static RoscoTypeContextServiceType *
type_context_register_service (RoscoTypeContext *ctx, RoscoServiceType *type)
{
  RoscoTypeContextServiceType *node = DSK_NEW (RoscoTypeContextServiceType);
  node->service_type = type;
  RoscoTypeContextServiceType *conflict;
  DSK_RBTREE_INSERT (GET_SERVICE_TREE (ctx), node, conflict);
  assert(conflict == NULL);
  return node;
}

RoscoServiceType *
rosco_type_context_get_service(RoscoTypeContext    *context,
			       const char          *name,
                               ssize_t              opt_name_len,
			       DskError           **error)
{
  char *to_free = NULL;
  const char *normalized_name = normalize_type_name (name, opt_name_len, &to_free);

  RoscoTypeContextServiceType *rv_stype = NULL;
#define COMPARE_NORMALIZED_NAME_TO_SERVICE(nname, t, rv) rv = strcmp(nname, t->service_type->name)
  DSK_RBTREE_LOOKUP_COMPARATOR (GET_SERVICE_TREE (context), normalized_name, COMPARE_NORMALIZED_NAME_TO_SERVICE, rv_stype);
  if (rv_stype != NULL)
    {
      dsk_free (to_free);
      return rv_stype->service_type;
    }
  
  for (unsigned i = 0; i < context->n_dirs; i++)
    {
      char *filename = dsk_strdup_printf ("%s/srv/%s.srv", context->dirs[i], normalized_name);
      size_t content_size;
      // read file, pass to message parser
      uint8_t *contents = dsk_file_get_contents (filename, &content_size, NULL);
      if (contents != NULL)
	{
	  size_t n_input_fields;
	  RoscoMessageTypeField *input_fields;
	  size_t sizeof_input_message, alignof_input_message;
	  size_t n_output_fields;
	  RoscoMessageTypeField *output_fields;
	  size_t sizeof_output_message, alignof_output_message;

          const uint8_t *end_minusminusminus;
          const uint8_t *minusminusminus = find_minusminusminus_sep (content_size, contents, &end_minusminusminus);
          if (minusminusminus == NULL)
            {
              dsk_set_error (error, "missing --- in service definition file %s", filename);
	      dsk_free (contents);
	      dsk_free (filename);
	      goto cleanup_and_return_rv;
            }


	  if (!parse_message_fields_from_string (context,
						 minusminusminus-contents, contents,
						 filename, 1,
						 &n_input_fields, &input_fields,
						 &sizeof_input_message,
						 &alignof_input_message,
						 error))
	    { 
	      dsk_free (contents);
	      dsk_free (filename);
	      goto cleanup_and_return_rv;
	    }
          const uint8_t *content_end = contents + content_size;
          const uint8_t *response_start = end_minusminusminus;
          unsigned response_line_no = count_newlines (response_start - contents, contents);
          fprintf(stderr, "calling parse_message_fields_from_string..\n");
	  if (!parse_message_fields_from_string (context,
                                                 content_end - response_start,
						 response_start,
						 filename, response_line_no,
						 &n_output_fields, &output_fields,
						 &sizeof_output_message,
						 &alignof_output_message,
						 error))
	    { 
	      dsk_free (contents);
	      dsk_free (filename);
	      goto cleanup_and_return_rv;
	    }

	  RoscoServiceType *rv = DSK_NEW0 (RoscoServiceType);
          rv->is_static = DSK_FALSE;
          rv->name = dsk_strdup (normalized_name);
	  rv->cname = name_to_cname (normalized_name);

	  RoscoMessageType *input_mt = DSK_NEW0 (RoscoMessageType);
          char *input_normalized_name = dsk_strdup_printf("%s/Request", normalized_name);
	  input_mt->base.type = ROSCO_BUILTIN_TYPE_MESSAGE;
	  input_mt->base.cname = name_to_cname (input_normalized_name);
	  input_mt->base.name = input_normalized_name; /* takes ownership */
          setup_basename (&input_mt->base);
	  input_mt->base.func_prefix_name = message_name_to_func_prefix (input_normalized_name);
	  input_mt->base.sizeof_ctype = sizeof_input_message;
	  input_mt->base.alignof_ctype = alignof_input_message;
	  input_mt->base.serialize = message_serialize;
	  input_mt->base.deserialize = message_deserialize;
          input_mt->base.c_input_arg_type = dsk_strdup_printf ("const %s *", input_mt->base.cname);
	  input_mt->n_fields = n_input_fields;
	  input_mt->fields = input_fields;
          rv->input = input_mt;

	  RoscoMessageType *output_mt = DSK_NEW0 (RoscoMessageType);
          char *output_normalized_name = dsk_strdup_printf("%s/Response", normalized_name);
	  output_mt->base.type = ROSCO_BUILTIN_TYPE_MESSAGE;
	  output_mt->base.name = output_normalized_name; /* takes ownership */
	  output_mt->base.cname = name_to_cname (output_normalized_name);
          setup_basename (&output_mt->base);
	  output_mt->base.func_prefix_name = message_name_to_func_prefix (output_normalized_name);
	  output_mt->base.sizeof_ctype = sizeof_output_message;
	  output_mt->base.alignof_ctype = alignof_output_message;
	  output_mt->base.serialize = message_serialize;
	  output_mt->base.deserialize = message_deserialize;
          output_mt->base.c_input_arg_type = dsk_strdup_printf ("const %s *", output_mt->base.cname);
	  output_mt->n_fields = n_output_fields;
	  output_mt->fields = output_fields;
          rv->output = output_mt;

	  rv_stype = type_context_register_service (context, rv);
          dsk_free (filename);
          dsk_free (contents);
          goto cleanup_and_return_rv;
	}
      dsk_free (filename);
    }
  dsk_set_error (error, "service type %s not found", normalized_name);
  
cleanup_and_return_rv: 
  dsk_free (to_free);
  return rv_stype ? rv_stype->service_type : NULL;
}

static void
free_type_and_array_types (RoscoType *type)
{
  if (type->is_static)
    return;
  switch (type->type)
    {
      case ROSCO_BUILTIN_TYPE_MESSAGE:
	dsk_free (type->name);
	dsk_free (type->cname);
	dsk_free (type->func_prefix_name);
	break;
      default:
        break;
    }
  free_type_and_array_types ((RoscoType *) type->vararray_type);
  for (unsigned i = 0; i < type->n_fixed_array_types; i++)
    free_type_and_array_types ((RoscoType *) type->fixed_array_types[i]);
  dsk_free (type->fixed_array_types);
  dsk_free (type);
}

static void
free_type_node_recursive (RoscoTypeContextType *context_type)
{
  if (context_type == NULL)
    return;

  free_type_node_recursive (context_type->left);
  free_type_node_recursive (context_type->right);
  free_type_and_array_types (context_type->type);
  dsk_free (context_type);
}

static void
free_service_type (RoscoServiceType *stype)
{
  if (stype->is_static)
    return;
  free_type_and_array_types ((RoscoType *) stype->input);
  free_type_and_array_types ((RoscoType *) stype->output);
  dsk_free (stype->name);
  dsk_free (stype);
}

static void
free_service_node_recursive (RoscoTypeContextServiceType *service_type)
{
  if (service_type == NULL)
    return;

  free_service_node_recursive (service_type->left);
  free_service_node_recursive (service_type->right);

  free_service_type (service_type->service_type);
  dsk_free (service_type);
}

void
rosco_type_context_destroy (RoscoTypeContext *context)
{
  dsk_assert (context->recursion_guards == NULL);
  free_type_node_recursive (context->types_by_name);
  free_service_node_recursive (context->services_by_name);
  dsk_free (context);
}
