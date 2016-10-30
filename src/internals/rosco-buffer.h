/* invariant:  if a buffer.size==0, then first_frag/last_frag == NULL.
   corollary:  if a buffer.size==0, then the buffer is using no memory. */

typedef struct _RoscoBuffer RoscoBuffer;
typedef struct _RoscoBufferFragment RoscoBufferFragment;

struct _RoscoBufferFragment
{
  RoscoBufferFragment    *next;
  uint8_t              *buf;
  unsigned              buf_max_size;	/* allocation size of buf */
  unsigned              buf_start;	/* offset in buf of valid data */
  unsigned              buf_length;	/* length of valid data in buf; != 0 */
  
  rosco_boolean           is_foreign;
  RoscoDestroyNotify      destroy;
  void                 *destroy_data;
};

struct _RoscoBuffer
{
  unsigned              size;

  RoscoBufferFragment    *first_frag;
  RoscoBufferFragment    *last_frag;
};

#define ROSCO_BUFFER_INIT		{ 0, NULL, NULL }


void     rosco_buffer_init                (RoscoBuffer       *buffer);

unsigned rosco_buffer_read                (RoscoBuffer    *buffer,
                                           unsigned      max_length,
                                           void         *data);
unsigned rosco_buffer_peek                (const RoscoBuffer* buffer,
                                           unsigned      max_length,
                                           void         *data);
int      rosco_buffer_discard             (RoscoBuffer    *buffer,
                                           unsigned      max_discard);
char    *rosco_buffer_read_line           (RoscoBuffer    *buffer);

char    *rosco_buffer_parse_string0       (RoscoBuffer    *buffer);
                        /* Returns first char of buffer, or -1. */
int      rosco_buffer_peek_byte           (const RoscoBuffer *buffer);
int      rosco_buffer_read_byte           (RoscoBuffer    *buffer);

uint8_t  rosco_buffer_byte_at             (RoscoBuffer    *buffer,
                                           unsigned      index);
uint8_t  rosco_buffer_last_byte           (RoscoBuffer    *buffer);
/* 
 * Appending to the buffer.
 */
void     rosco_buffer_append              (RoscoBuffer    *buffer, 
                                           unsigned      length,
                                           const void   *data);

ROSCO_INLINE_FUNC void rosco_buffer_append_small(RoscoBuffer    *buffer, 
                                           unsigned      length,
                                           const void   *data);
void     rosco_buffer_append_string       (RoscoBuffer    *buffer, 
                                           const char   *string);
ROSCO_INLINE_FUNC void rosco_buffer_append_byte(RoscoBuffer    *buffer, 
                                           uint8_t       byte);
void     rosco_buffer_append_byte_f       (RoscoBuffer    *buffer, 
                                           uint8_t       byte);
void     rosco_buffer_append_repeated_byte(RoscoBuffer    *buffer, 
                                           size_t        count,
                                           uint8_t       byte);
#define rosco_buffer_append_zeros(buffer, count) \
  rosco_buffer_append_repeated_byte ((buffer), 0, (count))


void     rosco_buffer_append_string0      (RoscoBuffer    *buffer,
                                           const char   *string);

void     rosco_buffer_append_foreign      (RoscoBuffer    *buffer,
					   unsigned      length,
                                           const void   *data,
					   RoscoDestroyNotify destroy,
					   void         *destroy_data);

void     rosco_buffer_printf              (RoscoBuffer    *buffer,
					   const char   *format,
					   ...) ROSCO_GNUC_PRINTF(2,3);
void     rosco_buffer_vprintf             (RoscoBuffer    *buffer,
					   const char   *format,
					   va_list       args);

uint8_t  rosco_buffer_get_last_byte       (RoscoBuffer    *buffer);
uint8_t  rosco_buffer_get_byte_at         (RoscoBuffer    *buffer,
                                           size_t        idx);


/* --- appending data that will be filled in later --- */
typedef struct {
  RoscoBuffer *buffer;
  RoscoBufferFragment *fragment;
  unsigned offset;
  unsigned length;
} RoscoBufferPlaceholder;

void     rosco_buffer_append_placeholder  (RoscoBuffer    *buffer,
                                           unsigned      length,
                                           RoscoBufferPlaceholder *out);
void     rosco_buffer_placeholder_set     (RoscoBufferPlaceholder *placeholder,
                                           const void       *data);

/* --- buffer-to-buffer transfers --- */
/* Take all the contents from src and append
 * them to dst, leaving src empty.
 */
unsigned rosco_buffer_drain               (RoscoBuffer    *dst,
                                           RoscoBuffer    *src);

/* Like `drain', but only transfers some of the data. */
unsigned rosco_buffer_transfer            (RoscoBuffer    *dst,
                                           RoscoBuffer    *src,
					   unsigned      max_transfer);

/* --- file-descriptor mucking --- */
int      rosco_buffer_writev              (RoscoBuffer       *read_from,
                                           int              fd);
int      rosco_buffer_writev_len          (RoscoBuffer *read_from,
		                           int              fd,
		                           unsigned         max_bytes);
/* returns TRUE iff all the data was written.  'read_from' is blank. */
rosco_boolean rosco_buffer_write_all_to_fd  (RoscoBuffer       *read_from,
                                           int              fd,
                                           RoscoError       **error);
int      rosco_buffer_readv               (RoscoBuffer       *write_to,
                                           int              fd);

/* --- deallocating memory used by buffer --- */

/* This deallocates memory used by the buffer-- you are responsible
 * for the allocation and deallocation of the RoscoBuffer itself. */
void     rosco_buffer_clear               (RoscoBuffer    *to_destroy);

/* Same as calling clear/init */
void     rosco_buffer_reset               (RoscoBuffer    *to_reset);

/* Return a string and clear the buffer. */
char *rosco_buffer_empty_to_string (RoscoBuffer *buffer);

/* --- iterating through the buffer --- */
/* 'frag_offset_out' is the offset of the returned fragment in the whole
   buffer. */
RoscoBufferFragment *rosco_buffer_find_fragment (RoscoBuffer   *buffer,
                                                 unsigned     offset,
                                                 unsigned    *frag_offset_out);

/* Free all unused buffer fragments. */
void     _rosco_buffer_cleanup_recycling_bin ();


/* misc */
int rosco_buffer_index_of(RoscoBuffer *buffer, char char_to_find);

unsigned rosco_buffer_fragment_peek (RoscoBufferFragment *fragment,
                                     unsigned           offset,
                                     unsigned           length,
                                     void              *buf);
rosco_boolean rosco_buffer_fragment_advance (RoscoBufferFragment **frag_inout,
                                             unsigned           *offset_inout,
                                             unsigned            skip);

/* HACKS */
/* NOTE: the buffer is INVALID after this call, since no empty
   fragments are allowed.  You MUST deal with this if you do 
   not actually add data to the buffer */
void rosco_buffer_append_empty_fragment (RoscoBuffer *buffer);

void rosco_buffer_maybe_remove_empty_fragment (RoscoBuffer *buffer);

/* a way to delete the fragment from rosco_buffer_append_empty_fragment() */
void rosco_buffer_fragment_free (RoscoBufferFragment *fragment);


#if ROSCO_CAN_INLINE || defined(ROSCO_IMPLEMENT_INLINES)
ROSCO_INLINE_FUNC void rosco_buffer_append_small(RoscoBuffer    *buffer, 
                                                 unsigned      length,
                                                 const void   *data)
{
  RoscoBufferFragment *f = buffer->last_frag;
  if (f != NULL
   && !f->is_foreign
   && f->buf_start + f->buf_length + length <= f->buf_max_size)
    {
      uint8_t *dst = f->buf + (f->buf_start + f->buf_length);
      const uint8_t *src = data;
      f->buf_length += length;
      buffer->size += length;
      while (length--)
        *dst++ = *src++;
    }
  else
    rosco_buffer_append (buffer, length, data);
}
ROSCO_INLINE_FUNC void rosco_buffer_append_byte(RoscoBuffer    *buffer, 
                                                uint8_t       byte)
{
  RoscoBufferFragment *f = buffer->last_frag;
  if (f != NULL
   && !f->is_foreign
   && f->buf_start + f->buf_length < f->buf_max_size)
    {
      f->buf[f->buf_start + f->buf_length] = byte;
      ++(f->buf_length);
      buffer->size += 1;
    }
  else
    rosco_buffer_append (buffer, 1, &byte);
}

#endif
