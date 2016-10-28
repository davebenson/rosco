
typedef enum
{
  ROSCO_TYPE_BOOL,
  ROSCO_TYPE_INT8,
  ROSCO_TYPE_UINT8,
  ROSCO_TYPE_INT16,
  ROSCO_TYPE_UINT16,
  ROSCO_TYPE_INT32,
  ROSCO_TYPE_UINT32,
  ROSCO_TYPE_INT64,
  ROSCO_TYPE_UINT64,
  ROSCO_TYPE_TIME,
  ROSCO_TYPE_DURATION,
  ROSCO_TYPE_MESSAGE,
} RoscoType

typedef struct _RoscoType RoscoType;
struct _RoscoType
{
  char *name;
  char *ctype_name;
  void *descriptor;
};

struct _RoscoMessageDescriptorField
{
  char *name;
  RoscoType *type;
};

struct _RoscoMessageDescriptor
{
};
