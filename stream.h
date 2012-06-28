#ifndef _STREAM_H_
#define _STREAM_H_
/* */
#include <stdlib.h>
#include <stdio.h>

/* allocate, deallocate and reallocate are the memory management functions
 * used by the stream functions. They are to be defined as macros.
 * That way, if you don't explicitly define them, the standard C malloc
 * library will be used.
 *
 * This allows you to provide your own allocation functions
 */


/*Define our own memory management functions here*/
#include "util.h"
#define allocate xmalloc
#define deallocate xfree
#define reallocate xrealloc

#ifndef allocate
#include <malloc.h>
#define MALLOC_H_DEF
#define allocate(x) malloc(x)
#endif

#ifndef deallocate

#ifndef MALLOC_H_DEF
#include <malloc.h>
#endif /*MALLLOC_H_DEF*/

#define deallocate(x) free(x)
#endif /* deallocate */

#ifndef reallocate

#ifndef MALLOC_H_DEF
#include <malloc.h>
#endif /* MALLOC_DEF_H */

#define reallocate(x,y) realloc(x,y)
#endif /* reallocate */



typedef struct _STREAM
{
	size_t capacity; /* actual capacity of the buffer */
	size_t size; /* the largest offset written to in the buffer */
	int offset; /* current write offset into the buffer */
	char * buffer; /* actual data buffer */
	int r_offset; /* current read offset into the buffer */
}STREAM;

/* stream manipulation functions */
STREAM*stream_create 			(size_t len);
void   stream_free     			(STREAM * stream);
void   stream_write_reset       (STREAM * stream);
size_t stream_write    			(STREAM * stream, const char * data, size_t len);
STREAM * stream_create_from_buffer (const char * buffer, size_t buf_len);
char * stream_current_position  (STREAM * stream);
int    stream_read              (STREAM * stream, char * buffer, size_t len);
int    stream_read_char         (STREAM * stream, unsigned char * c);
int    stream_add_char 			(STREAM * stream, unsigned char ch);
int    stream_write_int_reverse (STREAM * stream, int i, size_t int_Size);
char * stream_data     			(STREAM * stream);
char * stream_copy_buffer 		(STREAM * stream);
int    stream_write_to_file     (STREAM * stream, char * filename);
int    stream_write_to_io       (STREAM * stream, FILE * io);
int    stream_copy_from_io      (STREAM * stream, FILE * io);
STREAM * stream_load_from_file  (char * filename);
#define stream_add_str(stream,data) stream_write(stream,data,strlen(data))

#define stream_end(s) (s->r_offset >= s->size)

#endif /* _STREAM_H_ */
