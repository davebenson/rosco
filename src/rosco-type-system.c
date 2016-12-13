#include "rosco-type-system.h"
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
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
  #c_type,                                            \
  #name,                                              \
  #func_prefix,                                       \
  sizeof(c_type),                                     \
  alignof(c_type),                                    \
  rosco_type__serialize__##name,                      \
  rosco_type__deserialize__##name,                    \
  rosco_type__destruct__##name,                       \
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

static void
rosco_type__destruct__no_op (RoscoType *type, void *data)
{
  (void)type;
  (void)data;
}

static dsk_boolean
rosco_type__serialize__bool(RoscoType *type,
		            const void *ptr_value,
		            DskBuffer *out,
		            DskError **error)
{
  uint8_t v = * (const uint8_t *) ptr_value;
  if (v > 1)
    {
      dsk_set_error (error, "bool serialize: bool value must be 0 or 1");
      return DSK_FALSE;
    }
  dsk_buffer_append_byte (out, v);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__bool(RoscoType *type,
			      DskBuffer *in,
			      void        *ptr_value_out,
			      DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE("bool", in, 1, error);
  uint8_t v = dsk_buffer_read_byte (in);
  if (v > 1)
    {
      dsk_set_error (error, "bool deserialize: bool value must be 0 or 1");
      return DSK_FALSE;
    }
  * (uint8_t *) ptr_value_out = v;
  return DSK_TRUE;
}
#define rosco_type__destruct__bool rosco_type__destruct__no_op
static RoscoType rosco_type__bool = DEFINE_ROSCO_TYPE(BOOL, rosco_bool, bool, rosco_bool);

static dsk_boolean
rosco_type__serialize__uint8(RoscoType *type,
		             const void *ptr_value,
		             DskBuffer *out,
		             DskError **error)
{
  dsk_buffer_append_byte (out, * (const uint8_t *) ptr_value);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__uint8(RoscoType *type,
			       DskBuffer *in,
			       void        *ptr_value_out,
			       DskError **error)
{
  /* note: use type->name so that the same function works for signed int8 */
  DESERIALIZE_CHECK_MIN_SIZE(type->name, in, 1, error);
  * (uint8_t *) ptr_value_out = (uint8_t) dsk_buffer_read_byte (in);
  return DSK_TRUE;
}
#define rosco_type__destruct__uint8 rosco_type__destruct__no_op
static RoscoType rosco_type__uint8 = DEFINE_ROSCO_TYPE(UINT8, uint8_t, uint8, rosco_uint8);

#define rosco_type__serialize__int8 rosco_type__serialize__uint8
#define rosco_type__deserialize__int8 rosco_type__deserialize__uint8
#define rosco_type__destruct__int8 rosco_type__destruct__no_op
static RoscoType rosco_type__int8 = DEFINE_ROSCO_TYPE(INT8, int8_t, int8, rosco_int8);

static dsk_boolean
rosco_type__serialize__uint16(RoscoType *type,
		              const void *ptr_value,
		              DskBuffer *out,
		              DskError **error)
{
  uint16_t v = * (const uint16_t *) ptr_value;
  uint8_t data[2] = { v & 0xff, v >> 8 };   /* little-endian */
  dsk_buffer_append (out, 2, data);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__uint16(RoscoType *type,
			        DskBuffer *in,
			        void        *ptr_value_out,
			        DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE(type->name, in, 2, error);
  uint8_t buf[2];
  dsk_buffer_read (in, 2, buf);
  uint16_t v = (uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8) ;
  * (uint16_t *) ptr_value_out = v;
  return DSK_TRUE;
}
#define rosco_type__destruct__uint16 rosco_type__destruct__no_op
static RoscoType rosco_type__uint16 = DEFINE_ROSCO_TYPE(UINT16, uint16_t, uint16, rosco_uint16);

#define rosco_type__serialize__int16 rosco_type__serialize__uint16
#define rosco_type__deserialize__int16 rosco_type__deserialize__uint16
#define rosco_type__destruct__int16 rosco_type__destruct__no_op
static RoscoType rosco_type__int16 = DEFINE_ROSCO_TYPE(INT16, int16_t, int16, rosco_int16);
static dsk_boolean
rosco_type__serialize__uint32(RoscoType *type,
		              const void *ptr_value,
		              DskBuffer *out,
		              DskError **error)
{
  uint32_t v = * (const uint32_t *) ptr_value;
  uint8_t data[4] = { UINT32_TO_UINT8LE_ARRAY (v) };
  dsk_buffer_append (out, 4, data);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__uint32(RoscoType *type,
			        DskBuffer *in,
			        void        *ptr_value_out,
			        DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE(type->name, in, 4, error);
  uint8_t buf[4];
  dsk_buffer_read (in, 4, buf);
  uint32_t v =  (uint32_t)(buf[0])
             | ((uint32_t)(buf[1]) << 8)
             | ((uint32_t)(buf[2]) << 16)
             | ((uint32_t)(buf[3]) << 24);
  * (uint32_t *) ptr_value_out = v;
  return DSK_TRUE;
}
#define rosco_type__destruct__uint32 rosco_type__destruct__no_op
static RoscoType rosco_type__uint32 = DEFINE_ROSCO_TYPE(UINT32, uint32_t, uint32, rosco_uint32);

#define rosco_type__serialize__int32 rosco_type__serialize__uint32
#define rosco_type__deserialize__int32 rosco_type__deserialize__uint32
#define rosco_type__destruct__int32 rosco_type__destruct__no_op
static RoscoType rosco_type__int32 = DEFINE_ROSCO_TYPE(INT32, int32_t, int32, rosco_int32);

static dsk_boolean
rosco_type__serialize__uint64(RoscoType *type,
		              const void *ptr_value,
		              DskBuffer *out,
		              DskError **error)
{
  uint64_t v = * (const uint64_t *) ptr_value;
  uint8_t data[8] = { v & 0xff, v >> 8, v >> 16, v >> 24,
                      v >> 32, v >> 40, v >> 48, v >> 54 };
  dsk_buffer_append (out, 8, data);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__uint64(RoscoType *type,
			        DskBuffer *in,
			        void        *ptr_value_out,
			        DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE(type->name, in, 8, error);
  uint8_t buf[8];
  dsk_buffer_read (in, 8, buf);
  uint64_t v =  (uint64_t)(buf[0])
             | ((uint64_t)(buf[1]) << 8)
             | ((uint64_t)(buf[2]) << 16)
             | ((uint64_t)(buf[3]) << 24)
             | ((uint64_t)(buf[4]) << 32)
             | ((uint64_t)(buf[5]) << 40)
             | ((uint64_t)(buf[6]) << 48)
             | ((uint64_t)(buf[7]) << 54);
  * (uint64_t *) ptr_value_out = v;
  return DSK_TRUE;
}
#define rosco_type__destruct__uint64 rosco_type__destruct__no_op
static RoscoType rosco_type__uint64 = DEFINE_ROSCO_TYPE(UINT64, uint64_t, uint64, rosco_uint64);

#define rosco_type__serialize__int64 rosco_type__serialize__uint64
#define rosco_type__deserialize__int64 rosco_type__deserialize__uint64
#define rosco_type__destruct__int64 rosco_type__destruct__no_op
static RoscoType rosco_type__int64 = DEFINE_ROSCO_TYPE(INT64, int64_t, int64, rosco_int64);

#define rosco_type__serialize__float32 rosco_type__serialize__uint32
#define rosco_type__deserialize__float32 rosco_type__deserialize__uint32
#define rosco_type__destruct__float32 rosco_type__destruct__no_op
static RoscoType rosco_type__float32 = DEFINE_ROSCO_TYPE(FLOAT32, float, float32, rosco_float32);
#define rosco_type__serialize__float64 rosco_type__serialize__uint64
#define rosco_type__deserialize__float64 rosco_type__deserialize__uint64
#define rosco_type__destruct__float64 rosco_type__destruct__no_op
static RoscoType rosco_type__float64 = DEFINE_ROSCO_TYPE(FLOAT64, double, float64, rosco_float64);

static dsk_boolean
rosco_type__serialize__string(RoscoType *type,
		              const void *ptr_value,
		              DskBuffer *out,
		              DskError **error)
{
  const char *str = * (const char **) ptr_value;
  unsigned len = strlen (str);
  uint8_t len_enc[4] = {UINT32_TO_UINT8LE_ARRAY(len)};
  dsk_buffer_append (out, 4, len_enc);
  dsk_buffer_append (out, len, str);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__string(RoscoType *type,
			        DskBuffer *in,
			        void        *ptr_value_out,
			        DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE (type->name, in, 4, error);
  uint8_t header[4];
  dsk_buffer_read (in, 4, header);
  uint32_t len = ((uint32_t)header[0])
               | ((uint32_t)header[1] << 8)
               | ((uint32_t)header[2] << 16)
               | ((uint32_t)header[3] << 24);
  if (in->size < len)
    {
      dsk_set_error (error, "%s deserialize: end-of-data", type->name);
      return DSK_FALSE;
    }
  char *str = dsk_malloc (len + 1);
  dsk_buffer_read (in, len, str);
  str[len] = 0;
  return DSK_TRUE;
}
static void
rosco_type__destruct__string (RoscoType *type, void *ptr_data)
{
  (void)type;
  dsk_free (*(char**)ptr_data);
}
static RoscoType rosco_type__string = DEFINE_ROSCO_TYPE(STRING, char *, string, rosco_string);

static dsk_boolean
rosco_type__serialize__time(RoscoType *type,
		            const void *ptr_value,
		            DskBuffer *out,
		            DskError **error)
{
  RoscoTime time = * (const RoscoTime *) ptr_value;
  uint8_t enc[8] = {
    UINT32_TO_UINT8LE_ARRAY(time.secs),
    UINT32_TO_UINT8LE_ARRAY(time.nsecs)
  };
  dsk_buffer_append (out, 8, enc);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__time(RoscoType *type,
			        DskBuffer *in,
			        void        *ptr_value_out,
			        DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE (type->name, in, 8, error);
  uint8_t tmp[8];
  dsk_buffer_read (in, 8, tmp);
  RoscoTime *out = ptr_value_out;
  out->secs  = ((uint32_t)tmp[0])
             | ((uint32_t)tmp[1] << 8)
             | ((uint32_t)tmp[2] << 16)
             | ((uint32_t)tmp[3] << 24);
  out->nsecs = ((uint32_t)tmp[4])
             | ((uint32_t)tmp[5] << 8)
             | ((uint32_t)tmp[6] << 16)
             | ((uint32_t)tmp[7] << 24);

  // normalize the duration so that 0 <= nsecs < BILLION
  if (DSK_UNLIKELY (out->nsecs > BILLION))
    {
      unsigned s_in_n = out->nsecs / BILLION;
      out->nsecs %= BILLION;
      out->secs += s_in_n;
    }
  return DSK_TRUE;
}
#define rosco_type__destruct__time rosco_type__destruct__no_op
static RoscoType rosco_type__time = DEFINE_ROSCO_TYPE(TIME, RoscoTime, time, rosco_time);

static dsk_boolean
rosco_type__serialize__duration(RoscoType *type,
		                const void *ptr_value,
		                DskBuffer *out,
		                DskError **error)
{
  RoscoDuration duration = * (const RoscoDuration *) ptr_value;
  uint8_t enc[8] = {
    UINT32_TO_UINT8LE_ARRAY(duration.secs),
    UINT32_TO_UINT8LE_ARRAY(duration.nsecs)
  };
  dsk_buffer_append (out, 8, enc);
  return DSK_TRUE;
}
static dsk_boolean
rosco_type__deserialize__duration(RoscoType *type,
			          DskBuffer *in,
			          void        *ptr_value_out,
			          DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE (type->name, in, 8, error);
  uint8_t tmp[8];
  dsk_buffer_read (in, 8, tmp);
  RoscoTime *out = ptr_value_out;
  out->secs  = ((int32_t)tmp[0])
             | ((int32_t)tmp[1] << 8)
             | ((int32_t)tmp[2] << 16)
             | ((int32_t)tmp[3] << 24);
  out->nsecs = ((int32_t)tmp[4])
             | ((int32_t)tmp[5] << 8)
             | ((int32_t)tmp[6] << 16)
             | ((int32_t)tmp[7] << 24);

  // normalize the duration so that 0 <= nsecs < BILLION
  if (DSK_UNLIKELY (out->nsecs > BILLION))
    {
      unsigned s_in_n = out->nsecs / BILLION;
      out->nsecs %= BILLION;
      out->secs += s_in_n;
    }
  return DSK_TRUE;
}
#define rosco_type__destruct__duration rosco_type__destruct__no_op
static RoscoType rosco_type__duration = DEFINE_ROSCO_TYPE(DURATION, RoscoDuration, duration, rosco_duration);
static void
type_context_register_type (RoscoTypeContext *ctx, RoscoType *type)
{
  RoscoTypeContextType *node = DSK_NEW (RoscoTypeContextType);
  node->type = type;
  RoscoTypeContextType *conflict;
  DSK_RBTREE_INSERT (GET_TYPE_TREE (ctx), node, conflict);
  assert(conflict == NULL);
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
  type_context_register_type (rv, &rosco_type__bool);
  type_context_register_type (rv, &rosco_type__uint8);
  type_context_register_type (rv, &rosco_type__int8);
  type_context_register_type (rv, &rosco_type__uint16);
  type_context_register_type (rv, &rosco_type__int16);
  type_context_register_type (rv, &rosco_type__uint32);
  type_context_register_type (rv, &rosco_type__int32);
  type_context_register_type (rv, &rosco_type__uint64);
  type_context_register_type (rv, &rosco_type__int64);
  type_context_register_type (rv, &rosco_type__float32);
  type_context_register_type (rv, &rosco_type__float64);
  type_context_register_type (rv, &rosco_type__string);
  type_context_register_type (rv, &rosco_type__time);
  type_context_register_type (rv, &rosco_type__duration);

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
      dsk_assert (arr.count == arrtype->length);
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
                                  DskError **error)
{
  const char *at = (const char *) text;
  const char *end = at + text_size;
  DSK_TMP_ARRAY_DECLARE (RoscoMessageTypeField, fields, 16);
  unsigned lineno;
  size_t cur_offset = sizeof (RoscoMessage);
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
      RoscoType *field_type = rosco_type_context_get (context, at, end_type - at, error);
      if (field_type == NULL)
        {
          goto fail;
        }
      
      // round cur_offset up to alignment
      cur_offset = DSK_ALIGN(cur_offset, field_type->alignof_ctype);

      RoscoMessageTypeField field;
      field.type = field_type;
      field.name = dsk_strcut (start_field_name, end_field_name);
      field.offset = cur_offset;
      DSK_TMP_ARRAY_APPEND (fields, field);

      cur_offset += field_type->sizeof_ctype;
      lineno++;
    }

  // round cur_offset up to max_alignment
  cur_offset = DSK_ALIGN(cur_offset, MAX_ALIGN);

  *n_fields_out = fields.length;
  DSK_TMP_ARRAY_CLEAR_TO_ALLOCATION (fields, *fields_out);
  *sizeof_message_out = cur_offset;
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
  msg->ref_count = 1;
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
message_name_to_cname (const char *name)
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
    return mctype->type;
  
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
          char *filename = dsk_strdup_printf ("%s/%s", context->dirs[i], base_type);
          size_t content_size;
          // read file, pass to message parser
          uint8_t *contents = dsk_file_get_contents (filename, &content_size, NULL);
          if (contents != NULL)
            {
              size_t n_fields;
              RoscoMessageTypeField *fields;
              size_t sizeof_message;
              if (!parse_message_fields_from_string (context,
                                                     content_size, contents,
                                                     filename, 1,
                                                     &n_fields, &fields,
                                                     &sizeof_message,
                                                     error))
                { 
                  dsk_free (contents);
                  dsk_free (filename);
                  goto cleanup_and_return_rv;
                }

              RoscoMessageType *mt = DSK_NEW0 (RoscoMessageType);
              mt->base.type = ROSCO_BUILTIN_TYPE_MESSAGE;
              mt->base.cname = message_name_to_cname (normalized_name);
              mt->base.name = dsk_strdup (normalized_name);
              mt->base.func_prefix_name = message_name_to_func_prefix (normalized_name);
              mt->base.sizeof_ctype = sizeof (RoscoMessage *);
              mt->base.alignof_ctype = alignof (RoscoMessage *);
              mt->base.serialize = message_serialize;
              mt->base.deserialize = message_deserialize;
              mt->n_fields = n_fields;
              mt->fields = fields;
              mt->sizeof_message = sizeof_message;
              rv = (RoscoType *) mt;

              type_context_register_type (context, rv);
            }
          dsk_free (filename);
        }
      if (mctype == NULL)
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
  free_type_node_recursive ((RoscoType *) stype->input);
  free_type_node_recursive ((RoscoType *) stype->output);
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
