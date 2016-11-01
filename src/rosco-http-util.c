#include "rosco-http-util.h"

typedef enum {
  ROSCO_HTTP_SCAN_STATUS_SUCCESS,
  ROSCO_HTTP_SCAN_STATUS_INCOMPLETE,
  ROSCO_HTTP_SCAN_STATUS_ERROR
} RoscoHttpScanStatus;


RoscoHttpScanStatus
rosco_http_scan_header (RoscoBuffer *buffer,
                        size_t      *n_bytes_out,
                        RoscoError **error)
{
  RoscoBufferFragment *frag;
  rosco_bool at_start_of_line = ROSCO_TRUE;
  size_t discarded = 0;
  for (frag = buffer->first_frag; frag != NULL; frag = frag->next)
    {
      const uint8_t *start = frag->buf + frag->buf_start;
      const uint8_t *at = start;
      const uint8_t *end = at + frag->buf_length;
      while (at < end)
        {
          if (*at == '\r' && at_start_of_line)
            at++;
          else if (*at == '\n' && at_start_of_line)
            {
              at++;
              *n_bytes_out = discarded + (at - start);
              return ROSCO_HTTP_SCAN_STATUS_SUCCESS;
            }
          else if (*at == '\n' && !at_start_of_line)
            {
              at++;
              at_start_of_line = ROSCO_TRUE;
            }
          else
            { 
              at++;
              at_start_of_line = ROSCO_FALSE;
            }
        }
      discarded += frag->buf_length;
    }
  return ROSCO_HTTP_SCAN_STATUS_INCOMPLETE;
}
