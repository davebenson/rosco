#include "../rosco-type-system.h"
#include "../dsk/dsk.h"
#include "../dsk/dsk-contained-array-macros.h"
#include <stdio.h>
#include <string.h>

struct StrArray {
  DSK_CONTAINED_ARRAY_DECLARE (strs, char *);
};
#define STR_ARRAY_INIT { DSK_CONTAINED_ARRAY_INITIALIZER (16) }

static void str_array_append_take (struct StrArray *arr, char *take)
{
  DSK_CONTAINED_ARRAY_APPEND (arr->strs, take);
}

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
    "struct %s\n{\n  RoscoMessage base_instance;\n",
    type->base.cname
  );
  unsigned i;
  for (i = 0; i < type->n_fields; i++)
    {
      dsk_buffer_printf (
        ccode,
        "  %s %s;\n",
        type->fields[i].type->cname,
        type->fields[i].name
      );
    }
  dsk_buffer_append_string (hcode, "};\n\n");
  
  dsk_buffer_printf (hcode,
    "void\n"
    "%s_serialize   (const %s *value,\n"
    "%*s              DskBuffer *target);\n"
    "dsk_boolean\n"
    "%s_deserialize (DskBuffer *buffer,\n"
    "%*s              %s *out);\n",
    type->base.func_prefix_name,
    type->base.cname,
    (int) strlen (type->base.func_prefix_name), "",
    type->base.func_prefix_name,
    (int) strlen (type->base.func_prefix_name), "",
    type->base.cname);

  // C File:  define RoscoMessageType
  ... 

  // C File:  implement serialize/deserialize
  ... 
}

int main(int argc, char **argv)
{
  dsk_boolean all = DSK_FALSE;
  dsk_boolean all_messages = DSK_FALSE;
  dsk_boolean all_services = DSK_FALSE;
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
    "message", "Add a message type to generate",
    0, add_arg_value_or_list_from_file_to_str_array, &message_type_names);
  dsk_cmdline_add_func (
    "service", "Add a service type to generate",
    0, add_arg_value_or_list_from_file_to_str_array, &service_type_names);

  dsk_cmdline_process_args (&argc, &argv);

  if (all) all_services = all_messages = DSK_TRUE;
  if (common_dest_dir) {
    dest_c = dsk_strdup_printf ("%s/src", common_dest_dir);
    dest_h = dsk_strdup_printf ("%s/include", common_dest_dir);
  }

  //TODO: check all message-types and service-types exist before beginning codegen
  RoscoMessageType **message_types = DSK_NEW_ARRAY (message_type_names.n, RoscoMessageType *);
  RoscoServiceType **service_types = DSK_NEW_ARRAY (service_type_names.n, RoscoServiceType *);
  RoscoTypeContext *ctx = rosco_type_context_new (n_source_dirs, (char**) source_dirs);
  for (size_t i = 0; i < message_type_names.n; i++)
    {
      DskError *error = NULL;
      RoscoType *t = rosco_type_context_get (ctx, message_type_names.strs[i], &error);
      if (t == NULL)
        {
          fprintf(stderr, "error parsing type for %s: %s\n",
                  message_type_names.strs[i], error->message);
          return 1;
        }
      if (t->type != ROSCO_BUILTIN_TYPE_MESSAGE)
        {
          fprintf(stderr, "error: type %s: not a message\n",
                  message_type_names.strs[i]);
          return 1;
        }
      message_types[i] = (RoscoMessageType *) t;
    }
  for (size_t i = 0; i < service_type_names.n; i++)
    {
      DskError *error = NULL;
      RoscoType *t = rosco_type_context_get_service (ctx, service_type_names.strs[i], &error);
      if (t == NULL)
        {
          fprintf(stderr, "error parsing service %s: %s\n",
                  service_type_names.strs[i], error->message);
          return 1;
        }
      service_types[i] = t;
    }
    
  for (size_t i = 0; i < message_type_names.n; i++)
    {
      DskBuffer h_code = DSK_BUFFER_INIT;
      DskBuffer c_code = DSK_BUFFER_INIT;
      generate_message_type (message_types[i], &h_code, &c_code);
      char *h_path = dsk_strdup_printf ("%s/%s.h", dest_h, message_type_names.strs[i]);
      char *c_path = dsk_strdup_printf ("%s/%s.c", dest_c, message_type_names.strs[i]);
      dsk_buffer_dump (&h_code, h_path, DSK_BUFFER_DUMP_DRAIN|DSK_BUFFER_DUMP_FATAL_ERRORS, NULL);
      dsk_buffer_dump (&c_code, c_path, DSK_BUFFER_DUMP_DRAIN|DSK_BUFFER_DUMP_FATAL_ERRORS, NULL);
      dsk_free (c_path);
      dsk_free (h_path);

    }
  
  RoscoType base;
  unsigned n_fields;
  RoscoMessageTypeField *fields;
 
  ...
}
