// functions primarily useful to the generated code.

void rosco_machgen_error_too_short (DskBuffer  *buf, 
                                    size_t      min_size,
                                    const char *type_name,
                                    DskError  **error);

#define ROSCO_MACHGEN_CHECK_BUFFER_SIZE(buf, min_size, type_name, error) \
  do {                                                                   \
    if (DSK_UNLIKELY ((buf)->size < (min_size)))                         \
      {                                                                  \
        rosco_machgen_error_too_short (buf, min_size, type_name, error); \
        return DSK_FALSE;                                                \
      }                                                                  \
  } while(0)

static inline dsk_boolean
rosco_bool__serialize(rosco_bool value,
                      DskBuffer *out,
                      DskError **error)
{
  uint8_t v = value;
  if (v > 1)
    {
      dsk_set_error (error, "bool serialize: bool value must be 0 or 1");
      return DSK_FALSE;
    }
  dsk_buffer_append_byte (out, v);
  return DSK_TRUE;
}
static dsk_boolean
rosco_bool__deserialize (DskBuffer *in,
                         rosco_bool        *ptr_value_out,
			 DskError **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE(in, 1, "bool", error);
  uint8_t v = dsk_buffer_read_byte (in);
  if (v > 1)
    {
      dsk_set_error (error, "bool deserialize: bool value must be 0 or 1");
      return DSK_FALSE;
    }
  * ptr_value_out = v;
  return DSK_TRUE;
}
static inline rosco_bool
rosco_uint8__serialize     (uint8_t     value,
                            DskBuffer  *out,
                            DskError  **error)
{
  (void) error;
  dsk_buffer_append_byte (out, value);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_uint8__deserialize   (DskBuffer  *in,
                            uint8_t    *value_out,
                            DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 1, "uint8", error);
  dsk_buffer_read (in, 1, value_out);
  return DSK_TRUE;
}
    
#define rosco_uint8__destruct(value)

rosco_bool rosco_int8__serialize      (int8_t      value,
                                       DskBuffer  *out,
                                       DskError  **error)
{
  (void) error;
  dsk_buffer_append_byte (out, (uint8_t) value);
  return DSK_TRUE;
}

rosco_bool rosco_int8__deserialize    (DskBuffer  *in,
                                       int8_t     *value_out,
                                       DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 1, "int8", error);
  dsk_buffer_read (in, 1, value_out);
  return DSK_TRUE;
}
#define rosco_int8__destruct(value)


static inline rosco_bool
rosco_uint16__serialize     (uint16_t    value,
                             DskBuffer  *out,
                             DskError  **error)
{
  (void) error;
  uint8_t tmp[2] = { (uint8_t) value, (uint8_t)(value >> 8) };
  dsk_buffer_append (out, 2, tmp);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_uint16__deserialize  (DskBuffer  *in,
                            uint16_t   *value_out,
                            DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 2, "uint16", error);
  uint8_t tmp[2];
  dsk_buffer_read (in, 2, tmp);
  *value_out = (uint16_t)tmp[0] | ((uint16_t)tmp[1] << 8);
  return DSK_TRUE;
}
    
#define rosco_uint16__destruct(value)

rosco_bool rosco_int16__serialize      (int16_t      value,
                                       DskBuffer  *out,
                                       DskError  **error)
{
  (void) error;
  uint8_t tmp[2] = { (uint8_t) value, (uint8_t)(value >> 8) };
  dsk_buffer_append (out, 2, tmp);
  return DSK_TRUE;
}

rosco_bool rosco_int16__deserialize    (DskBuffer *in,
                                       int16_t    *value_out,
                                       DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 2, "int16", error);
  uint8_t tmp[2];
  dsk_buffer_read (in, 2, tmp);
  *value_out = (int16_t) ((uint16_t)tmp[0] | ((uint16_t)tmp[1] << 8));
  return DSK_TRUE;
}
#define rosco_int16__destruct(value)


static inline rosco_bool
rosco_uint32__serialize     (uint32_t    value,
                             DskBuffer  *out,
                             DskError  **error)
{
  (void) error;
  uint8_t tmp[4] = { (uint8_t) value,
                     (uint8_t)(value >> 8),
                     (uint8_t)(value >> 16),
                     (uint8_t)(value >> 24) };
  dsk_buffer_append (out, 4, tmp);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_uint32__deserialize  (DskBuffer  *in,
                            uint32_t   *value_out,
                            DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 4, "uint32", error);
  uint8_t tmp[4];
  dsk_buffer_read (in, 4, tmp);
  *value_out = ((uint32_t)tmp[0])
             | ((uint32_t)tmp[1] << 8)
             | ((uint32_t)tmp[2] << 16)
             | ((uint32_t)tmp[3] << 24);
  return DSK_TRUE;
}
    
#define rosco_uint32__destruct(value)

rosco_bool rosco_int32__serialize     (int32_t     value,
                                       DskBuffer  *out,
                                       DskError  **error)
{
  (void) error;
  uint8_t tmp[4] = { (uint8_t) value,
                     (uint8_t)(value >> 8),
                     (uint8_t)(value >> 16),
                     (uint8_t)(value >> 24) };
  dsk_buffer_append (out, 4, tmp);
  return DSK_TRUE;
}

rosco_bool rosco_int32__deserialize    (DskBuffer *in,
                                       int32_t    *value_out,
                                       DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 4, "int32", error);
  uint8_t tmp[4];
  dsk_buffer_read (in, 4, tmp);
  *value_out = (int32_t) ( ((uint32_t)tmp[0])
                         | ((uint32_t)tmp[1] << 8)
                         | ((uint32_t)tmp[2] << 16)
                         | ((uint32_t)tmp[3] << 24)
                         );
  return DSK_TRUE;
}
#define rosco_int32__destruct(value)

static inline rosco_bool
rosco_uint64__serialize     (uint64_t    value,
                             DskBuffer  *out,
                             DskError  **error)
{
  (void) error;
  uint8_t tmp[8] = { (uint8_t)(value),
                     (uint8_t)(value >> 8),
                     (uint8_t)(value >> 16),
                     (uint8_t)(value >> 24),
                     (uint8_t)(value >> 32),
                     (uint8_t)(value >> 40),
                     (uint8_t)(value >> 48),
                     (uint8_t)(value >> 56) };
  dsk_buffer_append (out, 8, tmp);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_uint64__deserialize  (DskBuffer  *in,
                            uint64_t   *value_out,
                            DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 8, "uint64", error);
  uint8_t tmp[8];
  dsk_buffer_read (in, 8, tmp);
  *value_out = ((uint64_t)tmp[0])
             | ((uint64_t)tmp[1] << 8)
             | ((uint64_t)tmp[2] << 16)
             | ((uint64_t)tmp[3] << 24)
             | ((uint64_t)tmp[4] << 32)
             | ((uint64_t)tmp[5] << 40)
             | ((uint64_t)tmp[6] << 48)
             | ((uint64_t)tmp[7] << 56);
  return DSK_TRUE;
}
    
#define rosco_uint64__destruct(value)

rosco_bool rosco_int64__serialize     (int64_t     value,
                                       DskBuffer  *out,
                                       DskError  **error)
{
  (void) error;
  uint8_t tmp[8] = { (uint8_t) value,
                     (uint8_t)(value >> 8),
                     (uint8_t)(value >> 16),
                     (uint8_t)(value >> 24),
                     (uint8_t)(value >> 32),
                     (uint8_t)(value >> 40),
                     (uint8_t)(value >> 48),
                     (uint8_t)(value >> 56) };
  dsk_buffer_append (out, 8, tmp);
  return DSK_TRUE;
}

rosco_bool rosco_int64__deserialize    (DskBuffer *in,
                                       int64_t    *value_out,
                                       DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 8, "int64", error);
  uint8_t tmp[8];
  dsk_buffer_read (in, 8, tmp);
  *value_out = (int64_t) ( ((uint64_t)tmp[0])
                         | ((uint64_t)tmp[1] << 8)
                         | ((uint64_t)tmp[2] << 16)
                         | ((uint64_t)tmp[3] << 24)
                         | ((uint64_t)tmp[4] << 32)
                         | ((uint64_t)tmp[5] << 40)
                         | ((uint64_t)tmp[6] << 48)
                         | ((uint64_t)tmp[7] << 56)
                         );
  return DSK_TRUE;
}
#define rosco_int64__destruct(value)

static inline rosco_bool
rosco_float32__serialize     (float       value,
                              DskBuffer  *out,
                              DskError  **error)
{
  (void) error;
  union {
    uint32_t ival;
    float fval;
  } u;
  u.fval = value;
  uint8_t tmp[4] = { (uint8_t)(u.ival),
                     (uint8_t)(u.ival >> 8),
                     (uint8_t)(u.ival >> 16),
                     (uint8_t)(u.ival >> 24) };
  dsk_buffer_append (out, 4, tmp);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_float32__deserialize  (DskBuffer  *in,
                             float      *value_out,
                             DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 4, "float32", error);
  union {
    uint32_t ival;
    float fval;
  } u;
  uint8_t tmp[4];
  dsk_buffer_read (in, 4, tmp);
  u.ival = ((uint32_t)tmp[0])
         | ((uint32_t)tmp[1] << 8)
         | ((uint32_t)tmp[2] << 16)
         | ((uint32_t)tmp[3] << 24);
  *value_out = u.fval;
  return DSK_TRUE;
}
    
#define rosco_float32__destruct(value)

static inline rosco_bool
rosco_float64__serialize     (double    value,
                             DskBuffer  *out,
                             DskError  **error)
{
  (void) error;
  union {
    uint64_t ival;
    double fval;
  } u;
  u.fval = value;
  uint8_t tmp[8] = { (uint8_t)(u.ival),
                     (uint8_t)(u.ival >> 8),
                     (uint8_t)(u.ival >> 16),
                     (uint8_t)(u.ival >> 24),
                     (uint8_t)(u.ival >> 32),
                     (uint8_t)(u.ival >> 40),
                     (uint8_t)(u.ival >> 48),
                     (uint8_t)(u.ival >> 56) };
  dsk_buffer_append (out, 8, tmp);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_float64__deserialize  (DskBuffer  *in,
                            double   *value_out,
                            DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 8, "uint64", error);
  uint8_t tmp[8];
  dsk_buffer_read (in, 8, tmp);
  union {
    double fval;
    uint64_t ival;
  } u;
  u.ival = ((uint64_t)tmp[0])
             | ((uint64_t)tmp[1] << 8)
             | ((uint64_t)tmp[2] << 16)
             | ((uint64_t)tmp[3] << 24)
             | ((uint64_t)tmp[4] << 32)
             | ((uint64_t)tmp[5] << 40)
             | ((uint64_t)tmp[6] << 48)
             | ((uint64_t)tmp[7] << 56);
  *value_out = u.fval;
  return DSK_TRUE;
}
    
#define rosco_float64__destruct(value)

static inline rosco_bool
rosco_duration__serialize     (RoscoDuration duration,
                               DskBuffer    *out,
                               DskError    **error)
{
  (void) error;
  uint8_t tmp[8] = { (uint8_t)(duration.secs),
                     (uint8_t)(duration.secs >> 8),
                     (uint8_t)(duration.secs >> 16),
                     (uint8_t)(duration.secs >> 24),
                     (uint8_t)(duration.nsecs),
                     (uint8_t)(duration.nsecs >> 8),
                     (uint8_t)(duration.nsecs >> 16),
                     (uint8_t)(duration.nsecs >> 24) };
  dsk_buffer_append (out, 8, tmp);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_duration__deserialize  (DskBuffer  *in,
                            RoscoDuration   *value_out,
                            DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 8, "duration", error);
  uint8_t tmp[8];
  dsk_buffer_read (in, 8, tmp);
  value_out->secs = (int32_t)
                      ( ((uint32_t)tmp[0])
                      | ((uint32_t)tmp[1] << 8)
                      | ((uint32_t)tmp[2] << 16)
                      | ((uint32_t)tmp[3] << 24)
                      );
  value_out->nsecs = (int32_t)
                      ( ((uint32_t)tmp[4])
                      | ((uint32_t)tmp[5] << 8)
                      | ((uint32_t)tmp[6] << 16)
                      | ((uint32_t)tmp[7] << 24)
                      );
  return DSK_TRUE;
}
    

#define rosco_duration__destruct(value)
static inline rosco_bool
rosco_time__serialize     (RoscoTime time,
                           DskBuffer    *out,
                           DskError    **error)
{
  (void) error;
  uint8_t tmp[8] = { (uint8_t)(time.secs),
                     (uint8_t)(time.secs >> 8),
                     (uint8_t)(time.secs >> 16),
                     (uint8_t)(time.secs >> 24),
                     (uint8_t)(time.nsecs),
                     (uint8_t)(time.nsecs >> 8),
                     (uint8_t)(time.nsecs >> 16),
                     (uint8_t)(time.nsecs >> 24) };
  dsk_buffer_append (out, 8, tmp);
  return DSK_TRUE;
}

static inline rosco_bool
rosco_time__deserialize  (DskBuffer  *in,
                          RoscoTime   *value_out,
                          DskError  **error)
{
  ROSCO_MACHGEN_CHECK_BUFFER_SIZE (in, 8, "time", error);
  uint8_t tmp[8];
  dsk_buffer_read (in, 8, tmp);
  value_out->secs =   ( ((uint32_t)tmp[0])
                      | ((uint32_t)tmp[1] << 8)
                      | ((uint32_t)tmp[2] << 16)
                      | ((uint32_t)tmp[3] << 24)
                      );
  value_out->nsecs =  ( ((uint32_t)tmp[4])
                      | ((uint32_t)tmp[5] << 8)
                      | ((uint32_t)tmp[6] << 16)
                      | ((uint32_t)tmp[7] << 24)
                      );
  return DSK_TRUE;
}
    

#define rosco_time__destruct(value)

rosco_bool rosco_string__serialize    (const char *str,
                                       DskBuffer  *out,
                                       DskError  **error);
rosco_bool rosco_string__deserialize  (DskBuffer  *in,
                                       char      **str_out,
                                       DskError  **error);
rosco_bool rosco_string__destruct     (char       *str);
