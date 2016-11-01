#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <search.h>             /* for the seldom-used lfind() */
#include "rosco.h"
#include "rosco-rbtree-macros.h"

/* TODO:
   _ check for NUL
   _ beef up "directive" handling
   _ replace RoscoBuffer with simple array length/alloced/data
 */

#define MAX_DEPTH       64

#define DEBUG_PARSE_STATE_TREE     0

/* References:
        The Annotated XML Specification, by Tim Bray
        http://www.xml.com/axml/testaxml.htm
 */
/* Layers
   (1) lexing
        - create a sequence of tags, text (incl cdata), comments etc
   (2) parsing occur in phases:
       (a) character set validation
       (b) namespace resolution
       (c) parse-state tree navigation, and returning nodes
*/

/* forward definitions for lexing (stage 1) */
typedef enum
{
  LEX_DEFAULT,                  /* the usual state parsing text */
  LEX_DEFAULT_ENTITY_REF,      /* the usual state parsing text */
  LEX_LT,                       /* just got an "<" */
  LEX_OPEN_ELEMENT_NAME,        /* in element-name of open tag */
  LEX_OPEN_IN_ATTRS,            /* waiting for attribute-name or ">" */
  LEX_OPEN_IN_ATTR_NAME,        /* in attribute name */
  LEX_OPEN_AFTER_ATTR_NAME,     /* after attribute name */
  LEX_OPEN_AFTER_ATTR_NAME_EQ,  /* after attribute name= */
  LEX_OPEN_IN_ATTR_VALUE_SQ,    /* in single-quoted attribute name */
  LEX_OPEN_IN_ATTR_VALUE_DQ,    /* in double-quoted attribute name */
  LEX_OPEN_IN_ATTR_VALUE_SQ_ENTITY_REF, /* in attribute-value, having      */
  LEX_OPEN_IN_ATTR_VALUE_DQ_ENTITY_REF, /* ...an "&" waiting for semicolon */
  LEX_LT_SLASH,                 /* just got an "</" */
  LEX_CLOSE_ELEMENT_NAME,       /* in element-name of close tag */
  LEX_AFTER_CLOSE_ELEMENT_NAME,       /* after element-name of close tag */
  LEX_OPEN_CLOSE,               /* got a slash after an open tag */
  LEX_LT_BANG,
  LEX_LT_BANG_MINUS,            /* <!- */
  LEX_COMMENT,                  /* <!-- */
  LEX_COMMENT_MINUS,            /* <!-- comment - */
  LEX_COMMENT_MINUS_MINUS,      /* <!-- comment -- */
  LEX_LT_BANG_LBRACK,
  LEX_LT_BANG_LBRACK_IN_CDATAHDR,  /* <![CD   (for example, nchar of "CDATA" given by ??? */
  LEX_LT_BANG_LBRACK_CDATAHDR,  /* <![CD   (for example, nchar of "CDATA" given by ??? */
  LEX_CDATA,
  LEX_CDATA_RBRACK,
  LEX_CDATA_RBRACK_RBRACK,
  LEX_PROCESSING_INSTRUCTION,
  LEX_PROCESSING_INSTRUCTION_QM,        /* after question mark in PI */
  LEX_BANG_DIRECTIVE     /* this encompasses ELEMENT, ATTLIST, ENTITY, DOCTYPE declarations */
} LexState;
static const char *
lex_state_description (LexState state)
{
  switch (state)
    {
    case LEX_DEFAULT:                           return "the usual state parsing text";
    case LEX_DEFAULT_ENTITY_REF:                return "the usual state parsing text in entity";
    case LEX_LT:                                return "after '<'";
    case LEX_OPEN_ELEMENT_NAME:                 return "in element-name of open tag";
    case LEX_OPEN_IN_ATTRS:                     return "waiting for attribute-name or '>'";
    case LEX_OPEN_IN_ATTR_NAME:
    case LEX_OPEN_AFTER_ATTR_NAME:
    case LEX_OPEN_AFTER_ATTR_NAME_EQ:
    case LEX_OPEN_IN_ATTR_VALUE_SQ:
    case LEX_OPEN_IN_ATTR_VALUE_DQ:
    case LEX_OPEN_IN_ATTR_VALUE_SQ_ENTITY_REF:
    case LEX_OPEN_IN_ATTR_VALUE_DQ_ENTITY_REF:  return "in attributes";
    case LEX_LT_SLASH:                          return "after '</'";
    case LEX_CLOSE_ELEMENT_NAME:                return "in element-name of close tag";
    case LEX_AFTER_CLOSE_ELEMENT_NAME:          return "after element-name of close tag";
    case LEX_OPEN_CLOSE:                        return "got a slash after an open tag";
    case LEX_LT_BANG:
    case LEX_LT_BANG_MINUS:
    case LEX_COMMENT_MINUS:
    case LEX_COMMENT_MINUS_MINUS:
    case LEX_COMMENT:                           return "in comment";
    case LEX_LT_BANG_LBRACK:
    case LEX_LT_BANG_LBRACK_IN_CDATAHDR:
    case LEX_LT_BANG_LBRACK_CDATAHDR:           return "beginning cdata block";
    case LEX_CDATA:
    case LEX_CDATA_RBRACK:
    case LEX_CDATA_RBRACK_RBRACK:               return "in CDATA";
    case LEX_PROCESSING_INSTRUCTION:
    case LEX_PROCESSING_INSTRUCTION_QM:         return "in processing-instruction";
    case LEX_BANG_DIRECTIVE:                    return "various directives";
    }
  return NULL;
}

#define WHITESPACE_CASES  case ' ': case '\t': case '\r': case '\n'

/* --- configuration --- */
typedef struct _ParseStateTransition ParseStateTransition;
typedef struct _ParseState ParseState;
struct _ParseStateTransition
{
  char *str;                    /* empty string for default NS */
  ParseState *state;
};
struct _ParseState
{
  unsigned n_ret;
  unsigned *ret_indices;

  unsigned n_transitions;
  ParseStateTransition *transitions;

  ParseState *wildcard_transition;
};

struct _RoscoXmlParserConfig
{
  /* sorted by url */
  unsigned n_ns;
  RoscoXmlParserNamespaceConfig *ns;

  ParseState base;

  unsigned ref_count;

  unsigned ignore_ns : 1;
  unsigned suppress_whitespace : 1;
  unsigned include_comments : 1;
  unsigned passthrough_bad_ns_prefixes : 1;
  unsigned destroyed : 1;
};

/* --- parser --- */
typedef struct _NsAbbrevMap NsAbbrevMap;
struct _NsAbbrevMap
{
  char *abbrev;         /* used in source doc; NULL for default ns */
  RoscoXmlParserNamespaceConfig *translate;

  /* containing namespace abbreviation (that we are overriding);
     so called b/c this abbreviation is masking the one outside
     our XML element */
  NsAbbrevMap *masking;

  /* list of namespace abbreviations defined at a single element
     (owned by the stack-node) */
  NsAbbrevMap *defined_list_next;

  /* rbtree by name */
  NsAbbrevMap *parent, *left, *right;
  unsigned is_red : 1;

  /* are the source-doc abbreviation and the config abbreviation equal? */
  unsigned is_nop : 1;
};

typedef struct _StackNode StackNode;
struct _StackNode
{
  unsigned name_kv_space;
  unsigned n_attrs;
  ParseState *state;
  NsAbbrevMap *defined_list;
  unsigned char needed;
  unsigned char condense_text;
  unsigned n_children;          /* children should only be added if 'needed' */
};

#define MAX_ENTITY_REF_LENGTH  16

typedef struct _ResultQueueNode ResultQueueNode;
struct _ResultQueueNode
{
  unsigned index;
  RoscoXml *xml;
  ResultQueueNode *next;
};

/* simple-buffer: a trivial one-slab buffer */
typedef struct _SimpleBuffer SimpleBuffer;
struct _SimpleBuffer
{
  uint8_t *data;
  unsigned length, alloced;
};
struct _RoscoXmlParser
{
  RoscoXmlFilename *filename;
  unsigned line_no;

  /* for text, comments, etc */
  SimpleBuffer buffer;

  /* line-no when buffer was started */
  unsigned start_line;

  /* if parsing an open tag, this is the number of attributes so far */
  unsigned n_attrs;

  NsAbbrevMap *ns_map;
  NsAbbrevMap *default_ns;

  unsigned stack_size;
  StackNode stack[MAX_DEPTH];
  SimpleBuffer stack_tag_strs;          /* names and attrs */
  unsigned n_stack_children;
  RoscoXml **stack_children;
  unsigned stack_children_alloced;

  char entity_buf[MAX_ENTITY_REF_LENGTH];
  unsigned entity_buf_len;

  /* numbers to get the max-space needed for namespace handling */
  unsigned max_ns_expand, max_ns_attr_expand;

  /* queue of completed xml nodes waiting to be popped off */
  ResultQueueNode *first_result, *last_result;

  LexState lex_state;

  RoscoXmlParserConfig *config;
};
#define COMPARE_STR_TO_NS_ABBREV_TREE(a,b,rv) rv = strcmp (a, b->abbrev)
#define COMPARE_NS_ABBREV_TREE_NODES(a,b,rv) rv = strcmp (a->abbrev, b->abbrev)
#define GET_NS_ABBREV_TREE(parser) \
  (parser)->ns_map, NsAbbrevMap *, ROSCO_STD_GET_IS_RED, ROSCO_STD_SET_IS_RED, \
  parent, left, right, COMPARE_NS_ABBREV_TREE_NODES

/* utility: split our xml-path into components, checking for unallowed things. */
static char **
validate_and_split_xpath (const char *xmlpath,
                          RoscoError  **error)
{
  rosco_boolean slash_allowed = ROSCO_FALSE;
  unsigned n_slashes = 0;
  const char *at;
  char **rv;
  unsigned i;
  for (at = xmlpath; *at; at++)
    switch (*at)
      {
      WHITESPACE_CASES:
        rosco_set_error (error, "whitespace not allowed in xmlpath");
        return NULL;
      case '/':
        if (!slash_allowed)
          {
            if (at == xmlpath)
              rosco_set_error (error, "initial '/' in xmlpath not allowed");
            else
              rosco_set_error (error, "two consecutive '/'s in xmlpath not allowed");
            return ROSCO_FALSE;
          }
        n_slashes++;
        slash_allowed = ROSCO_FALSE;
        break;
      default:
        slash_allowed = ROSCO_TRUE;
      }
  if (!slash_allowed && at > xmlpath)
    {
      rosco_set_error (error, "final '/' in xmlpath not allowed");
      return ROSCO_FALSE;
    }
  if (!slash_allowed)
    {
      rv = ROSCO_NEW0 (char *);
      return rv;
    }
  rv = ROSCO_NEW_ARRAY (char *, n_slashes + 2);
  i = 0;
  for (at = xmlpath; at != NULL; )
    {
      const char *slash = strchr (at, '/');
      if (slash)
        {
          rv[i++] = rosco_strdup_slice (at, slash);
          at = slash + 1;
        }
      else
        {
          rv[i++] = rosco_strdup (at);
          at = NULL;
        }
    }
  rv[i] = NULL;
  return rv;
}
static int compare_str_to_namespace_configs (const void *a, const void *b)
{
  const char *A = a;
  const RoscoXmlParserNamespaceConfig *B = b;
  return strcmp (A, B->url);
}
static int compare_namespace_configs (const void *a, const void *b)
{
  const RoscoXmlParserNamespaceConfig *A = a;
  const RoscoXmlParserNamespaceConfig *B = b;
  return strcmp (A->url, B->url);
}

static int compare_str_to_parse_state_transition (const void *a, const void *b)
{
  const char *A = a;
  const ParseStateTransition *B = b;
  return strcmp (A, B->str);
}
static int compare_parse_state_transitions (const void *a, const void *b)
{
  const ParseStateTransition *A = a;
  const ParseStateTransition *B = b;
  return strcmp (A->str, B->str);
}
static int compare_uint (const void *a, const void *b)
{
  const unsigned *A = a;
  const unsigned *B = b;
  return (*A < *B) ? -1 : (*A > *B) ? 1 : 0;
}

static ParseState *
copy_parse_state (ParseState *src)
{
  ParseState *ps = ROSCO_NEW0 (ParseState);
  unsigned i;
  ps->n_transitions = src->n_transitions;
  ps->transitions = ROSCO_NEW_ARRAY (ParseStateTransition, src->n_transitions);
  for (i = 0; i < ps->n_transitions; i++)
    {
      ps->transitions[i].str = rosco_strdup (src->transitions[i].str);
      ps->transitions[i].state = copy_parse_state (src->transitions[i].state);
    }
  ps->n_ret = src->n_ret;
  ps->ret_indices = rosco_memdup (sizeof (unsigned) * src->n_ret, src->ret_indices);
  return ps;
}

/* helper function to add all ret_indices contained in src
   to dst */
static void
union_copy_parse_state (ParseState *dst,
                        ParseState *src)
{
  unsigned i;
  for (i = 0; i < src->n_transitions; i++)
    {
      unsigned j;
      for (j = 0; j < dst->n_transitions; j++)
        if (strcmp (src->transitions[i].str, dst->transitions[j].str) == 0)
          {
            union_copy_parse_state (dst->transitions[j].state, src->transitions[i].state);
            break;
          }
      if (j == dst->n_transitions)
        {
          dst->transitions = rosco_realloc (dst->transitions, sizeof (ParseStateTransition) * (dst->n_transitions + 1));
          dst->transitions[dst->n_transitions].str = rosco_strdup (src->transitions[i].str);
          dst->transitions[dst->n_transitions].state = copy_parse_state (src->transitions[i].state);
          dst->n_transitions++;
        }
    }

  /* add return points */
  if (src->n_ret > 0)
    {
      dst->ret_indices = rosco_realloc (dst->ret_indices, sizeof (unsigned) * (src->n_ret + dst->n_ret));
      memcpy (dst->ret_indices + dst->n_ret, src->ret_indices, src->n_ret * sizeof (unsigned));
      dst->n_ret += src->n_ret;
    }
}

/* Our goal is to integrate pure wildcards into the
 * state-machine.  Assuming there is a wildcard tree:  it is moved
 * into the 'wildcard_transition' member of the ParseState
 * However, it must be replicated in each non-wildcard subtree:
 * we must perform a union with each non-wildcard subtree.
 * 
 * If this state DOES NOT contain a wildcard,
 * just recurse on the transitions.
 */
static void
expand_parse_state_wildcards_recursive (ParseState *state)
{
  unsigned i;
  unsigned wc_trans;
  for (wc_trans = 0; wc_trans < state->n_transitions; wc_trans++)
    if (strcmp (state->transitions[wc_trans].str, "*") == 0)
      break;
  if (wc_trans < state->n_transitions)
    {
      /* We have a "*" node. */

      /* move transition to wildcard transition */
      state->wildcard_transition = state->transitions[wc_trans].state;
      rosco_free (state->transitions[wc_trans].str);

      /* union "*" subtree with each transition's subtree */
      for (i = 0; i < state->n_transitions; i++)
        if (i != wc_trans)
          union_copy_parse_state (state->transitions[i].state, state->wildcard_transition);

      /* remove "*" node from transition list */
      if (wc_trans + 1 < state->n_transitions)
        state->transitions[wc_trans] = state->transitions[state->n_transitions-1];
      if (--state->n_transitions == 0)
        {
          rosco_free (state->transitions);
          state->transitions = NULL;
        }
    }

  qsort (state->transitions, state->n_transitions, sizeof (ParseStateTransition),
         compare_parse_state_transitions);

  /* not really necessary, since we don't care if identical xml nodes are returned
     in any particular order to they various ret-indices.  predicatibility is
     slightly useful though, so may as well. */
  qsort (state->ret_indices, state->n_ret, sizeof (unsigned), compare_uint);

  /* recurse on children */
  for (i = 0; i < state->n_transitions; i++)
    expand_parse_state_wildcards_recursive (state->transitions[i].state);
  if (state->wildcard_transition != NULL)
    expand_parse_state_wildcards_recursive (state->wildcard_transition);
}

#if DEBUG_PARSE_STATE_TREE
static void
dump_parse_state_recursive (ParseState *state,
                            unsigned    depth)
{
  unsigned i;
  if (state->n_ret > 0)
    {
      fprintf(stderr, "%*sreturn", depth*2, "");
      for (i = 0; i < state->n_ret; i++)
        fprintf(stderr, " %u", state->ret_indices[i]);
      fprintf(stderr, "\n");
    }
  for (i = 0; i < state->n_transitions; i++)
    {
      fprintf(stderr, "%*s%s ->\n", depth*2, "", state->transitions[i].str);
      dump_parse_state_recursive (state->transitions[i].state, depth+1);
    }
  if (state->wildcard_transition)
    {
      fprintf(stderr, "%*s* ->\n", depth*2, "");
      dump_parse_state_recursive (state->wildcard_transition, depth+1);
    }
}
#endif
RoscoXmlParserConfig *
rosco_xml_parser_config_new_simple (RoscoXmlParserFlags flags,
                                  const char       *path)
{
  RoscoXmlParserConfig *cfg;
  cfg = rosco_xml_parser_config_new (flags | ROSCO_XML_PARSER_IGNORE_NS,
                                   0, NULL,
                                   1, (char **) (&path),
                                   NULL);
  rosco_assert (cfg);
  return cfg;
}
RoscoXmlParserConfig *
rosco_xml_parser_config_new (RoscoXmlParserFlags flags,
			   unsigned          n_ns,
			   const RoscoXmlParserNamespaceConfig *ns,
			   unsigned          n_xmlpaths,
			   char            **xmlpaths,
                           RoscoError        **error)
{
  /* copy and sort the namespace mapping, if enabled */
  RoscoXmlParserNamespaceConfig *ns_slab = NULL;
  RoscoXmlParserConfig *config;
  unsigned i;
  if ((flags & ROSCO_XML_PARSER_IGNORE_NS) == 0)
    {
      unsigned total_strlen = 0;
      char *at;
      for (i = 0; i < n_ns; i++)
        total_strlen += strlen (ns[i].url) + 1
                      + strlen (ns[i].prefix) + 1;
      ns_slab = rosco_malloc (sizeof (RoscoXmlParserNamespaceConfig) * n_ns
                            + total_strlen);
      at = (char*)(ns_slab + n_ns);
      for (i = 0; i < n_ns; i++)
        {
          ns_slab[i].url = at;
          at = rosco_stpcpy (at, ns[i].url) + 1;
          ns_slab[i].prefix = at;
          at = rosco_stpcpy (at, ns[i].prefix) + 1;
        }
      qsort (ns_slab, n_ns, sizeof (RoscoXmlParserNamespaceConfig),
             compare_namespace_configs);
      for (i = 1; i < n_ns; i++)
        if (strcmp (ns_slab[i-1].url, ns_slab[i].url) == 0)
          {
            rosco_free (ns_slab);
            rosco_warning ("creating parser-config: two mappings for %s",
                         ns_slab[i-1].url);
            return NULL;
          }
    }

  config = ROSCO_NEW0 (RoscoXmlParserConfig);
  config->ref_count = 1;
  config->ignore_ns = (flags & ROSCO_XML_PARSER_IGNORE_NS) ? 1 : 0;
  config->include_comments = (flags & ROSCO_XML_PARSER_INCLUDE_COMMENTS) ? 1 : 0;
  config->passthrough_bad_ns_prefixes = 0;
  config->n_ns = n_ns;
  config->ns = ns_slab;

  /* in phase 1, we pretend '*' is a legitimate normal path component.
     we rework the state machine in phase 2 to fix this. */
  for (i = 0; i < n_xmlpaths; i++)
    {
      char **pieces = validate_and_split_xpath (xmlpaths[i], error);
      ParseState *at;
      unsigned p;
      if (pieces == NULL)
        {
          rosco_xml_parser_config_destroy (config);
          return config;
        }
      at = &config->base;
      for (p = 0; pieces[p] != NULL; p++)
        {
          size_t tmp = at->n_transitions;
          ParseStateTransition *trans = lfind (pieces[p], at->transitions,
                                               &tmp,          /* wtf lsearch()? */
                                               sizeof (ParseStateTransition),
                                               compare_str_to_parse_state_transition);
          if (trans == NULL)
            {
              at->transitions = rosco_realloc (at->transitions, sizeof (ParseStateTransition) * (1+at->n_transitions));
              trans = &at->transitions[at->n_transitions];
              trans->str = pieces[p];
              trans->state = ROSCO_NEW0 (ParseState);
              at->n_transitions += 1;
            }
          else
            rosco_free (pieces[p]);

          at = trans->state;
        }
      rosco_free (pieces);

      /* add to return-list */
      at->ret_indices = rosco_realloc (at->ret_indices, sizeof (unsigned) * (at->n_ret+1));
      at->ret_indices[at->n_ret++] = i;
    }

  /* phase 2, '*' nodes are deleted and the subtree is moved to the wildcard subtree,
     and the subtree is copied into each siblings subtree. */
  expand_parse_state_wildcards_recursive (&config->base);

#if DEBUG_PARSE_STATE_TREE
  /* dump tree */
  dump_parse_state_recursive (&config->base, 0);
#endif
  return config;
}

/* --- SimpleBuffer API --- */
static inline void simple_buffer_init (SimpleBuffer *sb)
{
  sb->length = 0;
  sb->alloced = 128;
  sb->data = rosco_malloc (sb->alloced);
}

static void
_simple_buffer_must_resize (SimpleBuffer *sb, unsigned needed)
{
  unsigned alloced = sb->alloced;
  alloced += alloced;
  while (alloced < needed)
    alloced += alloced;
  sb->data = rosco_realloc (sb->data, alloced);
  sb->alloced = alloced;
}
static inline void
simple_buffer_ensure_end_space (SimpleBuffer *sb, unsigned end_space)
{
  unsigned alloced = sb->alloced;
  unsigned needed = end_space + sb->length;
  if (needed > alloced)
    _simple_buffer_must_resize (sb, needed);
}
static inline uint8_t *
simple_buffer_end_data (SimpleBuffer *sb)
{
  return sb->data + sb->length;
}
static inline void
simple_buffer_append (SimpleBuffer *sb, unsigned length, const void *data)
{
  simple_buffer_ensure_end_space (sb, length);
  memcpy (sb->data + sb->length, data, length);
  sb->length += length;
}
static inline void
simple_buffer_append_byte (SimpleBuffer *sb, uint8_t byte)
{
  if (sb->length == sb->alloced)
    {
      sb->alloced *= 2;
      sb->data = rosco_realloc (sb->data, sb->alloced);
    }
  sb->data[sb->length++] = byte;
}
static inline void
simple_buffer_clear (SimpleBuffer *sb)
{
  rosco_free (sb->data);
}


/* --- character entities --- */

static rosco_boolean
handle_char_entity (RoscoXmlParser *parser,
                    RoscoError    **error)
{
  const char *b = parser->entity_buf;
  switch (parser->entity_buf_len)
    {
    case 0: case 1:
      rosco_set_error (error, "character entity too short (%u bytes) (%s, line %u)", parser->entity_buf_len,
                 parser->filename ? parser->filename->filename : "string",
                 parser->line_no);
      return ROSCO_FALSE;
    case 2:
      switch (b[0])
        {
        case '#':
          goto unicode_by_value;
        case 'l': case 'L':
          if (b[1] == 't' || b[1] == 'T')
            {
              simple_buffer_append_byte (&parser->buffer, '<');
              return ROSCO_TRUE;
            }
          break;
        case 'g': case 'G':
          if (b[1] == 't' || b[1] == 'T')
            {
              simple_buffer_append_byte (&parser->buffer, '>');
              return ROSCO_TRUE;
            }
          break;
        }

    case 3:
      switch (parser->entity_buf[0])
        {
        case '#':
          goto unicode_by_value;
        case 'a': case 'A':
          if ((b[1] == 'm' || b[1] == 'M')
           || (b[2] == 'p' || b[2] == 'P'))
            {
              simple_buffer_append_byte (&parser->buffer, '&');
              return ROSCO_TRUE;
            }
          break;
        }
    case 4:
      switch (parser->entity_buf[0])
        {
        case '#':
          goto unicode_by_value;
        case 'a': case 'A':
          if ((b[1] == 'p' || b[1] == 'P')
           || (b[2] == 'o' || b[2] == 'O')
           || (b[3] == 's' || b[3] == 'S'))
            {
              simple_buffer_append_byte (&parser->buffer, '\'');
              return ROSCO_TRUE;
            }
          break;
        case 'q': case 'Q':
          if ((b[1] == 'u' || b[1] == 'U')
           || (b[2] == 'o' || b[2] == 'O')
           || (b[3] == 't' || b[3] == 'T'))
            {
              simple_buffer_append_byte (&parser->buffer, '"');
              return ROSCO_TRUE;
            }
          break;
        }
      break;
    default:
      if (b[0] == '#')
        goto unicode_by_value;
      break;
    }
  rosco_set_error (error, "unknown character entity (&%.*s;)",
                 (int) parser->entity_buf_len, parser->entity_buf);
  return ROSCO_FALSE;

unicode_by_value:
  {
    unsigned unicode = 0;
    if (b[1] == 'x')
      {
        /* parse hex value */
        unsigned i;
        for (i = 2; i < parser->entity_buf_len; i++)
          {
            int v = rosco_ascii_xdigit_value (b[i]);
            if (v < 0)
              goto bad_number;
            unicode <<= 4;
            unicode += v;
          }
      }
    else if (!rosco_ascii_isdigit (b[1]))
      goto bad_number;
    else
      {
        unsigned i;
        for (i = 1; i < parser->entity_buf_len; i++)
          {
            int v = rosco_ascii_digit_value (b[i]);
            if (v < 0)
              goto bad_number;
            unicode *= 10;
            unicode += v;
          }
      }
    char utf8[16];
    unsigned utf8_len = rosco_utf8_encode_unichar (utf8, unicode);
    simple_buffer_append (&parser->buffer, utf8_len, utf8);
    return ROSCO_TRUE;
  }

bad_number:
  rosco_set_error (error, "bad numeric character entity &%.*s;",
                 (int) parser->entity_buf_len, parser->entity_buf);
  return ROSCO_FALSE;
}

/* --- handling open/close tags --- */

/* utf8_state:
     0: default
     1: 2-byte codes
     2,3: 3-byte codes
     4,5,6: 4-byte codes
     7: error
 */

static rosco_boolean is_valid_ascii_char (unsigned c, rosco_boolean strict)
{
  if (c == 32 && !strict)
    return ROSCO_TRUE;
  return c > 32;
}
static rosco_boolean is_valid_unichar2 (unsigned c, rosco_boolean strict)
{
  ROSCO_UNUSED (strict);
  ROSCO_UNUSED (c);
  return ROSCO_TRUE;
}
static rosco_boolean is_valid_unichar3 (unsigned c, rosco_boolean strict)
{
  ROSCO_UNUSED (strict);
  if (! (c <= 0xd7ff || (0xe000 <= c && c <= 0xfffd)
        || (c > 0x10000)))
    return ROSCO_FALSE;
  return ROSCO_TRUE;
}
static rosco_boolean is_valid_unichar4 (unsigned c, rosco_boolean strict)
{
  ROSCO_UNUSED (strict);
  return c <= 0x10ffff;
}

static rosco_boolean utf_validate_open_element (RoscoXmlParser *parser,
                                              char        **attrs_out,
                                              RoscoError    **error)
{
  /* do UTF-8 validation (and other checks) */
  //unsigned utf8_state = 0;
  unsigned cur;
  unsigned line_offset = 0;
  rosco_boolean is_strict = ROSCO_TRUE;
  char *at = (char*)(parser->buffer.data);
  char *end = at + parser->buffer.length;
  unsigned attr_index = 0;
  while (at < end)
    {
      if ((*at & 0x80) == 0)
        {
          if (*at == 0)
            {
              attrs_out[attr_index++] = at + 1;

              /* even elements are all strict */
              is_strict = (attr_index & 1);
            }
          else
            {
              if (!is_valid_ascii_char (*at, is_strict))
                goto bad_character;
              if (*at == '\n')
                line_offset++;
            }
          at++;
        }
      else if ((*at & 0xe0) == 0xc0)
        {
          unsigned cur;
          //utf8_state = 1;
          if ((at[1] & 0xc0) != 0x80)
            goto bad_utf8;
          cur = ((at[0] & 0x1f) << 6) | (at[1] & 0x3f);
          if (cur < 0x80)
            goto bad_utf8;
          if (!is_valid_unichar2 (cur, is_strict))
            goto bad_character;
          at += 2;
        }
      else if ((*at & 0xf0) == 0xe0)
        {
          if ((at[1] & 0xc0) != 0x80 || (at[2] & 0xc0) != 0x80)
            goto bad_utf8;
          cur = ((at[0] & 0xf) << 12) | ((at[1] & 0x3f) << 6)
                       | (at[2] & 0x3f);
          if (cur < 0x800)
            goto bad_utf8;
          if (!is_valid_unichar3 (cur, is_strict))
            goto bad_character;
          at += 3;
        }
      else if ((*at & 0xf8) == 0xf0)
        {
          if ((at[1] & 0xc0) != 0x80 || (at[2] & 0xc0) != 0x80
           || (at[3] & 0xc0) != 0x80)
            goto bad_utf8;
          cur = ((at[0] & 0xf) << 18) | ((at[1] & 0x3f) << 12)
                        | ((at[2] & 0x3f) << 6) | (at[3] & 0x3f);
          if (cur < 0x10000)
            goto bad_utf8;
          if (!is_valid_unichar4 (cur, is_strict))
            goto bad_character;
          at += 4;
        }
      else
        goto bad_utf8;
    }

  rosco_assert (attr_index == parser->n_attrs * 2 + 1);
  return ROSCO_TRUE;

bad_utf8:
  /* TODO: this doesn't count whitespace added between the attributes!!!!!! */
  rosco_set_error (error, "bad UTF-8 at %s, line %u, in open tag",
                 parser->filename ? parser->filename->filename : "string",
                 parser->start_line + line_offset);
  return ROSCO_FALSE;

bad_character:
  /* TODO: this doesn't count whitespace added between the attributes!!!!!! */
  rosco_set_error (error, "bad character %s at %s, line %u, in open tag",
                 rosco_ascii_byte_name (*at),
                 parser->filename ? parser->filename->filename : "string",
                 parser->start_line + line_offset);
  return ROSCO_FALSE;

}
static rosco_boolean utf_validate_text (RoscoXmlParser *parser,
                                      RoscoError    **error)
{
  /* do UTF-8 validation (and other checks) */
  unsigned cur;
  unsigned line_offset = 0;
  char *at = (char*)(parser->buffer.data);
  char *end = at + parser->buffer.length;
  while (at < end)
    {
      if ((*at & 0x80) == 0)
        {
          if (*at == 0)
            goto bad_character;
          if (*at == '\n')
            line_offset++;
          at++;
        }
      else if ((*at & 0xe0) == 0xc0)
        {
          unsigned cur;
          if ((at[1] & 0xc0) != 0x80)
            goto bad_utf8;
          cur = ((at[0] & 0x1f) << 6) | (at[1] & 0x3f);
          if (cur < 0x80)
            goto bad_utf8;
          if (!is_valid_unichar2 (cur, ROSCO_FALSE))
            goto bad_character;
          at += 2;
        }
      else if ((*at & 0xf0) == 0xe0)
        {
          if ((at[1] & 0xc0) != 0x80 || (at[2] & 0xc0) != 0x80)
            goto bad_utf8;
          cur = ((at[0] & 0xf) << 12) | ((at[1] & 0x3f) << 6)
                       | (at[2] & 0x3f);
          if (cur < 0x800)
            goto bad_utf8;
          if (!is_valid_unichar3 (cur, ROSCO_FALSE))
            goto bad_character;
          at += 3;
        }
      else if ((*at & 0xf8) == 0xf0)
        {
          if ((at[1] & 0xc0) != 0x80 || (at[2] & 0xc0) != 0x80
           || (at[3] & 0xc0) != 0x80)
            goto bad_utf8;
          cur = ((at[0] & 0xf) << 18) | ((at[1] & 0x3f) << 12)
                        | ((at[2] & 0x3f) << 6) | (at[3] & 0x3f);
          if (cur < 0x10000)
            goto bad_utf8;
          if (!is_valid_unichar4 (cur, ROSCO_FALSE))
            goto bad_character;
          at += 4;
        }
      else
        goto bad_utf8;
    }
  return ROSCO_TRUE;

bad_utf8:
  /* TODO: this doesn't count whitespace added between the attributes!!!!!! */
  rosco_set_error (error, "bad UTF-8 at %s, line %u, in text",
                 parser->filename ? parser->filename->filename : "string",
                 parser->start_line + line_offset);
  return ROSCO_FALSE;

bad_character:
  /* TODO: this doesn't count whitespace added between the attributes!!!!!! */
  rosco_set_error (error, "bad character %s at %s, line %u, in text",
                 rosco_ascii_byte_name (*at),
                 parser->filename ? parser->filename->filename : "string",
                 parser->start_line + line_offset);
  return ROSCO_FALSE;
}

static RoscoXmlParserNamespaceConfig *
lookup_xmlns_translation (const char   *url,
                          RoscoXmlParser *parser,
                          RoscoError    **error)
{
  RoscoXmlParserNamespaceConfig *rv;
  rv = bsearch ((void*)url, parser->config->ns, parser->config->n_ns, sizeof (RoscoXmlParserNamespaceConfig),
                compare_str_to_namespace_configs);
  if (rv == NULL)
    rosco_set_error (error, "unhandled namespace URL '%s' encountered", url);
  return rv;
}


static void
add_to_child_stack__take (RoscoXmlParser *parser,
                          RoscoXml *node,
                          unsigned frame)
{
  StackNode *stack = parser->stack + frame;
  stack->n_children += 1;
  if (parser->n_stack_children == parser->stack_children_alloced)
    {
      parser->stack_children_alloced *= 2;
      parser->stack_children = rosco_realloc (parser->stack_children,
                                            sizeof(RoscoXml*) *
                                            parser->stack_children_alloced);
    }
  parser->stack_children[parser->n_stack_children++] = node;
}

#if 0
static rosco_boolean
has_ns_prefix (RoscoXmlParser *parser,
               char         *attr_name,
               unsigned   *prefix_len_out,
               RoscoXmlParserNamespaceConfig **trans_out)
{
  /* search for ':' in attr */
  unsigned offset = attr->frag_offset;
  RoscoBufferFragment *fragment = attr->fragment;
  unsigned length = 0;
  while (fragment)
    {
      uint8_t *start = fragment->buf + fragment->buf_start;
      uint8_t *at = start + offset;
      uint8_t *end = start + fragment->buf_length;
      while (at < end)
        {
          if (*at == ':')
            {
              char *pref = alloca (length + 1);
              NsAbbrevMap *abbrev;
              rosco_buffer_fragment_peek (attr->fragment, attr->frag_offset, length, pref);
              pref[length] = 0;
              if (abbrev == NULL)
                *trans_out = NULL;
              else
                *trans_out = abbrev->translate;

              end_prefix_out->length = attr->length - length - 1;
              end_prefix_out->fragment = fragment;
              *prefix_len_out = length;

              return ROSCO_TRUE;
            }
          else if (*at == 0)
            return ROSCO_FALSE;
          at++;
          length++;
        }
      fragment = fragment->next;
      offset = 0;
    }
  rosco_return_val_if_reached (NULL, ROSCO_FALSE);
}

static char *
buffer_fragment_get_string (RoscoBufferFragment *fragment,
                            unsigned           frag_offset,
                            unsigned           length)
{
  char *rv = rosco_malloc (length + 1);
  rosco_buffer_fragment_peek (fragment, frag_offset, length, rv);
  rv[length] = 0;
  return rv;
}
#endif

static rosco_boolean handle_open_element (RoscoXmlParser *parser,
                                        RoscoError    **error)
{
  ParseState *cur_state = parser->stack[parser->stack_size-1].state;
  ParseState *new_state;
  unsigned n_attrs = parser->n_attrs;
  unsigned str_size; /* space used on the "stack_tag_strs" buffer */
  /* there's 2*n_attrs strings for the attributes
     and 1 additional sentinel marking the end of the array */
  char **attrs = alloca (sizeof (char*) * (n_attrs * 2 + 1));
  NsAbbrevMap *defined_list = NULL;      /* namespace defs in this element */
  unsigned max_space_needed;
  StackNode *st;

  /* perform UTF-8 and character validation;
     find attribute locations. */
  if (!utf_validate_open_element (parser, attrs, error))
    return ROSCO_FALSE;


  /* do xmlns namespace translation (unless suppressed) */
  if (!parser->config->ignore_ns)
    {
      unsigned i;
      RoscoXmlParserNamespaceConfig **xlats = alloca (sizeof(void*) * n_attrs);
      char *attr_slab, *attr_slab_at;
      char *colon;
      unsigned n_attrs_out;
      for (i = 0; i < n_attrs; i++)
        {
          char *name = attrs[2*i];
          if (memcmp (name, "xmlns", 5) == 0
           && (name[5] == 0 || name[5] == ':'))
            {
              /* get url and see if we have a translation for it. */
              xlats[i] = lookup_xmlns_translation (attrs[2*i+1], parser, error);
              if (xlats[i] == NULL)
                {
                  rosco_add_error_prefix (error, "at %s, line %u",
                                        parser->filename ? parser->filename->filename : "string",
                                        parser->start_line);
                  return ROSCO_FALSE;
                }
              if (name[5] == 0)
                {
                  unsigned expand = strlen (xlats[i]->prefix) + 1;
                  if (expand > parser->max_ns_expand)
                    parser->max_ns_expand = expand;
                }
              else
                {
                  /* prefix + colon */
                  unsigned new_len = strlen (xlats[i]->prefix) + 1;

                  /* also includes colon */
                  unsigned old_len = attrs[2*i+1] - attrs[2*i+0] - 6;

                  if (new_len > old_len)
                    {
                      unsigned expand = new_len - old_len;
                      if (expand > parser->max_ns_expand)
                        parser->max_ns_expand = expand;
                      if (expand > parser->max_ns_attr_expand)
                        parser->max_ns_attr_expand = expand;
                    }
                }
            }
          else
            xlats[i] = NULL;
        }
      for (i = 0; i < n_attrs; i++)
        if (xlats[i] != NULL)
          {
            if (attrs[2*i][5] == 0)
              {
                /* default ns */
                NsAbbrevMap *map;
                map = ROSCO_NEW (NsAbbrevMap);
                map->abbrev = NULL;
                map->translate = xlats[i];

                /* add map to list for stack-node */
                map->defined_list_next = defined_list;
                defined_list = map;

                /* set/replace default ns */
                map->masking = parser->default_ns;
                parser->default_ns = map;

              }
            else
              {
                /* prefixed-namespace */
                NsAbbrevMap *map;
                NsAbbrevMap *existing;
                char *abbrev_name = attrs[2*i]+6;
                unsigned abbrev_len = attrs[2*i+1] - abbrev_name; /* includes NUL */
                map = rosco_malloc (sizeof (NsAbbrevMap) + abbrev_len);
                memcpy (map + 1, abbrev_name, abbrev_len);
                map->abbrev = (char*)(map+1);
                map->translate = xlats[i];

                /* add map to list for stack-node */
                map->defined_list_next = defined_list;
                defined_list = map;

                /* insert/replace tree node */
                ROSCO_RBTREE_INSERT (GET_NS_ABBREV_TREE (parser), map, existing);
                map->masking = existing;
                if (existing != NULL)
                  ROSCO_RBTREE_REPLACE_NODE (GET_NS_ABBREV_TREE (parser), existing, map);
              }
          }

      /* upperbound on space required for name and attrs */
      max_space_needed = parser->buffer.length
                       + parser->max_ns_expand
                       + n_attrs * parser->max_ns_attr_expand;

      /* ensure stack_tag_strs can accomodate it */
      simple_buffer_ensure_end_space (&parser->stack_tag_strs, max_space_needed);
      attr_slab = (char*) parser->stack_tag_strs.data
                + parser->stack_tag_strs.length;
      attr_slab_at = attr_slab;

      /* get space/new-prefix for element name using namespace config */
      
      colon = strchr ((char*)parser->buffer.data, ':');
      if (colon != NULL)
        {
          NsAbbrevMap *abbrev;
          const char *name_prefix;
          *colon = 0;
          ROSCO_RBTREE_LOOKUP_COMPARATOR (GET_NS_ABBREV_TREE (parser), (char*)parser->buffer.data, COMPARE_STR_TO_NS_ABBREV_TREE, abbrev);
          *colon = ':';
          name_prefix = abbrev->translate->prefix;
          if (name_prefix[0] != 0)
            {
              attr_slab_at = rosco_stpcpy (attr_slab_at, name_prefix);
              attr_slab_at = rosco_stpcpy (attr_slab_at, colon);
            }
        }
      else if (parser->default_ns != NULL
            && parser->default_ns->translate->prefix[0] != 0)
        {
          attr_slab_at = rosco_stpcpy (attr_slab_at, parser->default_ns->translate->prefix);
          *attr_slab_at++ = ':';
          attr_slab_at = rosco_stpcpy (attr_slab_at, (char*)parser->buffer.data);
        }
      else
        {
          attr_slab_at = rosco_stpcpy (attr_slab_at, (char*)parser->buffer.data);
        }
      *attr_slab_at++ = 0;

      /* rewrite attribute names to use namespace config */
      n_attrs_out = 0;
      for (i = 0; i < n_attrs; i++)
        {
          /* only passthough non-xmlns attributes */
          if (xlats[i] == NULL)
            {
              /* after dealing with the namespace, we copy the remainder of
                 the attribute name (it may be the whole name) and the value,
                 which are contiguous in the buffer.  */
              char *start_copy = attrs[2*i];
              char *end_copy = attrs[2*i+2];
              char *name;

              n_attrs_out++;

              /* is there a namespace prefix on this attr? */
              name = attrs[2*i];
              if ((colon=strchr (name, ':')) != NULL)
                {
                  NsAbbrevMap *abbrev;
                  char *prefix = name;
                  *colon = 0;
                  ROSCO_RBTREE_LOOKUP_COMPARATOR (GET_NS_ABBREV_TREE (parser), prefix, COMPARE_STR_TO_NS_ABBREV_TREE, abbrev);
                  *colon = ':';
                  if (abbrev == NULL)
                    {
                      if (!parser->config->passthrough_bad_ns_prefixes)
                        {
                          rosco_set_error (error,
                                         "bad namespace prefix for '%s' at %s, line %u",
                                         name,
                                         parser->filename ? parser->filename->filename : "string",
                                         parser->start_line);
                          rosco_free (prefix);
                          return ROSCO_FALSE;
                        }
                    }
                  else
                    {
                      /* stash away/mod data */
                      attr_slab_at = rosco_stpcpy (attr_slab_at, abbrev->translate->prefix);
                      start_copy = colon;
                      memcpy (attr_slab_at, colon, attrs[2*i+2] - colon);
                      attr_slab_at += (attrs[2*i+2] - colon);
                    }
                }
              memcpy (attr_slab_at, start_copy, end_copy - start_copy);
              attr_slab_at += (end_copy - start_copy);
            }
        }
      str_size = attr_slab_at
               - (char*)simple_buffer_end_data (&parser->stack_tag_strs);
      parser->stack_tag_strs.length += str_size;
      n_attrs = n_attrs_out;
    }
  else
    {
      str_size = parser->buffer.length;
      simple_buffer_append (&parser->stack_tag_strs,
                            parser->buffer.length,
                            parser->buffer.data);
    }

  /* Find the next state to transition to. */
  if (cur_state == NULL)
    {
      new_state = NULL;
    }
  else
    {
      char *name = (char*)simple_buffer_end_data(&parser->stack_tag_strs)
                 - str_size;
      ParseStateTransition *trans = bsearch (name, cur_state->transitions, cur_state->n_transitions, sizeof (ParseStateTransition),
                                             compare_str_to_parse_state_transition);
      if (trans != NULL)
        new_state = trans->state;
      else
        new_state = cur_state->wildcard_transition;
    }

  /* push entry onto stack */
  if (parser->stack_size == MAX_DEPTH)
    {
      rosco_set_error (error, "tag stack too deep");
      return ROSCO_FALSE;
    }
  st = &parser->stack[parser->stack_size++];
  st->name_kv_space = str_size;
  st->n_attrs = n_attrs;
  st->state = new_state;
  st->defined_list = defined_list;
  st->n_children = 0;
  st->needed = (st-1)->needed || (new_state != NULL && new_state->n_ret != 0);
  st->condense_text = 0;

  /* clear buffer */
  parser->buffer.length = 0;

  return ROSCO_TRUE;
}

static rosco_boolean handle_close_element (RoscoXmlParser *parser,
                                        RoscoError    **error)
{
  StackNode *stack = parser->stack + (parser->stack_size-1);
  char *end_of_name;
  if (parser->stack_size == 1)
    {
      rosco_set_error (error, "close tag with no open-tag (in outermost context), %s, line %u",
                     parser->filename ? parser->filename->filename : "string",
                     parser->line_no);
      return ROSCO_FALSE;
    }
  /* do UTF-8 validation (and other checks) */
  parser->n_attrs = 0;
  if (!utf_validate_open_element (parser, &end_of_name, error))
    return ROSCO_FALSE;

  /* do xmlns namespace translation (unless suppressed) */
  {
    char *orig = (char*) parser->buffer.data;
    char *colon = strchr (orig, ':');
    char *prefix = NULL;
    char *suffix = orig;
    char *stack_end;
    char *open_name_attrs;
    if (colon == NULL)
      {
        if (parser->default_ns)
          prefix = parser->default_ns->translate->prefix;
      }
    else
      {
        NsAbbrevMap *abbrev;
        *colon = 0;
        ROSCO_RBTREE_LOOKUP_COMPARATOR (GET_NS_ABBREV_TREE (parser), orig, COMPARE_STR_TO_NS_ABBREV_TREE, abbrev);
        if (abbrev)
          prefix = abbrev->translate->prefix;
        suffix = colon + 1;
      }
    if (prefix && prefix[0] == 0)
      prefix = NULL;

    /* check that prefix+suffix matches the top of the stack */
    stack_end = (char*) simple_buffer_end_data (&parser->stack_tag_strs);
    open_name_attrs = stack_end - stack->name_kv_space;
    if (prefix == NULL)
      {
        if (strcmp (suffix, open_name_attrs) != 0)
          {
            rosco_set_error (error, "close tag mismatch (<%s> versus </%s>, %s, line %u",
                           open_name_attrs, suffix,
                           parser->filename ? parser->filename->filename : "string",
                           parser->line_no);
            return ROSCO_FALSE;
          }
      }
    else
      {
        char *colon = strchr (open_name_attrs, ':');
        rosco_boolean bad;
        if (colon == NULL)
          bad = ROSCO_TRUE;
        else
          {
            *colon = 0;
            bad = strcmp (prefix, open_name_attrs) != 0
               || strcmp (suffix, colon + 1) != 0;
            *colon = ':';
          }
        if (bad)
          {
            rosco_set_error (error, "close tag mismatch (<%s> versus </%s:%s>, %s, line %u",
                           open_name_attrs, prefix, suffix,
                           parser->filename ? parser->filename->filename : "string",
                           parser->line_no);
            return ROSCO_FALSE;
          }
      }
 

  if (stack->needed)
    {
      /* construct xml node / remove children from stack */
      unsigned n_children = stack->n_children;
      RoscoXml **children = parser->stack_children
                        + parser->n_stack_children
                        - n_children;
      RoscoXml *node;
      unsigned i;
      node = _rosco_xml_new_elt_parse (stack->n_attrs,
                                     stack->name_kv_space,
                                     open_name_attrs,
                                     n_children, children,
                                     stack->condense_text);
      if (parser->filename)
        _rosco_xml_set_position (node, parser->filename, parser->line_no);
      parser->n_stack_children -= n_children;

      /* push results on queue */
      if (stack->state != NULL)
        for (i = 0; i < stack->state->n_ret; i++)
          {
            ResultQueueNode *result = ROSCO_NEW (ResultQueueNode);
            result->index = stack->state->ret_indices[i];
            result->xml = rosco_xml_ref (node);
            result->next = NULL;
            if (parser->last_result == NULL)
              parser->first_result = result;
            else
              parser->last_result->next = result;
            parser->last_result = result;
          }

      /* add to parent's children array if useful */
      if ((stack-1)->needed)
        {
          add_to_child_stack__take (parser, node, parser->stack_size-2);
        }
      else
        rosco_xml_unref (node);
    }
  else
    rosco_assert (stack->n_children == 0);

  /* remove any xmlns definitions */
  while (stack->defined_list != NULL)
    {
      NsAbbrevMap *kill = stack->defined_list;
      stack->defined_list = kill->defined_list_next;

      if (kill->abbrev == NULL)
        {
          parser->default_ns = kill->masking;
        }
      else
        {
          if (kill->masking)
            ROSCO_RBTREE_REPLACE_NODE (GET_NS_ABBREV_TREE (parser),
                                     kill, kill->masking);
          else
            ROSCO_RBTREE_REMOVE (GET_NS_ABBREV_TREE (parser), kill);
        }
      rosco_free (kill);
    }
  }

  /* pop the stack */
  parser->stack_tag_strs.length -= stack->name_kv_space;
  parser->stack_size--;
  
  /* note: child xml nodes were already popped off the stack */

  /* clear buffer */
  parser->buffer.length = 0;

  return ROSCO_TRUE;
}

static rosco_boolean handle_empty_element (RoscoXmlParser *parser,
                                         RoscoError    **error)
{
  unsigned name_len = strlen ((char*)parser->buffer.data);
  if (!handle_open_element (parser, error))
    return ROSCO_FALSE;

  /* this takes advantage of the fact that the buffer is not overwritten
     but merely zeroed out! */
  parser->buffer.length = name_len + 1;

  if (!handle_close_element (parser, error))
    return ROSCO_FALSE;
  
  return ROSCO_TRUE;
}


/* only called if we need to handle the text node (ie its in a xml element
   we are going to return) */
static rosco_boolean        handle_text_node          (RoscoXmlParser *parser,
                                                     RoscoError    **error)
{
  RoscoXml *node;
  /* UTF-8 validate buffer */
  if (!utf_validate_text (parser, error))
    return ROSCO_FALSE;

  node = rosco_xml_text_new_len (parser->buffer.length, (char*)parser->buffer.data);
  if (parser->filename)
    _rosco_xml_set_position (node, parser->filename, parser->start_line);

  /* add to children stack */
  add_to_child_stack__take (parser, node, parser->stack_size-1);
  parser->buffer.length = 0;
  return ROSCO_TRUE;
}

/* only called if we need to handle the comment (ie its in a xml element
   we are going to return, and the user expresses interest in the comments) */
static rosco_boolean     handle_comment            (RoscoXmlParser *parser,
                                                  RoscoError    **error)
{
  RoscoXml *node;
  /* UTF-8 validate buffer */
  if (!utf_validate_text (parser, error))
    return ROSCO_FALSE;

  node = rosco_xml_comment_new_len (parser->buffer.length, (char*)parser->buffer.data);
  if (parser->filename)
    _rosco_xml_set_position (node, parser->filename, parser->start_line);

  /* add to children stack */
  add_to_child_stack__take (parser, node, parser->stack_size-1);
  return ROSCO_TRUE;
}

typedef enum
{
  TRY_DIRECTIVE__NEEDS_MORE,
  TRY_DIRECTIVE__SUCCESS,
  TRY_DIRECTIVE__ERROR
} TryDirectiveResult;
static TryDirectiveResult try_directive (RoscoXmlParser *parser,
                                         RoscoError    **error)
{
  /* FIXME: this does no validation at all of directives. */
  /* FIXME: we actually want to handle internal and external entities. */
  /* FIXME: we should probably handle conditionals??? */
  /* FIXME: this wont work right for directives that are nested... */
  ROSCO_UNUSED (parser);
  ROSCO_UNUSED (error);
  return TRY_DIRECTIVE__SUCCESS;
}
/* --- lexing --- */
static unsigned count_newlines (unsigned length, const uint8_t *data)
{
  unsigned rv = 0;
  while (length--)
    if (*data++ == '\n')
      rv++;
  return rv;
}
rosco_boolean
rosco_xml_parser_feed(RoscoXmlParser       *parser,
                    unsigned            length,
                    const uint8_t      *data,
                    RoscoError          **error)
{
  //rosco_boolean suppress;

  //rosco_warning ("rosco_xml_parser_feed: '%.*s'", length,data);

#define DEBUG_ADVANCE           //rosco_warning ("rosco_xml_parser_feed: ADVANCE %s [%p] [%s]", rosco_ascii_byte_name (*data),data,lex_state_description(parser->lex_state))

#define BUFFER_CLEAR            parser->buffer.length = 0
#define APPEND_BYTE(val)        simple_buffer_append_byte (&parser->buffer, (val))
#define MAYBE_RETURN            do{if(length == 0) return ROSCO_TRUE;}while(0)
#define ADVANCE_NON_NL          do{DEBUG_ADVANCE; length--; data++;}while(0)
#define ADVANCE_CHAR            do{DEBUG_ADVANCE; if (*data == '\n')parser->line_no++; length--; data++;}while(0)
#define ADVANCE_NL              do{DEBUG_ADVANCE; parser->line_no++; length--; data++;}while(0)
#define CONSUME_CHAR_AND_SWITCH_STATE(STATE) do{ parser->lex_state = STATE; ADVANCE_CHAR; MAYBE_RETURN; goto label__##STATE; }while(0)
#define CONSUME_NL_AND_SWITCH_STATE(STATE) do{ parser->lex_state = STATE; ADVANCE_NL; MAYBE_RETURN; goto label__##STATE; }while(0)
#define CONSUME_NON_NL_AND_SWITCH_STATE(STATE) do{ parser->lex_state = STATE; ADVANCE_NON_NL; MAYBE_RETURN; goto label__##STATE; }while(0)
#define IS_SUPPRESSED           (!parser->stack[parser->stack_size-1].needed)
#define CUT_TO_BUFFER           do {if (!IS_SUPPRESSED && start < data) simple_buffer_append (&parser->buffer, data-start, start); }while(0)
#define CHECK_ENTITY_TOO_LONG(newlen)                                      \
        do { if (newlen > MAX_ENTITY_REF_LENGTH)                           \
          {                                                                \
            memcpy (parser->entity_buf + parser->entity_buf_len,           \
                    data,                                                  \
                    MAX_ENTITY_REF_LENGTH - parser->entity_buf_len);       \
            goto entity_ref_too_long;                                      \
          } } while (0)
  //suppress = parser->n_to_be_returned == 0
         //&& (parser->stack_size == 0 || parser->stack[parser->stack_size-1].state == NULL);
  MAYBE_RETURN;
  switch (parser->lex_state)
    {
    case LEX_DEFAULT:
    label__LEX_DEFAULT:
      {
        const uint8_t *start = data;
      continue_default:
        switch (*data)
          {
          case '<':
              if (!IS_SUPPRESSED)
                {
                  CUT_TO_BUFFER;
                  if (parser->buffer.length > 0)
                    {
                      if (!handle_text_node (parser, error))
                        return ROSCO_FALSE;
                    }
                }
              else
                parser->buffer.length = 0;
            CONSUME_NON_NL_AND_SWITCH_STATE (LEX_LT);
          case '&':
            CUT_TO_BUFFER;
            parser->entity_buf_len = 0;
            CONSUME_NON_NL_AND_SWITCH_STATE (LEX_DEFAULT_ENTITY_REF);
          default:
            ADVANCE_CHAR;
            if (length == 0)
              {
                CUT_TO_BUFFER;
                return ROSCO_TRUE;
              }
            goto continue_default;
          }
      }

    case LEX_DEFAULT_ENTITY_REF:
    label__LEX_DEFAULT_ENTITY_REF:
      {
        const uint8_t *semicolon = memchr (data, ';', length);
        unsigned app_len;
        unsigned new_elen;
        if (semicolon == NULL)
          app_len = length;
        else
          app_len = semicolon - data;
        new_elen = parser->entity_buf_len + app_len;
        CHECK_ENTITY_TOO_LONG(new_elen);
        if (IS_SUPPRESSED)
          {
            if (semicolon == NULL)
              {
                parser->entity_buf_len = new_elen;
                return ROSCO_TRUE;
              }
          }
        else
          {
            memcpy (parser->entity_buf + parser->entity_buf_len, data, app_len);
            parser->entity_buf_len = new_elen;
            if (semicolon == NULL)
              return ROSCO_TRUE;
            if (!handle_char_entity (parser, error))
              return ROSCO_FALSE;
          }
        length -= (semicolon + 1 - data);
        data = semicolon + 1;
        parser->lex_state = LEX_DEFAULT;
        MAYBE_RETURN;
        goto label__LEX_DEFAULT;
      }
    case LEX_LT:
    label__LEX_LT:
      switch (*data)
        {
        case '!': CONSUME_CHAR_AND_SWITCH_STATE (LEX_LT_BANG);
        case '/': CONSUME_CHAR_AND_SWITCH_STATE (LEX_LT_SLASH);
        case '?':
                  parser->stack[parser->stack_size-1].condense_text = 1;
                  CONSUME_CHAR_AND_SWITCH_STATE (LEX_PROCESSING_INSTRUCTION);
        WHITESPACE_CASES: 
          ADVANCE_CHAR;
          MAYBE_RETURN;
          goto label__LEX_LT;
        default:
          APPEND_BYTE (*data);
          parser->n_attrs = 0;
          parser->start_line = parser->line_no;
          CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_ELEMENT_NAME);
        }

    case LEX_OPEN_ELEMENT_NAME:
    label__LEX_OPEN_ELEMENT_NAME:
      {
        switch (*data)
          {
          WHITESPACE_CASES:
            APPEND_BYTE (0);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTRS);
          case '>':
            APPEND_BYTE (0);
            if (!handle_open_element (parser, error))
              return ROSCO_FALSE;
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_DEFAULT);
          case '/':
            APPEND_BYTE (0);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_CLOSE);
          case '=':
            goto disallowed_char;
          default:
            APPEND_BYTE (*data);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_ELEMENT_NAME);
          }
      }
    case LEX_OPEN_IN_ATTRS:
    label__LEX_OPEN_IN_ATTRS:
      {
        switch (*data)
          {
          WHITESPACE_CASES:
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTRS);
          case '>':
            if (!handle_open_element (parser, error))
              return ROSCO_FALSE;
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_DEFAULT);
          case '/':
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_CLOSE);
          case '=':
            goto disallowed_char;
          default:
            APPEND_BYTE (*data);
            ++(parser->n_attrs);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTR_NAME);
          }
      }
    case LEX_OPEN_IN_ATTR_NAME:
    label__LEX_OPEN_IN_ATTR_NAME:
      {
        switch (*data)
          {
          WHITESPACE_CASES:
            APPEND_BYTE (0);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_AFTER_ATTR_NAME);
          case '>': case '/':
            goto disallowed_char;
          case '=':
            APPEND_BYTE (0);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_AFTER_ATTR_NAME_EQ);
          default:
            APPEND_BYTE (*data);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTR_NAME);
          }
      }
    case LEX_OPEN_AFTER_ATTR_NAME:
    label__LEX_OPEN_AFTER_ATTR_NAME:
      {
        switch (*data)
          {
          WHITESPACE_CASES:
            ADVANCE_CHAR;
            MAYBE_RETURN;
            goto label__LEX_OPEN_AFTER_ATTR_NAME;
          case '>': case '/':
            goto disallowed_char;
          case '=':
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_AFTER_ATTR_NAME_EQ);
          default:
            goto disallowed_char;
          }
      }
    case LEX_OPEN_AFTER_ATTR_NAME_EQ:
    label__LEX_OPEN_AFTER_ATTR_NAME_EQ:
      {
        switch (*data)
          {
          WHITESPACE_CASES:
            ADVANCE_CHAR;
            MAYBE_RETURN;
            goto label__LEX_OPEN_AFTER_ATTR_NAME;
          case '"':
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTR_VALUE_DQ);
          case '\'':
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTR_VALUE_SQ);
          default:
            goto disallowed_char;
          }
      }
    case LEX_OPEN_IN_ATTR_VALUE_SQ:
    label__LEX_OPEN_IN_ATTR_VALUE_SQ:
      {
        switch (*data)
          {
          case '\'':
            APPEND_BYTE (0);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTRS);
          case '&':
            parser->entity_buf_len = 0;
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTR_VALUE_SQ_ENTITY_REF);
          default:
            APPEND_BYTE (*data);
            ADVANCE_CHAR;
            MAYBE_RETURN;
            goto label__LEX_OPEN_IN_ATTR_VALUE_SQ;
          }
      }
    case LEX_OPEN_IN_ATTR_VALUE_DQ:
    label__LEX_OPEN_IN_ATTR_VALUE_DQ:
      {
        switch (*data)
          {
          case '"':
            APPEND_BYTE (0);
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTRS);
          case '&':
            parser->entity_buf_len = 0;
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_OPEN_IN_ATTR_VALUE_DQ_ENTITY_REF);
          default:
            APPEND_BYTE (*data);
            ADVANCE_CHAR;
            MAYBE_RETURN;
            goto label__LEX_OPEN_IN_ATTR_VALUE_DQ;
          }
      }
    case LEX_OPEN_IN_ATTR_VALUE_SQ_ENTITY_REF:
    case LEX_OPEN_IN_ATTR_VALUE_DQ_ENTITY_REF:
    label__LEX_OPEN_IN_ATTR_VALUE_SQ_ENTITY_REF:
    label__LEX_OPEN_IN_ATTR_VALUE_DQ_ENTITY_REF:
      {
        const uint8_t *semicolon = memchr (data, ';', length);
        unsigned new_elen;
        if (semicolon == NULL)
          new_elen = parser->entity_buf_len + length;
        else
          new_elen = parser->entity_buf_len + (semicolon - data);
        CHECK_ENTITY_TOO_LONG(new_elen);
        if (IS_SUPPRESSED)
          {
            if (semicolon == NULL)
              {
                parser->entity_buf_len = new_elen;
                return ROSCO_TRUE;
              }
          }
        else
          {
            if (semicolon == NULL)
              {
                memcpy (parser->entity_buf + parser->entity_buf_len, data, length);
                parser->entity_buf_len += length;
                return ROSCO_TRUE;
              }
            memcpy (parser->entity_buf + parser->entity_buf_len, data, semicolon - data);
            if (!handle_char_entity (parser, error))
              return ROSCO_FALSE;
          }
        length -= (semicolon + 1 - data);
        data = semicolon + 1;
        if (parser->lex_state == LEX_OPEN_IN_ATTR_VALUE_SQ_ENTITY_REF)
          {
            parser->lex_state = LEX_OPEN_IN_ATTR_VALUE_SQ;
            MAYBE_RETURN;
            goto label__LEX_OPEN_IN_ATTR_VALUE_SQ;
          }
        else
          {
            parser->lex_state = LEX_OPEN_IN_ATTR_VALUE_DQ;
            MAYBE_RETURN;
            goto label__LEX_OPEN_IN_ATTR_VALUE_DQ;
          }
      }
    case LEX_LT_SLASH:
    label__LEX_LT_SLASH:
      switch (*data)
        {
        WHITESPACE_CASES:
          ADVANCE_CHAR;
          MAYBE_RETURN;
          goto label__LEX_LT_SLASH;
        case '<': case '>': case '=':
          goto disallowed_char;
        default:
          APPEND_BYTE (*data);
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_CLOSE_ELEMENT_NAME);
        }

    case LEX_CLOSE_ELEMENT_NAME:
    label__LEX_CLOSE_ELEMENT_NAME:
      switch (*data)
        {
        WHITESPACE_CASES:
          CONSUME_CHAR_AND_SWITCH_STATE (LEX_AFTER_CLOSE_ELEMENT_NAME);
        case '<': case '=':
          goto disallowed_char;
        case '>':
          APPEND_BYTE (0);
          if (!handle_close_element (parser, error))
            return ROSCO_FALSE;
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_DEFAULT);
        default:
          APPEND_BYTE (*data);
          ADVANCE_NON_NL;
          MAYBE_RETURN;
          goto label__LEX_CLOSE_ELEMENT_NAME;
        }
    case LEX_AFTER_CLOSE_ELEMENT_NAME:
    label__LEX_AFTER_CLOSE_ELEMENT_NAME:
      switch (*data)
        {
        WHITESPACE_CASES:
          ADVANCE_CHAR;
          MAYBE_RETURN;
          goto label__LEX_AFTER_CLOSE_ELEMENT_NAME;
        case '>':
          APPEND_BYTE (0);
          if (!handle_close_element (parser, error))
            return ROSCO_FALSE;
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_DEFAULT);
        default:
          goto disallowed_char;
        }
    case LEX_OPEN_CLOSE:
    label__LEX_OPEN_CLOSE:
      switch (*data)
        {
        WHITESPACE_CASES:
          ADVANCE_CHAR;
          MAYBE_RETURN;
          goto label__LEX_OPEN_CLOSE;
        case '>':
          if (!handle_empty_element (parser, error))
            return ROSCO_FALSE;
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_DEFAULT);
        default:
          goto disallowed_char;
        }
    case LEX_LT_BANG:
    label__LEX_LT_BANG:
      switch (*data)
        {
        case '[':
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_LT_BANG_LBRACK);
        case '-':
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_LT_BANG_MINUS);
        WHITESPACE_CASES:
          ADVANCE_CHAR;
          MAYBE_RETURN;
          goto label__LEX_LT_BANG;
        default:
          BUFFER_CLEAR;
          APPEND_BYTE (*data);
          parser->stack[parser->stack_size-1].condense_text = 1;
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_BANG_DIRECTIVE);
        }
    case LEX_LT_BANG_MINUS:
    label__LEX_LT_BANG_MINUS:
      switch (*data)
        {
        case '-':
          if (parser->config->include_comments)
            {
            }
          parser->buffer.length = 0;
          parser->stack[parser->stack_size-1].condense_text = 1;
          CONSUME_CHAR_AND_SWITCH_STATE (LEX_COMMENT);
          break;
        default:
          goto disallowed_char;
        }
    case LEX_COMMENT:
    label__LEX_COMMENT:
      {
        const uint8_t *hyphen = memchr (data, '-', length);
        if (hyphen == NULL)
          {
            if (!IS_SUPPRESSED && parser->config->include_comments)
              simple_buffer_append (&parser->buffer, length, data);
            parser->line_no += count_newlines (length, data);
            return ROSCO_TRUE;
          }
        else
          {
            unsigned skip;
            if (!IS_SUPPRESSED && parser->config->include_comments)
              simple_buffer_append (&parser->buffer, hyphen - data, data);
            skip = hyphen - data;
            parser->line_no += count_newlines (skip, data);
            length -= skip;
            data += skip;
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_COMMENT_MINUS);
          }
      }
    case LEX_COMMENT_MINUS:
    label__LEX_COMMENT_MINUS:
      {
        switch (*data)
          {
          case '-':
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_COMMENT_MINUS_MINUS);
          default:
            APPEND_BYTE ('-');
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_COMMENT);
          }
      }
    case LEX_COMMENT_MINUS_MINUS:
    label__LEX_COMMENT_MINUS_MINUS:
      {
        switch (*data)
          {
          case '-':
            if (!IS_SUPPRESSED && parser->config->include_comments)
              APPEND_BYTE('-');
            CONSUME_NON_NL_AND_SWITCH_STATE (LEX_COMMENT_MINUS_MINUS);
          case '>':
            if (!IS_SUPPRESSED && parser->config->include_comments)
              {
                if (!handle_comment (parser, error))
                  return ROSCO_FALSE;
                BUFFER_CLEAR;
              }
            CONSUME_NON_NL_AND_SWITCH_STATE (LEX_DEFAULT);
          default:
            CONSUME_CHAR_AND_SWITCH_STATE (LEX_COMMENT);
          }
      }
    case LEX_LT_BANG_LBRACK:
    label__LEX_LT_BANG_LBRACK:
      switch (*data)
        {
        case 'c': case 'C':
          parser->entity_buf_len = 1;
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_LT_BANG_LBRACK_IN_CDATAHDR);
        default:
          goto disallowed_char;
        }
    case LEX_LT_BANG_LBRACK_IN_CDATAHDR:
    label__LEX_LT_BANG_LBRACK_IN_CDATAHDR:
      if (*data == "cdata"[parser->entity_buf_len] || *data == "CDATA"[parser->entity_buf_len])
        {
          if (++parser->entity_buf_len == 5)
            CONSUME_NON_NL_AND_SWITCH_STATE (LEX_LT_BANG_LBRACK_CDATAHDR);
          ADVANCE_NON_NL;
          MAYBE_RETURN;
          goto label__LEX_LT_BANG_LBRACK_IN_CDATAHDR;
        }
      else
        goto disallowed_char;
    case LEX_LT_BANG_LBRACK_CDATAHDR:
    label__LEX_LT_BANG_LBRACK_CDATAHDR:
      switch (*data)
        {
        WHITESPACE_CASES:
          ADVANCE_CHAR;
          MAYBE_RETURN;
          goto label__LEX_LT_BANG_LBRACK_CDATAHDR;
        case '[':
          parser->stack[parser->stack_size-1].condense_text = 1;
          CONSUME_CHAR_AND_SWITCH_STATE (LEX_CDATA);
        default:
          goto disallowed_char;
        }
    case LEX_CDATA:
    label__LEX_CDATA:
      {
        const uint8_t *rbracket = memchr (data, ']', length);
        if (rbracket == NULL)
          {
            if (!IS_SUPPRESSED)
              simple_buffer_append (&parser->buffer, length, data);
            parser->line_no += count_newlines (length, data);
            return ROSCO_TRUE;
          }
        else
          {
            unsigned skip = rbracket - data;
            if (!IS_SUPPRESSED)
              simple_buffer_append (&parser->buffer, skip, data);
            parser->line_no += count_newlines (skip, data);
            data = rbracket;
            length -= skip;
            CONSUME_NON_NL_AND_SWITCH_STATE (LEX_CDATA_RBRACK);
          }
      }
    case LEX_CDATA_RBRACK:
    label__LEX_CDATA_RBRACK:
      if (*data == ']')
        CONSUME_NON_NL_AND_SWITCH_STATE (LEX_CDATA_RBRACK_RBRACK);
      else
        {
          if (!IS_SUPPRESSED)
            {
              APPEND_BYTE (']');
              APPEND_BYTE (*data);
            }
          CONSUME_CHAR_AND_SWITCH_STATE (LEX_CDATA);
        }
    case LEX_CDATA_RBRACK_RBRACK:
    label__LEX_CDATA_RBRACK_RBRACK:
      switch (*data)
        {
        case ']':
          if (!IS_SUPPRESSED)
            APPEND_BYTE (']');
          ADVANCE_NON_NL;
          MAYBE_RETURN;
          goto label__LEX_CDATA_RBRACK_RBRACK;
        case '>':
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_DEFAULT);
        default:
          simple_buffer_append (&parser->buffer, 2, "]]");
          APPEND_BYTE (*data);
          CONSUME_CHAR_AND_SWITCH_STATE (LEX_CDATA);
        }
    case LEX_BANG_DIRECTIVE:
    label__LEX_BANG_DIRECTIVE:
      /* this encompasses ELEMENT, ATTLIST, ENTITY, DOCTYPE declarations */
      /* strategy: try every substring until one ends with '>' */
      {
        const uint8_t *rangle = memchr (data, '>', length);
        if (rangle == NULL)
          {
            simple_buffer_append (&parser->buffer, length, data);
            return ROSCO_TRUE;
          }
        else
          {
            unsigned append = rangle + 1 - data;
            simple_buffer_append (&parser->buffer, append, data);
            parser->line_no += count_newlines (append, data);
            length -= append;
            data += append;
            switch (try_directive (parser, error))
              {
              case TRY_DIRECTIVE__NEEDS_MORE:
                MAYBE_RETURN;
                goto label__LEX_BANG_DIRECTIVE;
              case TRY_DIRECTIVE__SUCCESS:
                parser->lex_state = LEX_DEFAULT;
                MAYBE_RETURN;
                goto label__LEX_DEFAULT;
              case TRY_DIRECTIVE__ERROR:
                return ROSCO_FALSE;
              }
          }
      }
    case LEX_PROCESSING_INSTRUCTION:
    label__LEX_PROCESSING_INSTRUCTION:
      switch (*data)
        {
        case '?':
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_PROCESSING_INSTRUCTION_QM);
        default:
          ADVANCE_CHAR;
          MAYBE_RETURN;
          goto label__LEX_PROCESSING_INSTRUCTION;
        }
    case LEX_PROCESSING_INSTRUCTION_QM:
    label__LEX_PROCESSING_INSTRUCTION_QM:
      switch (*data)
        {
        case '?':
          ADVANCE_NON_NL;
          MAYBE_RETURN;
          goto label__LEX_PROCESSING_INSTRUCTION_QM;
        case '>':
          CONSUME_NON_NL_AND_SWITCH_STATE (LEX_DEFAULT);
        default:
          CONSUME_CHAR_AND_SWITCH_STATE (LEX_PROCESSING_INSTRUCTION);
        }
    }

  rosco_assert_not_reached ();

#undef MAYBE_RETURN
#undef ADVANCE_CHAR
#undef ADVANCE_NL
#undef ADVANCE_NON_NL
#undef CONSUME_CHAR_AND_SWITCH_STATE
#undef CONSUME_NL_AND_SWITCH_STATE
#undef CONSUME_NON_NL_AND_SWITCH_STATE
#undef APPEND_BYTE

disallowed_char:
  {
    rosco_assert (length > 0);
    rosco_set_error (error,
                   "unexpected character %s in %s, line %u (%s)",
                   rosco_ascii_byte_name (*data),
                   parser->filename ? parser->filename->filename : "string",
                   parser->line_no,
                   lex_state_description (parser->lex_state));
    return ROSCO_FALSE;
  }

entity_ref_too_long:
  {
    rosco_set_error (error,
                   "entity reference &%.*s... too long in %s, line %u",
                   (int)(MAX_ENTITY_REF_LENGTH - 3), parser->entity_buf,
                   parser->filename ? parser->filename->filename : "string",
                   parser->line_no);
    return ROSCO_FALSE;
  }
}

static void
destruct_parse_state_recursive (ParseState *state)
{
  unsigned i;
  for (i = 0; i < state->n_transitions; i++)
    {
      destruct_parse_state_recursive (state->transitions[i].state);
      rosco_free (state->transitions[i].state);
      rosco_free (state->transitions[i].str);
    }
  if (state->wildcard_transition)
    {
      destruct_parse_state_recursive (state->wildcard_transition);
      rosco_free (state->wildcard_transition);
    }
  rosco_free (state->transitions);
  rosco_free (state->ret_indices);
}

void
rosco_xml_parser_config_unref (RoscoXmlParserConfig *config)
{
  if (--(config->ref_count) == 0)
    {
      rosco_assert (config->destroyed);
      rosco_free (config->ns);
      destruct_parse_state_recursive (&config->base);
      rosco_free (config);
    }
}

void
rosco_xml_parser_config_destroy (RoscoXmlParserConfig *config)
{
  rosco_assert (!config->destroyed);
  config->destroyed = 1;
  rosco_xml_parser_config_unref (config);
}

static RoscoXmlFilename *new_filename (const char *str)
{
  unsigned length = strlen (str);
  RoscoXmlFilename *filename = rosco_malloc (sizeof (RoscoXmlFilename) + length + 1);
  memcpy (filename + 1, str, length + 1);
  filename->filename = (char*)(filename+1);
  filename->ref_count = 1;
  return filename;
}

RoscoXmlParser *
rosco_xml_parser_new (RoscoXmlParserConfig *config,
                    const char         *display_filename)
{
  RoscoXmlParser *parser;
  rosco_assert (config != NULL);
  rosco_assert (!config->destroyed);
  parser = ROSCO_NEW (RoscoXmlParser);

  parser->filename = display_filename ? new_filename (display_filename) : NULL;
  parser->line_no = 1;
  parser->lex_state = LEX_DEFAULT;
  simple_buffer_init (&parser->buffer);
  simple_buffer_init (&parser->stack_tag_strs);
  parser->ns_map = NULL;
  parser->stack_size = 1;
  parser->stack[0].needed = ROSCO_FALSE;
  parser->stack[0].name_kv_space = 0;
  parser->stack[0].n_attrs = 0;
  parser->stack[0].state = &config->base;
  parser->stack[0].defined_list = NULL;
  parser->stack[0].n_children = 0;
  parser->stack_children_alloced = 8;
  parser->stack_children = ROSCO_NEW_ARRAY (RoscoXml *, parser->stack_children_alloced);
  parser->n_stack_children = 0;
  parser->config = config;
  parser->default_ns = NULL;
  parser->max_ns_expand = 0;
  parser->max_ns_attr_expand = 0;
  parser->last_result = parser->first_result = NULL;
  config->ref_count += 1;
  return parser;
}


RoscoXml *
rosco_xml_parser_pop (RoscoXmlParser       *parser,
                    unsigned           *xpath_index_out)
{
  ResultQueueNode *n = parser->first_result;
  RoscoXml *xml;
  if (n == NULL)
    return NULL;
  if (xpath_index_out)
    *xpath_index_out = n->index;
  xml = n->xml;
  parser->first_result = n->next;
  if (parser->first_result == NULL)
    parser->last_result = NULL;
  rosco_free (n);                 /* TO CONSIDER: recycle n? */
  return xml;
}

void
rosco_xml_parser_free(RoscoXmlParser       *parser)
{
  unsigned i;
  while (parser->first_result)
    {
      ResultQueueNode *n = parser->first_result;
      parser->first_result = n->next;

      rosco_xml_unref (n->xml);
      rosco_free (n);
    }
  parser->last_result = NULL;

  /* free stack */
  for (i = 0; i < parser->stack_size; i++)
    {
      NsAbbrevMap *d = parser->stack[i].defined_list;
      while (d)
        {
          NsAbbrevMap *n = d->defined_list_next;
          rosco_free (d->abbrev);
          rosco_free (d);
          d = n;
        }
    }
  for (i = 0; i < parser->n_stack_children; i++)
    rosco_xml_unref (parser->stack_children[i]);
  rosco_free (parser->stack_children);
  simple_buffer_clear (&parser->stack_tag_strs);
  simple_buffer_clear (&parser->buffer);

  if (parser->filename)
    {
      if (--(parser->filename->ref_count) == 0)
        rosco_free (parser->filename);
    }

  /* free config */
  rosco_xml_parser_config_unref (parser->config);

  /* TODO: audit / use valgrind to ensure no leakage */

  rosco_free (parser);
}

