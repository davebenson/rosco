/* this file is only intended for use
 * from generated message and service files.
 */

static inline dsk_boolean 
rosco_deserialization_check_min_size (DskBuffer *buffer,
                                      size_t     min_size,
                                      RoscoType *type,
                                      DskError **error)
{
  if (buffer->size < min_size)
    {
      dsk_set_error (error, "buffer too small for type %s", type->name);
      return DSK_FALSE;
    }
  else 
    return DSK_TRUE;
}

static inline dsk_boolean
rosco_serialization__bool  (RoscoType        *type,
		            const rosco_bool *ptr_value,
		            DskBuffer        *out,
		            DskError        **error)
{
  dsk_buffer_append_byte (out, *ptr_value);
  return DSK_TRUE;
}
static inline dsk_boolean
rosco_deserialization__bool  (RoscoType   *type,
			      DskBuffer   *in,
			      rosco_bool  *ptr_value_out,
		              DskError  **error)
{
  if (!rosco_deserialization_check_min_size (in, 1, type, error))
    return DSK_FALSE;
  *ptr_value_out = dsk_buffer_read_byte (in);
  return DSK_TRUE;
}
static inline dsk_boolean
rosco_serialization__uint8 (RoscoType        *type,
		            const uint8_t    *ptr_value,
		            DskBuffer        *out,
		            DskError        **error)
{
  dsk_buffer_append_byte (out, *ptr_value);
  return DSK_TRUE;
}
static inline dsk_boolean
rosco_deserialization__uint8 (RoscoType   *type,
			      DskBuffer   *in,
			      uint8_t     *ptr_value_out,
		              DskError  **error)
{
  if (!rosco_deserialization_check_min_size (in, 1, type, error))
    return DSK_FALSE;
  *ptr_value_out = dsk_buffer_read_byte (in);
  return DSK_TRUE;
}
static inline dsk_boolean
rosco_serialization__int8  (RoscoType        *type,
		            const int8_t     *ptr_value,
		            DskBuffer        *out,
		            DskError        **error)
{
  dsk_buffer_append_byte (out, *ptr_value);
  return DSK_TRUE;
}
static inline dsk_boolean
rosco_deserialization__int8 (RoscoType   *type,
			     DskBuffer   *in,
			     int8_t      *ptr_value_out,
		             DskError   **error)
{
  if (!rosco_deserialization_check_min_size (in, 1, type, error))
    return DSK_FALSE;
  *ptr_value_out = dsk_buffer_read_byte (in);
  return DSK_TRUE;
}
