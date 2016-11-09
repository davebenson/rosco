// Errors that originate from this library (Rosco).
// Errors from other layers should not be included here.

typedef enum {
  // errors from parsing URLs
  ROSCO_ERROR_INVALID_URL,
  ROSCO_ERROR_UNSUPPORTED_URL_SCHEME,
  ROSCO_ERROR_UNSUPPORTED_URL,

  // errors from sockets
  ...

  // errors from http-parser
  ...

  // errors from http-parser
  ...
} RoscoErrorCode;

extern DskErrorDataType rosco_error_code_dsk_error_type;
dsk_boolean dsk_error_get_rosco_error_code      (const DskError *error,
                                                 RoscoErrorCode *out);
void        dsk_error_set_rosco_error_code      (DskError       *error,
                                                 RoscoErrorCode code);

// convenience for dsk_error_new() + dsk_error_set_rosco_error_code()
DskError   *rosco_error_new    (RoscoErrorCode code,
                                const char    *format,
                                ...);

