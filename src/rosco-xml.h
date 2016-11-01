typedef struct _RoscoXmlFilename RoscoXmlFilename;
typedef struct _RoscoXml RoscoXml;

typedef enum
{
  ROSCO_XML_ELEMENT,
  ROSCO_XML_TEXT,
  ROSCO_XML_COMMENT
} RoscoXmlType;

struct _RoscoXmlFilename
{
  unsigned ref_count;
  char *filename;
};

struct _RoscoXml
{
  /* all fields public and read-only */

  unsigned line_no;
  RoscoXmlFilename *filename;
  RoscoXmlType type;
  unsigned ref_count;
  char *str;		/* name for ELEMENT, text for COMMENT and TEXT */
  char **attrs;		/* key-value pairs */
  unsigned n_children;
  RoscoXml **children;
};

RoscoXml *rosco_xml_ref   (RoscoXml *xml);
void    rosco_xml_unref (RoscoXml *xml);

/* --- TODO: add wad of constructors --- */
RoscoXml *rosco_xml_text_new_len (unsigned length,
                              const char *data);
RoscoXml *rosco_xml_text_new     (const char *str);

RoscoXml *rosco_xml_new_take_1   (const char *name,
                              RoscoXml     *child);
RoscoXml *rosco_xml_text_child_new (const char *name,
                                const char *contents);

/* Create an empty xml node */
RoscoXml *rosco_xml_new_empty    (const char *name);

/* NOTE: we do NOT take the actual array 'children', just every element
   (every XML node) of it. */
RoscoXml *rosco_xml_new_take_n   (const char *name,
                              unsigned    n_children,
                              RoscoXml    **children);

RoscoXml *rosco_xml_new_take_list (const char *name,
                               RoscoXml     *first_or_null,
                               ...);

extern RoscoXml rosco_xml_empty_text_global;
#define rosco_xml_empty_text   (&rosco_xml_empty_text_global)

/* --- comments (not recommended much) --- */
RoscoXml *rosco_xml_comment_new_len (unsigned length,
                                 const char *text);


/* --- xml lookup functions --- */
const char *rosco_xml_find_attr (const RoscoXml *xml,
                               const char   *name);

rosco_boolean rosco_xml_is_whitespace (const RoscoXml *xml);
char *rosco_xml_get_all_text (const RoscoXml *xml);
rosco_boolean rosco_xml_is_element (const RoscoXml *xml, const char *name);
RoscoXml *rosco_xml_find_solo_child (RoscoXml *, RoscoError **error);

/* FOR INTERNAL USE ONLY: create an xml node from a packed set of
   attributes, and a set of children, which we will take ownership of.
   We will do text-node compacting.
 */
RoscoXml *_rosco_xml_new_elt_parse (unsigned n_attrs,
                                unsigned name_kv_space,
                                const char *name_and_attrs,
                                unsigned n_children,
                                RoscoXml **children,
                                rosco_boolean condense_text_nodes);
void _rosco_xml_set_position (RoscoXml *xml,
                            RoscoXmlFilename *filename,
                            unsigned line_no);
