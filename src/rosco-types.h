
#include <stdint.h>
#include <stdalign.h>

typedef uint8_t rosco_bool;

typedef struct RoscoTime
{
  uint32_t secs, nsecs;
} RoscoTime;
typedef struct RoscoDuration
{
  int32_t secs, nsecs;
} RoscoDuration;

