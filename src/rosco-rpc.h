
// --- Query the master ---
typedef void (*RoscoRPC_AllTopicsCallback) (DskError *error,
                                            size_t    n_topics,
					    char    **topics,
					    void     *func_data);
void                rosco_rpc_get_all_topics     (DskURL              *master_url,
                                                  RoscoRPC_AllTopicsCallback func,
						  void                *func_data,
						  DskDestroyNotify     destroy);
