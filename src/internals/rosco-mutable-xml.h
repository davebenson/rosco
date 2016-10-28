
typedef struct RoscoMutableXML RoscoMutableXML;

struct RoscoMutableXMLNamespace {
  char *url;
  unsigned ref_count;
};

struct RoscoMutableXML_Base {
  RoscoMutableXMLNamespace *ns;                 // unused for now
};

typedef struct {
  RoscoMutableXMLNamespace *ns;                 // unused for now
  char *name;                                   // (private detail: these are allocated separately at the moment)
  char *value;
} RoscoMutableXMLAttr;

struct RoscoMutableXML_Node {
  RoscoMutableXML_Base base;

  unsigned n_attrs;
  RoscoMutableXMLAttr *attrs;

  unsigned n_children;
  RoscoMutableXML **children;

  // private
  unsigned n_attrs_alloced;
  unsigned n_children_alloced;
};

struct RoscoMutableXML_ProcessingInstruction {
  RoscoMutableXML_Base base;
  char *text;
};

struct RoscoMutableXML_Document {
  RoscoMutableXML_Base base;
  RoscoMutableXML_ProcessingInstruction *pi_list;
  RoscoMutableXML_Node *root;
};


union RoscoMutableXML {
  RoscoMutableXML_Base base;
  RoscoMutableXML_Node node;
  RoscoMutableXML_Document document;
};

RoscoBinaryData *rosco_mutable_xml_serialize (const RoscoMutableXML *in);
RoscoMutableXML_Document *rosco_mutable_xml_document_parse (RoscoBuffer *buffer, RoscoError **error);

