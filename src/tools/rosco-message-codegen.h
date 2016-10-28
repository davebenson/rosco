
typedef struct _RoscoMessageCodegenType RoscoMessageCodegenType;
typedef struct _RoscoMessageCodegen RoscoMessageCodegen;

struct _RoscoMessageCodegenType {
  RoscoType *type;		// must be a message
};
struct _RoscoMessageCodegen {
  unsigned n_types;
  RoscoMessageCodegenType *types;
};

RoscoMessageCodegen * rosco_message_codegen_new (void);
void                  rosco_message_codegen_add_message_type (RoscoMessageCodegen *codegen,
                                                              RoscoType *type);

void                  rosco_message_codegen_generate (RoscoMessageCodegen *codegen,
                                                      RoscoBuffer         *h_file,
                                                      RoscoBuffer         *c_file);
void                  rosco_message_codegen_destroy (RoscoMessageCodegen *codegen);

