#include "rosco-type-system.h"
#include <stdalign.h>
#include <string.h>
#include "dsk/dsk-rbtree-macros.h"

#define COMPARE_TYPE_CONTEXT_TYPES(a,b, rv) \
  rv = strcmp(a->type->name, b->type->name)
#define GET_TYPE_TREE(ctx) \
  (ctx)->types_by_name, RoscoTypeContextType*, DSK_STD_GET_IS_RED, DSK_STD_SET_IS_RED, parent, left, right, COMPARE_TYPE_CONTEXT_TYPES

#define COMPARE_TYPE_CONTEXT_SERVICE_TYPES(a,b,rv) \
  rv = strcmp(a->service_type->name, b->service_type->name)
#define GET_SERVICE_TREE(ctx) \
  (ctx)->services_by_name, RoscoTypeContextServiceType*, DSK_STD_GET_IS_RED, DSK_STD_SET_IS_RED, parent, left, right, COMPARE_TYPE_CONTEXT_SERVICE_TYPES

#define DEFINE_ROSCO_TYPE(BUILTIN_BASE, c_type, name) \
{                                                     \
  ROSCO_BUILTIN_TYPE_##BUILTIN_BASE,                  \
  #c_type,                                            \
  #name,                                              \
  sizeof(c_type),                                     \
  alignof(c_type),                                    \
  rosco_type__serialize__##name,                      \
  rosco_type__deserialize__##name,                    \
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
static RoscoType rosco_type__bool = DEFINE_ROSCO_TYPE(BOOL, rosco_bool, bool);

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
  DESERIALIZE_CHECK_MIN_SIZE("uint8", in, 1, error);
  * (uint8_t *) ptr_value_out = (uint8_t) dsk_buffer_read_byte (in);
  return DSK_TRUE;
}
static RoscoType rosco_type__uint8 = DEFINE_ROSCO_TYPE(UINT8, uint8_t, uint8);

#define rosco_type__serialize__int8 rosco_type__serialize__uint8
static dsk_boolean
rosco_type__deserialize__int8 (RoscoType *type,
			       DskBuffer *in,
			       void        *ptr_value_out,
			       DskError **error)
{
  DESERIALIZE_CHECK_MIN_SIZE("int8", in, 1, error);
  * (int8_t *) ptr_value_out = (int8_t) dsk_buffer_read_byte (in);
  return DSK_TRUE;
}
static RoscoType rosco_type__int8 = DEFINE_ROSCO_TYPE(UINT8, uint8_t, uint8);

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
  rv->dirs = DSK_NEW_ARRAY (char *, n_dirs);
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

static RoscoType *
_rosco_type_get_array_type         (RoscoType *type)
{
  if (type->vararray_type == NULL)
    {
      RoscoArrayType *atype = ROSCO_NEW0 (RoscoArrayType);
      atype->base.type = ROSCO_BUILTIN_TYPE_ARRAY;
      atype->base.cname = ...
      atype->base.name = ...
      atype->base.sizeof_ctype = sizeof (RoscoArray);
      atype->base.alignof_ctype = alignof (RoscoArray);
      atype->base.serialize = array_serialize;
      atype->base.deserialize = array_deserialize;
      atype->element_type = type;
      atype->length = -1;
      type->vararray_type = (RoscoType *) atype;
    }
  return type->vararray_type;
}

static RoscoTypeContextType *
_rosco_type_context_get     (RoscoTypeContext *context,
                                const char          *normalized_name,
                                const char          *error_location,
                                DskError         **error)
{
  RoscoTypeContextType *mctype;
#define COMPARE_NORMALIZED_NAME(nname, t, rv) rv = strcmp(nname, t->name)
  DSK_RBTREE_LOOKUP_COMPARATOR (GET_TYPE_TREE (context), normalized_name, COMPARE_NORMALIZED_NAME, mctype);
#undef COMPARE_NORMALIZED_NAME
  if (mctype != NULL)
    return mctype;
  
  const char *end = strchr (normalized_name, 0);
  if (end == normalized_name)
    {
      ... empty type
    }

  if (*(end-1) == ']')
    {
      ssize_t len = -1;
      // the shortest possible type that ends in ']' is 3 characters: e.g. a[]
      if (end - 3 < normalized_name)
        {
          *error = rosco_error_new (ROSCO_ERROR_MESSAGE_CONTEXT_PARSE, "bad array specification at %s", error_location);
          return NULL;
        }

      if (*(end-2) == '[')
        {
          len = -1;                     // signal for variable length array
          element_type_end = end - 1;
        }
      else
        {
          for (const char *num_start = end - 1;
               num_start > normalized_name && isdigit (*(num_start - 1));
               num_start--)
            ;
          if (num_start == normalized_name || *(num_start-1) != '[')
            {
              *error = rosco_error_new (ROSCO_ERROR_MESSAGE_CONTEXT_PARSE,
                                        "bad fixed-length array specification at %s",
                                        error_location);
              return NULL;
            }
          ... parse number
          element_type_end = num_start - 1;
        }

      // allocate subname
      ... parse element type (recurse)

      // create new subtype (should not exist already, due to prior checks)
      ...
    }
  else
    {
      .. search path for msg files
    }

  ...
 *   LOOKUP_COMPARATOR(tree, key, key_comparator, out)
}

RoscoType *
rosco_type_context_get     (RoscoTypeContext *context,
                               const char          *name,
                               DskError         **error)
{
  char *to_free = NULL;
  const char *nname = normalize_type_name (name, &to_free);
  RoscoTypeContextType *type = _rosco_type_context_get (context, nname, error);
  if (to_free != NULL)
    free (to_free);
  return type == NULL ? NULL : type->type;
}

void
rosco_type_context_destroy (RoscoTypeContext *context)
{
}

