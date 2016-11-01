#include "rosco.h"

/* --- decoder --- */
typedef struct _RoscoXmlEscaperClass RoscoXmlEscaperClass;
struct _RoscoXmlEscaperClass
{
  RoscoOctetFilterClass base_class;
};
typedef struct _RoscoXmlEscaper RoscoXmlEscaper;
struct _RoscoXmlEscaper
{
  RoscoOctetFilter base_instance;
};

#define rosco_xml_escaper_init NULL
#define rosco_xml_escaper_finalize NULL

static rosco_boolean
rosco_xml_escaper_process (RoscoOctetFilter *filter,
                        RoscoBuffer      *out,
                        unsigned        in_length,
                        const uint8_t  *in_data,
                        RoscoError      **error)
{
  ROSCO_UNUSED (filter);
  ROSCO_UNUSED (error);
  while (in_length > 0)
    {
      unsigned n = 0;
      while (n < in_length && in_data[n] != '<' && in_data[n] != '>'
             && in_data[n] != '&')
        n++;
      if (n)
        rosco_buffer_append (out, n, in_data);
      in_data += n;
      in_length -= n;
      while (in_length > 0)
        if (*in_data == '<')
          {
            in_data++;
            in_length--;
            rosco_buffer_append (out, 4, "&lt;");
          }
        else if (*in_data == '>')
          {
            in_data++;
            in_length--;
            rosco_buffer_append (out, 4, "&gt;");
          }
        else if (*in_data == '&')
          {
            in_data++;
            in_length--;
            rosco_buffer_append (out, 5, "&amp;");
          }
        else
          break;
    }
  return ROSCO_TRUE;
}
#define rosco_xml_escaper_finish NULL
ROSCO_OCTET_FILTER_SUBCLASS_DEFINE(static, RoscoXmlEscaper, rosco_xml_escaper);
RoscoOctetFilter *rosco_xml_escaper_new            (void)
{
  return rosco_object_new (&rosco_xml_escaper_class);
}

RoscoOctetFilter *rosco_xml_unescaper_new  (void);
