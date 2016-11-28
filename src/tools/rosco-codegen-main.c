#include "rosco-type-system.h"
#include <dsk/dsk.h>

static const char short_desc[] = 
    "Generate Message and Services for use with ROSCO"
;
static const char long_desc[] = 
  "Read a bunch of .srv and .msg files from the path, "
  "and generate .h and .c files.\n\n"
;
  const c

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
    "dest", "Destination for source/include files", NULL, 0,
    &common_dest_dir
  );
  dsk_cmdline_add_string (
    "dest-c", "Destination for source files", NULL, 0,
    &c_dest_dir
  );
  dsk_cmdline_add_string (
    "dest-h", "Destination for include files", NULL, 0,
    &h_dest_dir
  );
  dsk_cmdline_mutually_exclusive (DSK_TRUE, "dest", "dest-c", NULL);
  dsk_cmdline_mutually_exclusive (DSK_TRUE, "dest", "dest-h", NULL);

  dsk_cmdline_process_args (&argc, &argv);

  if (all) all_services = all_messages = DSK_TRUE;
  if (common_dest_dir) {
    dest_c = dsk_strdup_printf ("%s/src", common_dest_dir);
    dest_h = dsk_strdup_printf ("%s/include", common_dest_dir);
  }

  ...
}