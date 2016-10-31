
typedef enum {
  ROSCO_HTTP_SCAN_STATUS_SUCCESS,
  ROSCO_HTTP_SCAN_STATUS_INCOMPLETE,
  ROSCO_HTTP_SCAN_STATUS_ERROR
} RoscoHttpScanStatus;

RoscoHttpScanStatus rosco_http_scan_header (RoscoBuffer *buffer,
                                            size_t      *n_bytes_out,
                                            RoscoError **error);
