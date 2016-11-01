#include <stdlib.h>
#include <string.h>
#include "rosco.h"

RoscoXml *rosco_xml_ref   (RoscoXml *xml)
{
  ++(xml->ref_count);
  return xml;
}

static inline void
rosco_xml_filename_unref (RoscoXmlFilename *filename)
{
  if (--(filename->ref_count) == 0)
    rosco_free (filename);
}

void    rosco_xml_unref (RoscoXml *xml)
{
restart:
  if (--(xml->ref_count) == 0)
    {
      if (xml->filename)
        rosco_xml_filename_unref (xml->filename);
      if (xml->type == ROSCO_XML_ELEMENT)
        {
          unsigned n = xml->n_children;
          RoscoXml *tail = NULL;
          unsigned i;
          for (i = 0; i < n; i++)
            {
              if (xml->children[i]->ref_count > 1)
                --(xml->children[i]->ref_count);
              else if (tail)
                rosco_xml_unref (xml->children[i]);
              else
                tail = xml->children[i];
            }
          if (tail)
            {
              rosco_free (xml);
              xml = tail;
              goto restart;
            }
        }
      rosco_free (xml);
    }
}

/* --- TODO: add wad of constructors --- */
RoscoXml *rosco_xml_text_new_len (unsigned length,
                              const char *data)
{
  RoscoXml *xml = rosco_malloc (length + 1 + sizeof (RoscoXml));
  memcpy (xml + 1, data, length);
  xml->str = (char*) (xml + 1);
  xml->str[length] = 0;
  xml->filename = NULL;
  xml->line_no = 0;
  xml->type = ROSCO_XML_TEXT;
  xml->ref_count = 1;
  return xml;
}
RoscoXml *rosco_xml_text_new     (const char *data)
{
  return rosco_xml_text_new_len (strlen (data), data);
}
RoscoXml rosco_xml_empty_text_global = {
  0,             /* line_no */
  NULL,          /* filename */
  ROSCO_XML_TEXT,  /* type */
  1,             /* ref_count */
  "",            /* str */
  NULL,          /* attrs */
  0,             /* n_children */
  NULL,          /* children */
};

RoscoXml *rosco_xml_comment_new_len (unsigned length,
                                 const char *text)
{
  RoscoXml *rv = rosco_xml_text_new_len (length, text);
  rv->type = ROSCO_XML_COMMENT;
  return rv;
}
static RoscoXml *condense_text_nodes (unsigned n, RoscoXml **xml)
{
  unsigned length = 0;
  char *at;
  unsigned i;
  RoscoXml *rv;
  rosco_assert (n > 1);
  for (i = 0; i < n; i++)
    length += strlen (xml[i]->str);
  rv = rosco_malloc (sizeof (RoscoXml) + length + 1);
  rv->str = (char*)(rv+1);
  rv->filename = NULL;
  rv->line_no = 0;
  rv->type = ROSCO_XML_TEXT;
  rv->ref_count = 1;
  at = rv->str;
  for (i = 0; i < n; i++)
    {
      at = rosco_stpcpy (at, xml[i]->str);
      rosco_xml_unref (xml[i]);
    }
  return rv;
}

RoscoXml *rosco_xml_new_empty   (const char *name)
{
  return _rosco_xml_new_elt_parse (0, strlen (name) + 1, name,
                                 0, NULL, ROSCO_FALSE);
}

RoscoXml *
rosco_xml_text_child_new (const char *name,
                        const char *contents)
{
  return rosco_xml_new_take_1 (name, rosco_xml_text_new (contents));
}

RoscoXml *rosco_xml_new_take_1   (const char *name,
                              RoscoXml     *child)
{
  return _rosco_xml_new_elt_parse (0, strlen (name) + 1, name,
                                 1, &child,
                                 ROSCO_FALSE);
}

RoscoXml *rosco_xml_new_take_n   (const char *name,
                              unsigned    n_children,
                              RoscoXml    **children)
{
  return _rosco_xml_new_elt_parse (0, strlen (name) + 1, name,
                                 n_children, children,
                                 ROSCO_TRUE);
}
RoscoXml *rosco_xml_new_take_list (const char *name,
                               RoscoXml     *first_or_null,
                               ...)
{
  va_list args;
  unsigned count = 0;
  RoscoXml *at;
  RoscoXml **children;

  va_start (args, first_or_null);
  for (at = first_or_null; at != NULL; at = va_arg (args, RoscoXml *))
    count++;
  va_end (args);

  children = alloca (sizeof (RoscoXml *) * count);
  count = 0;
  va_start (args, first_or_null);
  for (at = first_or_null; at != NULL; at = va_arg (args, RoscoXml *))
    children[count++] = at;
  va_end (args);

  return rosco_xml_new_take_n (name, count, children);
}

RoscoXml *_rosco_xml_new_elt_parse (unsigned n_attrs,
                                unsigned name_kv_space,
                                const char *name_and_attrs,
                                unsigned n_children,
                                RoscoXml **children,
                                rosco_boolean condense_text)
{
  RoscoXml *rv;
  unsigned i;
  char *at;
  if (condense_text && n_children > 1)
    {
      for (i = 0; i < n_children - 1; i++)
        if (children[i]->type == ROSCO_XML_TEXT
         && children[i+1]->type == ROSCO_XML_TEXT)
          break;
      if (i < n_children - 1)
        {
          unsigned o = i;
          while (i < n_children)
            {
              /* Condense more than one text node together */
              unsigned start_run = i;
              i += 2;
              while (i < n_children && children[i]->type == ROSCO_XML_TEXT)
                i++;
              children[o++] = condense_text_nodes (i - start_run,
                                                   children + start_run);

              /* Passthrough uncondensable items */
              while (i < n_children
                   && (children[i]->type != ROSCO_XML_TEXT
                    || i == n_children-1
                    || children[i+1]->type != ROSCO_XML_TEXT))
                children[o++] = children[i++];
            }
          n_children = o;
        }
    }
  rv = rosco_malloc (sizeof (char *) * (n_attrs * 2 + 1)
                           + sizeof (RoscoXml *) * n_children
                           + name_kv_space
                           + sizeof (RoscoXml));
  rv->line_no = 0;
  rv->filename = NULL;
  rv->type = ROSCO_XML_ELEMENT;
  rv->ref_count = 1;
  rv->n_children = n_children;
  rv->attrs = (char **) (rv + 1);
  rv->children = (RoscoXml **) (rv->attrs + n_attrs * 2 + 1);
  rv->str = (char*) (rv->children + n_children);
  memcpy (rv->str, name_and_attrs, name_kv_space);
  at = strchr (rv->str, 0) + 1;
  for (i = 0; i < 2*n_attrs; i++)
    {
      rv->attrs[i] = at;
      at = strchr (at, 0) + 1;
    }
  rv->attrs[i] = NULL;
  memcpy (rv->children, children, sizeof(RoscoXml *) * n_children);
  return rv;
}

void _rosco_xml_set_position (RoscoXml *xml,
                            RoscoXmlFilename *filename,
                            unsigned line_no)
{
  rosco_assert (xml->filename == NULL);
  xml->filename = filename;
  filename->ref_count++;
  xml->line_no = line_no;
}

const char *rosco_xml_find_attr (const RoscoXml *xml,
                               const char *name)
{
  unsigned i;
  if (xml->type == ROSCO_XML_ELEMENT)
    for (i = 0; xml->attrs[2*i] != NULL; i++)
      if (strcmp (xml->attrs[2*i], name) == 0)
        return xml->attrs[2*i+1];
  return NULL;
}

static void
get_all_text_to_buffer (const RoscoXml *xml,
                        RoscoBuffer *out)
{
  if (xml->type == ROSCO_XML_TEXT)
    rosco_buffer_append_string (out, xml->str);
  else if (xml->type == ROSCO_XML_ELEMENT)
    {
      unsigned i;
      for (i = 0; i < xml->n_children; i++)
        get_all_text_to_buffer (xml->children[i], out);
    }
}

char *rosco_xml_get_all_text (const RoscoXml *xml)
{
  RoscoBuffer buffer = ROSCO_BUFFER_INIT;
  char *rv;
  get_all_text_to_buffer (xml, &buffer);
  rv = rosco_malloc (buffer.size + 1);
  rv[buffer.size] = 0;
  rosco_buffer_read (&buffer, buffer.size, rv);
  return rv;
}

rosco_boolean
rosco_xml_is_element (const RoscoXml *xml, const char *name)
{
  return xml->type == ROSCO_XML_ELEMENT && strcmp (xml->str, name) == 0;
}
rosco_boolean
rosco_xml_is_whitespace (const RoscoXml *xml)
{
  const char *at;
  if (xml->type != ROSCO_XML_TEXT)
    return ROSCO_TRUE;
  at = xml->str;
  rosco_utf8_skip_whitespace (&at);
  return (*at == 0);
}
RoscoXml *
rosco_xml_find_solo_child (RoscoXml *xml,
                         RoscoError **error)
{
  if (xml->type != ROSCO_XML_ELEMENT)
    {
      rosco_set_error (error, "text or comment node has no children");
      return NULL;
    }
  switch (xml->n_children)
    {
    case 0: return rosco_xml_empty_text;
    case 1: return xml->children[0];
    case 2: if (rosco_xml_is_whitespace (xml->children[0]))
              return xml->children[1];
            if (rosco_xml_is_whitespace (xml->children[1]))
              return xml->children[0];
            break;
    case 3: if (rosco_xml_is_whitespace (xml->children[0])
             && rosco_xml_is_whitespace (xml->children[2]))
              return xml->children[1];
            break;
    }
  rosco_set_error (error, "multiple children under <%s>", xml->str);
  return NULL;
}
