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

typedef struct RoscoError RoscoError;{
struct RoscoError {
  RoscoErrorCode code;
  char *message;
  RoscoError *cause;
  unsigned ref_count;
};
RoscoError *rosco_error_new    (RoscoErrorCode code,
                                const char    *format,
                                ...);
RoscoError *rosco_error_ref    (RoscoError    *error);
void        rosco_error_unref  (RoscoError    *error);

