// functions primarily useful to the generated code.

void rosco_machgen_error_too_short (DskBuffer *buf, 
                                    size_t     min_size,
                                    RoscoType *type,
                                    DskError **error);

#define ROSCO_MACHGEN_CHECK_BUFFER_SIZE(buf, min_size, type_name, error) \
  do {                                                                   \
    if (DSK_UNLIKELY ((buf)->size < (min_size)))                         \
      {                                                                  \
        rosco_machgen_error_too_short (buf, min_size, type_name, error); \
        return DSK_FALSE;                                                \
      }                                                                  \
  } while(0)
static inline rosco_bool
rosco_uint8__serialize     (uint8_t     value,
                            DskBuffer  *out,
                            DskError  **error)
{
  (void) type;
  (void) error;
  dsk_buffer_append_byte (out, value);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_uint8__deserialize   (RoscoType  *type,
                            DskBuffer  *in,
                            uint8_t    *value_out,
                            DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 1, "uint8", error);
  dsk_buffer_read (in, 1, value_out);
  return DSK_TRUE;
}
    
#define rosco_uint8__destruct(value)

rosco_bool rosco_int8__serialize      (RoscoType  *type,
                                       int8_t      value,
                                       DskBuffer  *out,
                                       DskError  **error)
{
  (void) type;
  (void) error;
  dsk_buffer_append_byte (out, (uint8_t) value);
  return DSK_TRUE;
}

rosco_bool rosco_int8__deserialize    (RoscoType  *type,
                                       DskBuffer  *in,
                                       int8_t     *value_out,
                                       DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 1, "uint8", error);
  dsk_buffer_read (in, 1, value_out);
  return DSK_TRUE;
}
#define rosco_int8__destruct(value)



rosco_bool rosco_string__serialize    (const char *str,
                                       DskBuffer  *out,
                                       DskError  **error);
rosco_bool rosco_string__deserialize  (DskBuffer  *in,
                                       char      **str_out,
                                       DskError  **error);
rosco_bool rosco_string__destruct     (char       *str);
