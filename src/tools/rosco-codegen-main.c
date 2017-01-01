#include "../rosco-type-system.h"
#include "../dsk/dsk.h"
#include "../dsk/dsk-contained-array-macros.h"
#include <stdio.h>
#include <string.h>

struct StrArray {
  DSK_CONTAINED_ARRAY_DECLARE (strs, char *);
};
#define STR_ARRAY_INIT { DSK_CONTAINED_ARRAY_INITIALIZER (16) }

#if 0
static void str_array_append_take (struct StrArray *arr, char *take)
{
  DSK_CONTAINED_ARRAY_APPEND (arr->strs, take);
}
#endif

static void str_array_append (struct StrArray *arr, const char *str)
{
  DSK_CONTAINED_ARRAY_APPEND (arr->strs, dsk_strdup (str));
}

static void add_strings_from_fp (struct StrArray *arr, FILE *fp)
{
  char buf[2048];
  while (fgets (buf, sizeof (buf), fp) != NULL)
    {
      char *end = strchr (buf, 0);
      while (buf < end && dsk_ascii_isspace (*(end-1)))
        end--;
      *end = 0;
      if (buf[0] == '#' || buf == end)
        continue;
      str_array_append (arr, buf);
    }
}

static const char short_desc[] = 
    "Generate Message and Services for use with ROSCO"
;
static const char long_desc[] = 
  "Read a bunch of .srv and .msg files from the path, "
  "and generate .h and .c files.\n\n"
;

static struct StrArray source_dirs = STR_ARRAY_INIT;
static struct StrArray message_type_names = STR_ARRAY_INIT;
static struct StrArray service_type_names = STR_ARRAY_INIT;
static DSK_CMDLINE_CALLBACK_DECLARE(add_arg_value_or_list_from_file_to_str_array)
{
  (void) arg_name;
  struct StrArray *arr = callback_data;
  if (arg_value[0] == '@')
    {
      FILE *fp = fopen (arg_value + 1, "r");
      if (fp == NULL)
        {
          dsk_set_error (error, "file open failed: %s", strerror (errno));
          return DSK_FALSE;
        }
      add_strings_from_fp (arr, fp);
      fclose (fp);
    }
  else
    str_array_append (arr, arg_value);
  return DSK_TRUE;
}

static void 
generate_message_type (RoscoMessageType *type,
                       DskBuffer        *ccode,
                       DskBuffer        *hcode)
{
  dsk_buffer_printf (
    hcode,
    "typedef struct %s %s;\n"
    "struct %s\n{\n  RoscoMessage base_instance;\n",
    type->base.cname, type->base.cname,
    type->base.cname
  );
  unsigned i;
  for (i = 0; i < type->n_fields; i++)
    {
      dsk_buffer_printf (
        hcode,
        "  %s %s;\n",
        type->fields[i].type->cname,
        type->fields[i].name
      );
    }
  dsk_buffer_append_string (hcode, "};\n\n");

  int func_prefix_name_len = strlen (type->base.func_prefix_name);
  
#if 0
  dsk_buffer_printf (hcode,
    "dsk_boolean\n"
    "%*s__serialize   (const %s *value,\n"
    "%*s               DskBuffer *target,\n"
    "%*s               DskError **error);\n"
    "dsk_boolean\n"
    "%*s__deserialize (DskBuffer *buffer,\n"
    "%*s               %s *out,\n"
    "%*s               DskError **error);\n",
    0, type->base.func_prefix_name,
    type->base.cname,
    func_prefix_name_len, "",
    func_prefix_name_len, "",
    0, type->base.func_prefix_name,
    func_prefix_name_len, "",
    type->base.cname,
    func_prefix_name_len, ""
  );
#endif

  
  dsk_buffer_printf (ccode,
    "static dsk_boolean\n"
    "%*s__serialize_f   (RoscoType         *type,\n"
    "%*s                 const void        *ptr_value,\n"
    "%*s                 DskBuffer         *out,\n"
    "%*s                 DskError         **error)\n"
    "{\n"
    "  assert(type == %s__get_type());\n"
    "  assert(((RoscoMessage *) ptr_value)->message_type == (RoscoMessageType *) type);\n"
    "  return %s__serialize (ptr_value, out, error);\n"
    "}\n\n",
    0, type->base.func_prefix_name,
    func_prefix_name_len, "",
    func_prefix_name_len, "",
    func_prefix_name_len, "",
    type->base.func_prefix_name,
    type->base.func_prefix_name
  );

  dsk_buffer_printf (ccode,
    "static dsk_boolean\n"
    "%*s__deserialize_f   (RoscoType         *type,\n"
    "%*s                   DskBuffer         *in,\n"
    "%*s                   void              *ptr_value,\n"
    "%*s                   DskError         **error)\n"
    "{\n"
    "  assert(type == %s__get_type());\n"
    "  return %s__deserialize (in, ptr_value, error);\n"
    "}\n\n",
    0, type->base.func_prefix_name,
    func_prefix_name_len, "",
    func_prefix_name_len, "",
    func_prefix_name_len, "",
    type->base.func_prefix_name,
    type->base.func_prefix_name
  );

  if (type->base.destruct == NULL)
    dsk_buffer_printf (ccode, "#define %s__destruct_f NULL\n", type->base.func_prefix_name);
  else
    dsk_buffer_printf (ccode,
    "static void\n"
    "%*s__destruct_f   (RoscoType         *type,\n"
    "%*s                void              *ptr_value)\n"
    "{\n"
    "  assert(type == %s__get_type());\n"
    "  return %s__deserialize (in, ptr_value, error);\n"
    "}\n\n",
    0, type->base.func_prefix_name,
    func_prefix_name_len, "",
    type->base.func_prefix_name,
    type->base.func_prefix_name
  );

  // C File:  define RoscoMessageType
  dsk_buffer_printf (ccode,
    "static RoscoMessageTypeField %s__fields[%u] = {\n",
    type->base.func_prefix_name, type->n_fields
  );
  for (size_t i = 0; i < type->n_fields; i++)
    {
      dsk_buffer_printf (ccode,
        "  { \"%s\", NULL, offsetof(%s, %s) },\n",
        type->fields[i].name,
        type->base.cname,
        type->fields[i].name
      );
    }
  dsk_buffer_printf (ccode,
    "};\n"
  );
  dsk_buffer_printf (ccode,
    "static RoscoMessageType %s__message_type = {\n"
    "  {\n"
    "    ROSCO_BUILTIN_TYPE_MESSAGE,\n"
    "    DSK_TRUE,    /* is_static */\n"
    "    DSK_TRUE,    /* pass_by_ref */\n"
    "    \"%s\",      /* cname */\n"
    "    \"%s\",      /* name */\n"
    "    \"%s\",      /* base_name */\n"
    "    \"%s\",      /* func_prefix_name */\n"
    "    sizeof(%s),\n"
    "    alignof(%s),\n"
    "    \"%s\",      /* c_input_arg_type */\n"
    "    %s__serialize_f,\n"
    "    %s__deserialize_f,\n"
    "    %s__destruct_f,\n"
    "    NULL,    /* vararray_type */\n"
    "    0,    /* n_fixed_array_types */\n"
    "    NULL,    /* fixed_array_types */\n"
    "  },\n"
    "  %u,       /* n_fields */\n"
    "  %s__fields,\n"
    "};\n",
    type->base.func_prefix_name,
    type->base.cname,
    type->base.name,
    type->base.base_name,
    type->base.func_prefix_name,
    type->base.cname,  // for sizeof
    type->base.cname,  // for alignof
    type->base.c_input_arg_type,
    type->base.func_prefix_name,
    type->base.func_prefix_name,
    type->base.func_prefix_name,
    type->n_fields,
    type->base.func_prefix_name
  );

  int func_prefix_len = strlen (type->base.func_prefix_name);

  // C File:  implement serialize/deserialize
  dsk_buffer_printf (hcode,
    "static inline dsk_boolean\n"
    "%s__serialize   (const %s *value,\n"
    "%*s              DskBuffer *target,\n"
    "%*s              DskError **error)\n"
    "{\n",
    type->base.func_prefix_name,
    type->base.cname,
    func_prefix_len, "",
    func_prefix_len, "");
  for (unsigned i = 0; i < type->n_fields; i++)
    {
      RoscoMessageTypeField *field = type->fields + i;
      RoscoType *ftype = field->type;
      dsk_buffer_printf (hcode,
       "  if (!%s__serialize (%s(value->%s), target, error))\n"
       "    {\n"
       "      return DSK_FALSE;\n"
       "    }\n",
       ftype->func_prefix_name, ftype->pass_by_ref ? "&" : "",
       field->name);
    }
  dsk_buffer_printf(hcode,
    "  return DSK_TRUE;\n"
    "}\n"
  );
  dsk_buffer_printf (hcode,
    "static inline dsk_boolean\n"
    "%*s__deserialize  (DskBuffer *in,\n"
    "%*s                %s *out,\n"
    "%*s                DskError **error)\n"
    "{\n",
    0, type->base.func_prefix_name,
    func_prefix_len, "", type->base.cname,
    func_prefix_len, "");
  for (unsigned i = 0; i < type->n_fields; i++)
    {
      RoscoMessageTypeField *field = type->fields + i;
      RoscoType *ftype = field->type;
      dsk_buffer_printf (hcode,
       "  if (!%s__deserialize (in, &(out->%s), error))\n"
       "    {\n"
       "      return DSK_FALSE;\n"
       "    }\n",
       ftype->func_prefix_name, field->name);
    }
  dsk_buffer_printf(hcode,
    "  return DSK_TRUE;\n"
    "}\n"
  );
  dsk_buffer_printf (hcode,
    "static inline void\n"
    "%s__destruct   (%s *value)\n"
    "{\n",
    type->base.func_prefix_name,
    type->base.cname);
  for (unsigned i = 0; i < type->n_fields; i++)
    {
      RoscoMessageTypeField *field = type->fields + i;
      RoscoType *ftype = field->type;
      if (ftype->destruct != NULL)
        dsk_buffer_printf (hcode,
           "  %s__destruct (%s(value->%s));\n",
           ftype->func_prefix_name, ftype->pass_by_ref ? "&" : "",
           field->name);
    }
  dsk_buffer_printf(hcode,
    "}\n"
  );
  dsk_buffer_printf (hcode,
    "RoscoType *%s__get_type();\n",
    type->base.func_prefix_name);
  dsk_buffer_printf (ccode,
    "RoscoType *%s__get_type()\n"
    "{\n",
    type->base.func_prefix_name
  );
  if (type->n_fields > 0)
    {
      dsk_buffer_printf (ccode,
        "  if (%s__fields[0].type == NULL)\n"
        "    {\n",
        type->base.func_prefix_name
      );
      for (unsigned i = 0; i < type->n_fields; i++)
        dsk_buffer_printf (ccode,
          "      %s__fields[%u].type = %s__get_type();\n",
          type->base.func_prefix_name,
          i,
          type->fields[i].type->func_prefix_name
        );
      dsk_buffer_printf (ccode,
        "    }\n"
      );
    }
  dsk_buffer_printf (ccode,
    "  return (RoscoType *) &%s__message_type;\n"
    "}\n",
    type->base.func_prefix_name
  );
}

static void 
generate_service_type (RoscoServiceType *type,
                       DskBuffer        *ccode,
                       DskBuffer        *hcode)
{
  generate_message_type (type->input, ccode, hcode);
  generate_message_type (type->output, ccode, hcode);

  dsk_buffer_printf (hcode, "extern RoscoServiceType %s;\n", type->cname);
  dsk_buffer_printf (ccode,
    "extern RoscoServiceType %s;\n",
     type->cname
  );
}

static void
generate_preambles (const char *type_name,
                    const char *name,
                    DskBuffer *c_code,
                    DskBuffer *h_code)
{
  dsk_buffer_printf(h_code, "/* Generated code. */\n\n");
  dsk_buffer_printf(c_code, "/* Generated code. */\n\n");
  dsk_buffer_printf(h_code, "#include <rosco.h>\n");
  dsk_buffer_printf(c_code, "#include <rosco/%ss/%s.h>\n", type_name, name);
}

int main(int argc, char **argv)
{
  dsk_boolean all = DSK_FALSE;
  dsk_boolean all_messages = DSK_FALSE;
  dsk_boolean all_services = DSK_FALSE;
  const char *common_dest_dir = NULL;
  const char *h_dest_dir = NULL;
  const char *c_dest_dir = NULL;
  dsk_cmdline_init (short_desc, long_desc, NULL, 0);

  dsk_cmdline_add_boolean (
    "all", "Render all services and messages", NULL, 0,
    &all
  );
  dsk_cmdline_add_shortcut ('a', "all");
  dsk_cmdline_add_boolean (
    "all-messages", "Render all messages", NULL, 0,
    &all_messages
  );
  dsk_cmdline_add_boolean (
    "all-services", "Render all services", NULL, 0,
    &all_services
  );
  dsk_cmdline_mutually_exclusive (DSK_FALSE, "all", "all-messages", NULL);
  dsk_cmdline_mutually_exclusive (DSK_FALSE, "all", "all-services", NULL);
  dsk_cmdline_add_string (
    "dest", "Destination for source/include files", "DIR", 0,
    &common_dest_dir
  );
  dsk_cmdline_add_string (
    "dest-c", "Destination for source files", "DIR", 0,
    &c_dest_dir
  );
  dsk_cmdline_add_string (
    "dest-h", "Destination for include files", "DIR", 0,
    &h_dest_dir
  );
  dsk_cmdline_mutually_exclusive (DSK_TRUE, "dest", "dest-c", NULL);
  dsk_cmdline_mutually_exclusive (DSK_TRUE, "dest", "dest-h", NULL);

  dsk_cmdline_add_func (
    "src-dir", "Directory which may include msg or srv directories", "DIR",
    0, add_arg_value_or_list_from_file_to_str_array, &source_dirs);
  dsk_cmdline_add_func (
    "message", "Add a message type to generate", "TYPE",
    0, add_arg_value_or_list_from_file_to_str_array, &message_type_names);
  dsk_cmdline_add_func (
    "service", "Add a service type to generate", "TYPE",
    0, add_arg_value_or_list_from_file_to_str_array, &service_type_names);

  dsk_cmdline_process_args (&argc, &argv);

  if (all) all_services = all_messages = DSK_TRUE;
  if (common_dest_dir) {
    c_dest_dir = dsk_strdup_printf ("%s", common_dest_dir);
    h_dest_dir = dsk_strdup_printf ("%s", common_dest_dir);
  }

  //TODO: check all message-types and service-types exist before beginning codegen
  RoscoMessageType **message_types = DSK_NEW_ARRAY (message_type_names.strs.length, RoscoMessageType *);
  RoscoServiceType **service_types = DSK_NEW_ARRAY (service_type_names.strs.length, RoscoServiceType *);
  RoscoTypeContext *ctx = rosco_type_context_new (source_dirs.strs.length, source_dirs.strs.data);
  for (size_t i = 0; i < message_type_names.strs.length; i++)
    {
      DskError *error = NULL;
      RoscoType *t = rosco_type_context_get (ctx, message_type_names.strs.data[i], -1, &error);
      if (t == NULL)
        {
          fprintf(stderr, "error parsing type for %s: %s\n",
                  message_type_names.strs.data[i], error->message);
          return 1;
        }
      if (t->type != ROSCO_BUILTIN_TYPE_MESSAGE)
        {
          fprintf(stderr, "error: type %s: not a message\n",
                  message_type_names.strs.data[i]);
          return 1;
        }
      message_types[i] = (RoscoMessageType *) t;
    }
  for (size_t i = 0; i < service_type_names.strs.length; i++)
    {
      DskError *error = NULL;
      RoscoServiceType *t = rosco_type_context_get_service (ctx, service_type_names.strs.data[i], -1, &error);
      if (t == NULL)
        {
          fprintf(stderr, "error parsing service %s: %s\n",
                  service_type_names.strs.data[i], error->message);
          return 1;
        }
      service_types[i] = t;
    }
    
  for (size_t i = 0; i < message_type_names.strs.length; i++)
    {
dsk_warning("generating message %u: %s", (unsigned)i, message_type_names.strs.data[i]);
      DskBuffer h_code = DSK_BUFFER_INIT;
      DskBuffer c_code = DSK_BUFFER_INIT;
      generate_preambles ("message", message_type_names.strs.data[i], &c_code, &h_code);
      generate_message_type (message_types[i], &c_code, &h_code);
      char *h_path = dsk_strdup_printf ("%s/rosco/messages/%s.h", h_dest_dir, message_type_names.strs.data[i]);
      char *c_path = dsk_strdup_printf ("%s/rosco/messages/%s.c", c_dest_dir, message_type_names.strs.data[i]);
      dsk_buffer_dump (&h_code, h_path, DSK_BUFFER_DUMP_DRAIN|DSK_BUFFER_DUMP_FATAL_ERRORS, NULL);
      dsk_buffer_dump (&c_code, c_path, DSK_BUFFER_DUMP_DRAIN|DSK_BUFFER_DUMP_FATAL_ERRORS, NULL);
      dsk_free (c_path);
      dsk_free (h_path);
    }
  for (size_t i = 0; i < service_type_names.strs.length; i++)
    {
dsk_warning("generating service %u: %s", (unsigned)i, service_type_names.strs.data[i]);
      DskBuffer h_code = DSK_BUFFER_INIT;
      DskBuffer c_code = DSK_BUFFER_INIT;
      generate_preambles ("service", service_type_names.strs.data[i], &c_code, &h_code);
      generate_service_type (service_types[i], &c_code, &h_code);
      char *h_path = dsk_strdup_printf ("%s/rosco/services/%s.h", h_dest_dir, service_type_names.strs.data[i]);
      char *c_path = dsk_strdup_printf ("%s/rosco/services/%s.c", c_dest_dir, service_type_names.strs.data[i]);
      dsk_buffer_dump (&h_code, h_path, DSK_BUFFER_DUMP_DRAIN|DSK_BUFFER_DUMP_FATAL_ERRORS, NULL);
      dsk_buffer_dump (&c_code, c_path, DSK_BUFFER_DUMP_DRAIN|DSK_BUFFER_DUMP_FATAL_ERRORS, NULL);
      dsk_free (c_path);
      dsk_free (h_path);
    }
  return 0;
}
