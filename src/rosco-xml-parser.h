

typedef struct _RoscoXmlParserNamespaceConfig RoscoXmlParserNamespaceConfig;
typedef struct _RoscoXmlParserConfig RoscoXmlParserConfig;
typedef struct _RoscoXmlParser RoscoXmlParser;


/* --- xml parsing --- */

struct _RoscoXmlParserNamespaceConfig
{
  char *url;
  char *prefix;
};

typedef enum
{
  ROSCO_XML_PARSER_IGNORE_NS           = (1<<0),
  ROSCO_XML_PARSER_SUPPRESS_WHITESPACE = (1<<1),
  ROSCO_XML_PARSER_INCLUDE_COMMENTS    = (1<<2)
} RoscoXmlParserFlags;

RoscoXmlParserConfig *
rosco_xml_parser_config_new (RoscoXmlParserFlags flags,
			   unsigned          n_ns,
			   const RoscoXmlParserNamespaceConfig *ns,
			   unsigned          n_xmlpaths,
			   char            **xmlpaths,
                           RoscoError        **error);

/* IGNORE_NS will be supplied automatically to parsers 
   created in this way. */
RoscoXmlParserConfig *
rosco_xml_parser_config_new_simple (RoscoXmlParserFlags flags,
                                  const char       *path);

void
rosco_xml_parser_config_destroy (RoscoXmlParserConfig *config);


RoscoXmlParser *rosco_xml_parser_new (RoscoXmlParserConfig *config,
                                  const char         *display_filename);
RoscoXml       *rosco_xml_parser_pop (RoscoXmlParser       *parser,
                                  unsigned           *xmlpath_index_out);
rosco_boolean   rosco_xml_parser_feed(RoscoXmlParser       *parser,
                                  unsigned            length,
                                  const uint8_t      *data,
                                  RoscoError          **error_out);
void          rosco_xml_parser_free(RoscoXmlParser       *parser);

