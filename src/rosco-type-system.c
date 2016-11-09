
RoscoMessageContext *
rosco_message_context_new     (unsigned             n_dirs,
                               char               **dirs)
{
  RoscoMessageContext *rv = ROSCO_NEW (RoscoMessageContext);
  rv->n_dirs = n_dirs;
  rv->dirs = ROSCO_NEW_ARRAY (char *, n_dirs);
  for (unsigned i = 0; i < n_dirs; i++)
    rv->dirs[i] = rosco_strdup (dirs[i]);
  rv->types_by_name = NULL;

  // register base types
  ...

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

static RoscoMessageContextType *
_rosco_message_context_get     (RoscoMessageContext *context,
                                const char          *normalized_name,
                                const char          *error_location,
                                RoscoError         **error)
{
  RoscoMessageContextType *mctype;
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
rosco_message_context_get     (RoscoMessageContext *context,
                               const char          *name,
                               RoscoError         **error)
{
  char *to_free = NULL;
  const char *nname = normalize_type_name (name, &to_free);
  RoscoMessageContextType *type = _rosco_message_context_get (context, nname, error);
  if (to_free != NULL)
    free (to_free);
  return type == NULL ? NULL : type->type;
}

void
rosco_message_context_destroy (RoscoMessageContext *context)
{
}

