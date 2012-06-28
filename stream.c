/* 
 * Memory Streams - Haran Shivanan (Dec,2002)
 * Portable (mostly) code and data structures for implementing generic
 * memory streams that are allocated on the heap and auto-
 * expand.
 * They are designed to be used like FILE streams -- kind of.
 *
 * TODO: Allow transparent access to FILE streams
 */
#include <stdio.h>
#include <string.h>
#include "stream.h"

static int stream_expand(STREAM * stream, size_t new_capacity)
{
	char *tmp = (char *)reallocate(stream->buffer, new_capacity);
	if (!tmp)
		return 0;
	stream->capacity = new_capacity;
	stream->buffer = tmp;
	return 1;
	
	
}

STREAM * stream_create (size_t size)
{
	STREAM *result = (STREAM *)allocate(sizeof(STREAM));
	result->size = 0;
	result->capacity = size;
	result->offset = 0;
	result->buffer = (char *)allocate(size);
	result->r_offset = 0;
	if (!result->buffer)
		return NULL;
	return result;
}



void stream_free(STREAM *stream)
{
	if (stream->buffer)
		deallocate(stream->buffer);
	deallocate(stream);
}

void stream_write_reset (STREAM * stream)
{
	stream->size = 0;
	stream->offset = 0;
}

size_t stream_write(STREAM *stream,const char *data,size_t len)
{
	int final_size;
	
	/*check if the internal buffer has enough space for the
	 * incomming data */
	final_size  = stream->offset+len;
	if (final_size > stream->capacity)
	{
		/*if we need to expand to more than twice the current size
		 * ,just expand by the required amount*/
		if (final_size > 2*stream->capacity)
			stream_expand(stream,final_size);
		else
		/* otherwise, double the capacity for every expansion */
			stream_expand(stream,2*stream->capacity);
	}
	memcpy(stream->buffer+stream->offset,data,len);
	stream->offset += len;
	if (final_size > stream->size)
		stream->size = final_size;
	return len;
}



/* Given an existing data buffer, create a stream from it. 
 * We don't attach the buffer to the stream, but rather make
 * a fresh copy. This allows us to safely free the stream
 * later on.
 */
STREAM * stream_create_from_buffer (const char * buffer, size_t buf_len)
{
	STREAM * result = stream_create (buf_len);
	stream_write (result, buffer, buf_len);
	return result;
}


/* TODO: Implement a stream_peek() operation */
char * stream_current_position (STREAM * stream)
{
	return stream->buffer + stream->r_offset;
}


int stream_read (STREAM * stream, char * buffer, size_t len)
{
	int offset = stream->r_offset;
	if (len+offset >= stream->size) return 0;
	if (offset > stream->size)
		return 0;
	memcpy (buffer, stream->buffer + offset, len);
	stream->r_offset += len;
	return len;
}

int stream_read_char (STREAM * stream, unsigned char * c)
{
	int offset = stream->r_offset;
	if (offset > stream->size)
		return 0;
	*c = stream->buffer[offset];
	stream->r_offset++;
	return 1;
}
/* same as stream_write but without the overhead
 * of memcpy(...)
 */
int stream_add_char(STREAM *stream,unsigned char ch)
{
	int final_size;
	
	/*check if the internal buffer has enough space for the
	 * incomming data */
	final_size  = stream->offset+1;
	if (final_size > stream->capacity)
	{
		/*if we need to expand to more than twice the current size
		 * ,just expand by the required amount*/
		if (final_size > 2*stream->capacity)
			stream_expand(stream,final_size);
		else
		/* otherwise, double the capacity for every expansion */
			stream_expand(stream,2*stream->capacity);
	}
	stream->buffer[stream->offset]=ch;
	stream->offset++;
	if (final_size > stream->size)
		stream->size = final_size;
	return 1;	
}

/* write integers in BIG-ENDIAN format */
int stream_write_int_reverse (STREAM * stream, int i, size_t int_size)
{
	int k,j=i, result=0;
	char * buffer = (char *)allocate (int_size);
	for (k=int_size;k>0;k--)
	{
		unsigned char b = j & 0xFF;
		buffer[k-1] = b;
		j>>=8;
	}
	result = stream_write (stream, buffer, 4);
	deallocate (buffer);
	return result;
}

char * stream_data(STREAM *stream)
{
	return stream->buffer;
}

char * stream_copy_buffer (STREAM * stream)
{
	char * r;
	if (stream->size < 1)
		return NULL;
	r = (char *)allocate(stream->size);
	memcpy (r, stream->buffer, stream->size);
	return r;
}


int stream_write_to_file (STREAM * stream, char * filename)
{
	FILE * s;
	int result;
	s = fopen (filename, "wb");
	result = fwrite (stream->buffer, 1, stream->size, s);
	fclose(s);
	return result;
}

int stream_write_to_io (STREAM * stream, FILE * io)
{
	return fwrite (stream->buffer, 1, stream->size, io);
}

/* read data from a FILE stream into the STREAM object */
int stream_copy_from_io (STREAM * stream, FILE * io)
{
	int size = 0, total = 0;
	char buffer[1024];
	while ( (size = fread (buffer, 1, 1023, io))) 
		total += stream_write (stream, buffer, size);

	return total;
}

STREAM * stream_load_from_file (char * filename)
{
	FILE * io;
	STREAM * result;
	io = fopen (filename, "rb");
	if (!io) return NULL;

	result = stream_create (1);
	stream_copy_from_io (result, io);
	fclose (io);
	return result;
	
}

#ifdef DO_TEST
int main(int argc, char **argv)
{
	STREAM *stream;
	stream = stream_create(10);
	while (argc-->0)
	{
		stream_write(stream,*argv,strlen(*argv));
		stream_write(stream,"\n",1);
		argv++;
	}
	stream->buffer[stream->size]=0;
	printf(stream->buffer);
}
#endif /* DO_TEST */
