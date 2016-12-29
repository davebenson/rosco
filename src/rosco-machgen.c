#include "rosco.h"

void rosco_machgen_error_too_short (DskBuffer  *buf, 
                                    size_t      min_size,
                                    const char *type_name,
                                    DskError  **error)
{
  dsk_set_error (error,
                 "less data was available than expected: expected at least %u, but %u; when parsing %s",
                 (unsigned)min_size,
                 (unsigned)buf->size,
                 type_name);
}

///static dsk_boolean
///rosco_type__serialize__uint16(RoscoType *type,
///		              const void *ptr_value,
///		              DskBuffer *out,
///		              DskError **error)
///{
///  (void) type;
///  (void) error;
///  uint16_t v = * (const uint16_t *) ptr_value;
///  uint8_t data[2] = { v & 0xff, v >> 8 };   /* little-endian */
///  dsk_buffer_append (out, 2, data);
///  return DSK_TRUE;
///}
///static dsk_boolean
///rosco_type__deserialize__uint16(RoscoType *type,
///			        DskBuffer *in,
///			        void        *ptr_value_out,
///			        DskError **error)
///{
///  DESERIALIZE_CHECK_MIN_SIZE(type->name, in, 2, error);
///  uint8_t buf[2];
///  dsk_buffer_read (in, 2, buf);
///  uint16_t v = (uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8) ;
///  * (uint16_t *) ptr_value_out = v;
///  return DSK_TRUE;
///}
///#define rosco_type__destruct__uint16 rosco_type__destruct__no_op
///static RoscoType rosco_type__uint16 = DEFINE_ROSCO_TYPE(UINT16, uint16_t, uint16, rosco_uint16);
///
///#define rosco_type__serialize__int16 rosco_type__serialize__uint16
///#define rosco_type__deserialize__int16 rosco_type__deserialize__uint16
///#define rosco_type__destruct__int16 rosco_type__destruct__no_op
///static RoscoType rosco_type__int16 = DEFINE_ROSCO_TYPE(INT16, int16_t, int16, rosco_int16);
///static dsk_boolean
///rosco_type__serialize__uint32(RoscoType *type,
///		              const void *ptr_value,
///		              DskBuffer *out,
///		              DskError **error)
///{
///  (void) type;
///  (void) error;
///  uint32_t v = * (const uint32_t *) ptr_value;
///  uint8_t data[4] = { UINT32_TO_UINT8LE_ARRAY (v) };
///  dsk_buffer_append (out, 4, data);
///  return DSK_TRUE;
///}
///static dsk_boolean
///rosco_type__deserialize__uint32(RoscoType *type,
///			        DskBuffer *in,
///			        void        *ptr_value_out,
///			        DskError **error)
///{
///  DESERIALIZE_CHECK_MIN_SIZE(type->name, in, 4, error);
///  uint8_t buf[4];
///  dsk_buffer_read (in, 4, buf);
///  uint32_t v =  (uint32_t)(buf[0])
///             | ((uint32_t)(buf[1]) << 8)
///             | ((uint32_t)(buf[2]) << 16)
///             | ((uint32_t)(buf[3]) << 24);
///  * (uint32_t *) ptr_value_out = v;
///  return DSK_TRUE;
///}
///#define rosco_type__destruct__uint32 rosco_type__destruct__no_op
///static RoscoType rosco_type__uint32 = DEFINE_ROSCO_TYPE(UINT32, uint32_t, uint32, rosco_uint32);
///
///#define rosco_type__serialize__int32 rosco_type__serialize__uint32
///#define rosco_type__deserialize__int32 rosco_type__deserialize__uint32
///#define rosco_type__destruct__int32 rosco_type__destruct__no_op
///static RoscoType rosco_type__int32 = DEFINE_ROSCO_TYPE(INT32, int32_t, int32, rosco_int32);
///
///static dsk_boolean
///rosco_type__serialize__uint64(RoscoType *type,
///		              const void *ptr_value,
///		              DskBuffer *out,
///		              DskError **error)
///{
///  (void) type;
///  (void) error;
///  uint64_t v = * (const uint64_t *) ptr_value;
///  uint8_t data[8] = { v & 0xff, v >> 8, v >> 16, v >> 24,
///                      v >> 32, v >> 40, v >> 48, v >> 54 };
///  dsk_buffer_append (out, 8, data);
///  return DSK_TRUE;
///}
///static dsk_boolean
///rosco_type__deserialize__uint64(RoscoType *type,
///			        DskBuffer *in,
///			        void        *ptr_value_out,
///			        DskError **error)
///{
///  DESERIALIZE_CHECK_MIN_SIZE(type->name, in, 8, error);
///  uint8_t buf[8];
///  dsk_buffer_read (in, 8, buf);
///  uint64_t v =  (uint64_t)(buf[0])
///             | ((uint64_t)(buf[1]) << 8)
///             | ((uint64_t)(buf[2]) << 16)
///             | ((uint64_t)(buf[3]) << 24)
///             | ((uint64_t)(buf[4]) << 32)
///             | ((uint64_t)(buf[5]) << 40)
///             | ((uint64_t)(buf[6]) << 48)
///             | ((uint64_t)(buf[7]) << 54);
///  * (uint64_t *) ptr_value_out = v;
///  return DSK_TRUE;
///}
///#define rosco_type__destruct__uint64 rosco_type__destruct__no_op
///static RoscoType rosco_type__uint64 = DEFINE_ROSCO_TYPE(UINT64, uint64_t, uint64, rosco_uint64);
///
///#define rosco_type__serialize__int64 rosco_type__serialize__uint64
///#define rosco_type__deserialize__int64 rosco_type__deserialize__uint64
///#define rosco_type__destruct__int64 rosco_type__destruct__no_op
///static RoscoType rosco_type__int64 = DEFINE_ROSCO_TYPE(INT64, int64_t, int64, rosco_int64);
///
///#define rosco_type__serialize__float32 rosco_type__serialize__uint32
///#define rosco_type__deserialize__float32 rosco_type__deserialize__uint32
///#define rosco_type__destruct__float32 rosco_type__destruct__no_op
///static RoscoType rosco_type__float32 = DEFINE_ROSCO_TYPE(FLOAT32, float, float32, rosco_float32);
///#define rosco_type__serialize__float64 rosco_type__serialize__uint64
///#define rosco_type__deserialize__float64 rosco_type__deserialize__uint64
///#define rosco_type__destruct__float64 rosco_type__destruct__no_op
///static RoscoType rosco_type__float64 = DEFINE_ROSCO_TYPE(FLOAT64, double, float64, rosco_float64);
///
///static dsk_boolean
///rosco_type__serialize__string(RoscoType *type,
///		              const void *ptr_value,
///		              DskBuffer *out,
///		              DskError **error)
///{
///  (void) type;
///  (void) error;
///  const char *str = * (const char **) ptr_value;
///  unsigned len = strlen (str);
///  uint8_t len_enc[4] = {UINT32_TO_UINT8LE_ARRAY(len)};
///  dsk_buffer_append (out, 4, len_enc);
///  dsk_buffer_append (out, len, str);
///  return DSK_TRUE;
///}
///static dsk_boolean
///rosco_type__deserialize__string(RoscoType *type,
///			        DskBuffer *in,
///			        void        *ptr_value_out,
///			        DskError **error)
///{
///  DESERIALIZE_CHECK_MIN_SIZE (type->name, in, 4, error);
///  uint8_t header[4];
///  dsk_buffer_read (in, 4, header);
///  uint32_t len = ((uint32_t)header[0])
///               | ((uint32_t)header[1] << 8)
///               | ((uint32_t)header[2] << 16)
///               | ((uint32_t)header[3] << 24);
///  if (in->size < len)
///    {
///      dsk_set_error (error, "%s deserialize: end-of-data", type->name);
///      return DSK_FALSE;
///    }
///  char *str = dsk_malloc (len + 1);
///  dsk_buffer_read (in, len, str);
///  str[len] = 0;
///  * (char **) ptr_value_out = str;
///  return DSK_TRUE;
///}
///static void
///rosco_type__destruct__string (RoscoType *type, void *ptr_data)
///{
///  (void)type;
///  dsk_free (*(char**)ptr_data);
///}
///static RoscoType rosco_type__string = DEFINE_ROSCO_TYPE(STRING, char *, string, rosco_string);
///
///static dsk_boolean
///rosco_type__serialize__time(RoscoType *type,
///		            const void *ptr_value,
///		            DskBuffer *out,
///		            DskError **error)
///{
///  (void) type;
///  (void) error;
///  RoscoTime time = * (const RoscoTime *) ptr_value;
///  uint8_t enc[8] = {
///    UINT32_TO_UINT8LE_ARRAY(time.secs),
///    UINT32_TO_UINT8LE_ARRAY(time.nsecs)
///  };
///  dsk_buffer_append (out, 8, enc);
///  return DSK_TRUE;
///}
///static dsk_boolean
///rosco_type__deserialize__time(RoscoType *type,
///			        DskBuffer *in,
///			        void        *ptr_value_out,
///			        DskError **error)
///{
///  (void) type;
///  DESERIALIZE_CHECK_MIN_SIZE (type->name, in, 8, error);
///  uint8_t tmp[8];
///  dsk_buffer_read (in, 8, tmp);
///  RoscoTime *out = ptr_value_out;
///  out->secs  = ((uint32_t)tmp[0])
///             | ((uint32_t)tmp[1] << 8)
///             | ((uint32_t)tmp[2] << 16)
///             | ((uint32_t)tmp[3] << 24);
///  out->nsecs = ((uint32_t)tmp[4])
///             | ((uint32_t)tmp[5] << 8)
///             | ((uint32_t)tmp[6] << 16)
///             | ((uint32_t)tmp[7] << 24);
///
///  // normalize the duration so that 0 <= nsecs < BILLION
///  if (DSK_UNLIKELY (out->nsecs > BILLION))
///    {
///      unsigned s_in_n = out->nsecs / BILLION;
///      out->nsecs %= BILLION;
///      out->secs += s_in_n;
///    }
///  return DSK_TRUE;
///}
///#define rosco_type__destruct__time rosco_type__destruct__no_op
///static RoscoType rosco_type__time = DEFINE_ROSCO_TYPE(TIME, RoscoTime, time, rosco_time);
///
///static dsk_boolean
///rosco_type__serialize__duration(RoscoType *type,
///		                const void *ptr_value,
///		                DskBuffer *out,
///		                DskError **error)
///{
///  (void) type;
///  (void) error;
///  RoscoDuration duration = * (const RoscoDuration *) ptr_value;
///  uint8_t enc[8] = {
///    UINT32_TO_UINT8LE_ARRAY(duration.secs),
///    UINT32_TO_UINT8LE_ARRAY(duration.nsecs)
///  };
///  dsk_buffer_append (out, 8, enc);
///  return DSK_TRUE;
///}
///static dsk_boolean
///rosco_type__deserialize__duration(RoscoType *type,
///			          DskBuffer *in,
///			          void        *ptr_value_out,
///			          DskError **error)
///{
///  DESERIALIZE_CHECK_MIN_SIZE (type->name, in, 8, error);
///  uint8_t tmp[8];
///  dsk_buffer_read (in, 8, tmp);
///  RoscoTime *out = ptr_value_out;
///  out->secs  = ((int32_t)tmp[0])
///             | ((int32_t)tmp[1] << 8)
///             | ((int32_t)tmp[2] << 16)
///             | ((int32_t)tmp[3] << 24);
///  out->nsecs = ((int32_t)tmp[4])
///             | ((int32_t)tmp[5] << 8)
///             | ((int32_t)tmp[6] << 16)
///             | ((int32_t)tmp[7] << 24);
///
///  // normalize the duration so that 0 <= nsecs < BILLION
///  if (DSK_UNLIKELY (out->nsecs > BILLION))
///    {
///      unsigned s_in_n = out->nsecs / BILLION;
///      out->nsecs %= BILLION;
///      out->secs += s_in_n;
///    }
///  return DSK_TRUE;
///}
