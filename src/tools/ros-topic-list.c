#include "rosco-node.h"

static void
handle_all_topics  (DskError *error,
		    size_t    n_topics,
		    char    **topics,
		    void     *func_data)
{
  size_t i;
  for (i = 0; i < n_topics; i++)
    printf("%s\n", topics[i]);
}

int main(int argc, char **argv)
{
  DskURL *url = rosco_master_url ();
  rosco_rpc_get_all_topics (url, handle_all_topics, NULL, (DskDestroyNotify) dsk_main_quit);
  return dsk_main_run ();
}
