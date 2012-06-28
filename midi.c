/* 
 * HShivanan (June 2003)
 * self-contained code for reading and writing midi files
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#define assert_unreachable() assert(0)

#include "midi.h"
#include "stream.h"
#include "util.h"


const unsigned char MIDI_VOICE_EVENTS[VOICE_EVENT_COUNT][4] = {
	/*
	{bytes of data, event value, maxval for data1, maxval for data2}
	 */
	{2, 0x8, 0x7F, 0x7F}, /* note off */
	{2, 0x9, 0x7F, 0x7F}, /* note on  */
	{2, 0xA, 0x7F, 0x7F}, /* polyphonic pressure */
	{2, 0xB, 0x77, 0x7F}, /* controller change */
	{1, 0xC, 0x7F,    0}, /* program change */
	{1, 0xD, 0x7F,    0}, /* channel pressure */
	{2, 0xE, 0x7F, 0x7F}  /* pitch bend */
	
};



/* Meta Event of type 0x21 keeps showing up in midi files but
 * I havn't found it's semantic meaning */
const unsigned char MIDI_META_EVENTS[META_EVENT_COUNT][2] = {
	/* type, length (0xFF=arbitrary length, 0x0 implies no proceeding data */
	{0x0,  0x2},
	{0x1,  0xFF}, /* text */
	{0x2,  0xFF}, /* copyright */
	{0x3,  0xFF}, /* track name */
	{0x4,  0xFF}, /* instrument name */
	{0x5,  0xFF}, /* lyric */
	{0x6,  0xFF}, /* marker */
	{0x7,  0xFF}, /* cue */
	{0x20, 0x1},  /* channel prefix */
	{0x2F, 0x0},  /* eot */
	{0x51, 0x3},  /* set tempo */
	{0x54, 0x5},  /* smtpe offset */
	{0x58, 0x4},  /* time signature */
	{0x59, 0x2},  /* key signature */
	{0x7F, 0xFF}  /* specific */
};



const unsigned char MIDI_MODE_EVENTS[MODE_EVENT_COUNT][2] = {

	{0x78, 0},    /* sound off */
	{0x79, 0},    /* reset controllers */
	{0x7A, 0x7F}, /* local control */
	{0x7B, 0},    /* notes off */
	{0x7C, 0},    /* omni off */
	{0x7D, 0},    /* omni on */
	{0x7E, 0x10}, /* mono on */
	{0x7F, 0}     /* poly on */
} ;



const char * MIDI_ERROR_STR[0xe] = {
	NULL,
	"Unknown Meta Event:%#x\n",                          /* MIDI_ERROR_UNKOWN_META_EVENT */
	"Chunk Validation failed. Chunk contains no date\n", /* MIDI_ERROR_CHUNK_EMPTY */
	"Chunk Validation failed. Invalid type\n",           /* MIDI_ERROR_CHUNK_TYPE */
	"Voice Data 1 is out of bounds:%#x %#x\n",           /* MIDI_ERROR_VOICE_1 */
	"Voice Data 2 is out of bounds:%#x %#x\n",           /* MIDI_ERROR_VOICE_2 */
	"Error decoding Voice Event: %#x\n",                 /* MIDI_ERROR_UNKNOWN_VOICE */
	"Error decoding Mode Event: %#x\n",                  /* MIDI_ERROR_UNKNOWN_MODE */
	"Unknown midi event encountered:%i\n",               /* MIDI_ERROR_UNKNOWN_EVENT */
	"End Of Track marker not found\n",                   /* MIDI_ERROR_NO_EOT */
	"Track Chunk expected but not found\n",              /* MIDI_ERROR_NOT_TRACK_CHUNK */
	"Header Chunk expected but not found\n",             /* MIDI_ERROR_NOT_HEADER */
	"Error reading Header Chunk\n",                      /* MIDI_ERROR_HEADER */
	"File format error\n"                                /* MIDI_ERROR_BAD_FORMAT */
};



/* To write a custom error handler, define a function of the form:
 * error_report(int,...)
 * and then set midi_error_fun to point to this function.
 * All generated errors will then call your custom function (error_report
 * in this case)
 */
static void (*midi_error_fun)(int ,...) = NULL;


void printe(const char * text,...)
{
	va_list args;
	va_start (args, text);
	if (midi_error_fun)
		midi_error_fun (0, args);
	else
		vfprintf (stderr, text, args);
	va_end (args);
}




/* try not to choke on this */
static int stream_write_variable (STREAM * stream, unsigned int i)
{
	STREAM * hack;
	unsigned int j = i;
	unsigned char x;
	hack = stream_create(12);
	x = (j & 0x7F);
	stream_add_char (hack, x);
	j >>= 7;
	while (j) {
		x = j & 0x7F;
		j >>= 7;
		x = x|0x80;
		stream_add_char (hack, x);
	}
	for (j=hack->size;j>0;j--)
		stream_add_char (stream, hack->buffer[j-1]);
	stream_free (hack);
	return 1;
}


static unsigned long stream_read_variable (STREAM * stream)
{
	long result = 0;
	unsigned char b;
	stream_read_char (stream, &b);
	while (1) {
		result = (result<<7) + (b & 0x7F);
		if (!(b & 0x80))
			return result;
		stream_read_char (stream, &b);
	}
}


static int write_int_reverse (char * buffer, int i, size_t int_size)
{
	int k,j=i;
	for (k=int_size;k>0;k--)
	{
		unsigned char b = j & 0xFF;
		buffer[k-1] = b;
		j>>=8;
	}
	return 1;
}

void free_event_list (MIDI_EVENT * event)
{
	MIDI_EVENT * next;
	assert (event);
	next = event->next;
	switch (event->type)
	{
		case EVENT_TYPE_META:
			xfree (event->event.meta_event);
			break;
		case EVENT_TYPE_SYSEX:
			xfree (event->event.sysex_event);
			break;
		case EVENT_TYPE_VOICE:
			xfree (event->event.voice_event);
			break;
		case EVENT_TYPE_MODE:
			xfree (event->event.mode_event);
			break;
		default: assert_unreachable();
			
	}
	xfree (event);
	if (next)
		free_event_list (next);
}

/* determine the specific voice event from the first byte */
static unsigned char voice_event_type (unsigned char c)
{
	if (c> 0xE)
		return VOICE_EVENT_UNKNOWN;
	return (c-0x8);
}

static unsigned char mode_event_type (unsigned char c)
{
	unsigned char d = c- 0x78;
	if (d>=MODE_EVENT_COUNT)
		return MODE_EVENT_UNKNOWN;
	return d;
}
static unsigned char meta_event_type (unsigned char c)
{
	int i;
	for (i=0;i<META_EVENT_COUNT;i++)
		if	(MIDI_META_EVENTS[i][0]==c)
			return (unsigned char)i;
	printe("Unknown meta event type:%#x\n",c);
	return META_EVENT_UNKNOWN;
}


/* Meta events begin with 0xFF
 * Sysex events begin with 0xF0 or 0xF7
 * Voice events begin with anything from 0x8n to 0xEn
 * Mode events begin with 0xBn and have the next byte greater than 0x77
 */
static int get_event_type (char * buffer)
{
	unsigned char b = buffer[0];
	if (b>=0x80 && b<=0xEF) {
		if (b>>4 == 0xB && buffer[1] > 0x77)
			return EVENT_TYPE_MODE;
		else
			return EVENT_TYPE_VOICE;
		
	} else if (b==0xF0 || b==0xF7) {
		return EVENT_TYPE_SYSEX;
		
	} else if (b==0xFF) return EVENT_TYPE_META;
	
	return EVENT_TYPE_UNKNOWN;
}

int validate_chunk (MIDI_CHUNK * mc)
{
	if (!mc->data) {
		printe ("Chunk Validation failed. Chunk contains no data\n");
		return 0;
	}
	if (mc->type != HEADER_CHUNK && mc->type != TRACK_CHUNK) {
		printe ("Chunk validation failed. Invalid type\n");
		return 0;
	}
	return 1;
}

void encode_event_voice (MIDI_TRACK * mt, VOICE_EVENT * event)
{
	const unsigned char * info;
	assert (event->type < VOICE_EVENT_COUNT);
	info = MIDI_VOICE_EVENTS[event->type];

	/* XXX:Should we allow data larger than the legal value to be encoded? */
	if (!(event->data1 <= info[2])) {
		printe ("Voice data 1 is larger than legal limit:%#x %#x\n",event->type, event->data1);
	}

	if (info[0] == 2 && (event->data2>info[3]))
		printe ("Voice data 2 is larger than legal limit:%#x %#x\n",event->type, event->data2);

	stream_add_char (mt->stream, (info[1]<<4)|event->channel);
	stream_add_char (mt->stream, event->data1);
	
	if (info[0] == 2)
		stream_add_char (mt->stream, event->data2);
}
void encode_event_mode (MIDI_TRACK * mt, MODE_EVENT * event)
{
	const unsigned char * info;
	assert (event->type != MODE_EVENT_UNKNOWN);
	assert (event->channel <= 0xF);
	
	stream_add_char (mt->stream, 0xB0|event->channel);
	info = MIDI_MODE_EVENTS[event->type];
	stream_add_char (mt->stream, info[0]);
	
	if (info[1] == 0)
		stream_add_char (mt->stream, info[1]);
	else {
		assert (event->data <= info[1]);
		stream_add_char (mt->stream, info[1]);
	}
}
void encode_event_meta (MIDI_TRACK * mt, META_EVENT * event)
{
	const unsigned char * info;
	if (event->type < META_EVENT_COUNT) { 
		info = MIDI_META_EVENTS[event->type];
	
		if (info[1] != 0xFF) /* if we don't have a variable length value */
			assert ((unsigned char)event->length == info[1]);
		stream_add_char (mt->stream, 0xFF);
		stream_add_char (mt->stream, info[0]);

	} else {
		printe ("Writing unknown meta event:%#x\n",event->unknown_type);
		stream_add_char (mt->stream, 0xFF);
		stream_add_char (mt->stream, event->unknown_type);
	}
	
	stream_write_variable (mt->stream, event->length);
	if (event->length)
		stream_write (mt->stream, event->data, event->length);
}
void encode_event_meta_eot (MIDI_TRACK * mt)
{
	stream_add_char (mt->stream, 0x00);
	stream_add_char (mt->stream, 0xFF);
	stream_add_char (mt->stream, 0x2F);
	stream_add_char (mt->stream, 0x00);
}
void encode_event_sysex (MIDI_TRACK * mt, SYSEX_EVENT * event)
{
	assert (event->type == 0xF0 || event->type == 0xF7);
	stream_add_char (mt->stream, event->type);
	stream_write_variable (mt->stream, event->length);
	stream_write (mt->stream, event->data, event->length);
}

#define readstream(k) if (!stream_read_char(mt->stream,(k))) return 0


/* for the decode_event_* routines, the decoding routine itself will allocate the appropriate
 * member in the MIDI_EVENT structure -- Any pre allocated block will thus be lost 
 * TODO: Implement decoders using callback functions -- More efficient and no
 * memory allocation overhead
 */
int decode_event_voice (MIDI_TRACK * mt, MIDI_EVENT * event, unsigned char status)
{
	unsigned char k, type;
	VOICE_EVENT * ve;
	if (status==0){
		readstream (&k);
	} else
		k = status;
	type = voice_event_type (k>>4);
	if (type == VOICE_EVENT_UNKNOWN) {
		printe ("Error decoding voice event:%#x\n",k);
		return 0;
	}
	ve = xmalloc(sizeof(VOICE_EVENT));
	if (!ve)
		return 0; /* xmalloc failed */
	ve->type = type;
	event->type = EVENT_TYPE_VOICE; 
	ve->channel = k & 0xF;
	readstream (&k);
	ve->data1 = k;
	if (MIDI_VOICE_EVENTS[ve->type][0] == 2) {
		readstream (&k);
		ve->data2 = k;
	}
	event->event.voice_event = ve;
	return 1;
}
int decode_event_mode (MIDI_TRACK * mt, MIDI_EVENT * event, unsigned char status)
{
	unsigned char k, j, type;
	MODE_EVENT * me;
	if (status == 0) {
		readstream(&k);
	} else
		k = status;
	readstream (&j);
	type = mode_event_type (j);
	if (type == MODE_EVENT_UNKNOWN) {
		printe ("Error decoding mode event:%#x:%#x\n",k,j);
		return 0;
	}
	me = xmalloc(sizeof(MODE_EVENT));
	if (!me) 
		return 0;
	me->type = type;
	event->type = EVENT_TYPE_MODE;
	me->channel = k & 0xF;
	readstream(&k);
	me->data = k;
	event->event.mode_event = me;
	return 1;
}
int decode_event_meta (MIDI_TRACK * mt, MIDI_EVENT * event)
{
	unsigned char k;
	unsigned long len;
	META_EVENT * me;
	readstream(&k);
	assert (k == 0xFF);
	me = xmalloc (sizeof(META_EVENT));
	if (!me)
		return 0;
	me->data = NULL;
	event->type = EVENT_TYPE_META;
	event->event.meta_event = me;
	readstream(&k);
	me->type = meta_event_type (k); 
	if (me->type == META_EVENT_UNKNOWN)
		me->unknown_type = k;
	len = stream_read_variable(mt->stream);
	me->length = len;
	if (!me->length)
		return 1;
	me->data = xmalloc(len);
	if (!me->data)
		return 0;
	return stream_read (mt->stream, me->data, len);
}
int decode_event_sysex (MIDI_TRACK * mt, MIDI_EVENT * event)
{
	SYSEX_EVENT * se;
	unsigned char k;
	readstream(&k);
	assert (k==0xF0 || k==0xF7);
	se = xmalloc(sizeof(SYSEX_EVENT));
	if (!se)
		return 0;
	se->data = NULL;
	event->type = EVENT_TYPE_SYSEX;
	event->event.sysex_event = se;
	se->type = k;
	se->length = stream_read_variable (mt->stream);
	if (!se->length) 
		return 1;
	se->data = xmalloc(se->length);
	if (!se->data) return 0;
	return stream_read (mt->stream, se->data, se->length);
}
int encode_event (MIDI_TRACK * mt, MIDI_EVENT * event)
{
	switch (event->type) {
		case EVENT_TYPE_VOICE:
			{
				VOICE_EVENT * ve = event->event.voice_event;
				stream_write_variable (mt->stream, event->delta_time);
				encode_event_voice (mt, ve);
				return 1;
			}
		case EVENT_TYPE_MODE:
			{
				MODE_EVENT * me = event->event.mode_event;
				stream_write_variable (mt->stream, event->delta_time);
				encode_event_mode (mt, me);
				return 1;
			}
		case EVENT_TYPE_META:
			{
				META_EVENT * me = event->event.meta_event;
				stream_write_variable (mt->stream, event->delta_time);
				encode_event_meta (mt, me);
				return 1;
			}
		case EVENT_TYPE_SYSEX:
			{
				SYSEX_EVENT * se = event->event.sysex_event;
				stream_write_variable (mt->stream, event->delta_time);
				encode_event_sysex (mt, se);
				return 1;
			}
		default:
			return 0;
	}
}



/*The following encoder routines are just wrappers around the encode_event
 * function */

/* TODO: also handle running status */
int encode_voice (MIDI_TRACK * mt, unsigned long delta_time,
						unsigned char channel, unsigned char type,
						unsigned char data1, unsigned char data2)
{
	MIDI_EVENT event;
	VOICE_EVENT ve;
	event.delta_time = delta_time;
	event.type = EVENT_TYPE_VOICE;
	ve.channel = channel;
	ve.type = type;
	ve.data1 = data1;
	ve.data2 = data2;
	event.event.voice_event = &ve;
	return encode_event (mt, &event);
}

int encode_meta (MIDI_TRACK * mt, unsigned long delta_time, unsigned char type,
		         unsigned char unknown_type, unsigned long length,
				 unsigned char * data)
{
	MIDI_EVENT event;
	META_EVENT me;
	event.delta_time = delta_time;
	event.type = EVENT_TYPE_META;
	me.type = type;
	me.unknown_type = unknown_type;
	me.length = length;
	me.data = data;
	event.event.meta_event = &me;
	return encode_event (mt, &event);
}
int encode_sysex (MIDI_TRACK * mt, unsigned long delta_time, unsigned char type,
		          unsigned long length, unsigned char * data)
{
	MIDI_EVENT event;
	SYSEX_EVENT se;
	event.delta_time = delta_time;
	event.type = EVENT_TYPE_SYSEX;
	se.type = type;
	se.length = length;
	se.data = data;
	event.event.sysex_event = &se;
	return encode_event (mt, &event);
}

int encode_mode (MIDI_TRACK * mt, unsigned long delta_time, unsigned char type,
		         unsigned char channel, unsigned char data)
{
	MIDI_EVENT event;
	MODE_EVENT me;
	event.delta_time = delta_time;
	event.type = EVENT_TYPE_MODE;
	me.type = type;
	me.channel = channel;
	me.data = data;
	event.event.mode_event = &me;
	return encode_event (mt, &event);
}


MIDI_EVENT * parse_track_event (MIDI_TRACK * mt)
{
	int e_type;
	static int running = 0;
	static unsigned char status;
	MIDI_EVENT * event = xmalloc(sizeof(MIDI_EVENT));
	if (!event) return NULL;
	event->delta_time = stream_read_variable (mt->stream);

	/* inspect the next couple of bytes without actually reading it
	 * from the stream in order to determine the type of event*/
	e_type = get_event_type (stream_current_position(mt->stream));
	switch (e_type) {
		case EVENT_TYPE_SYSEX:
			if (!decode_event_sysex (mt, event))
				return NULL;
			running = 0;
			return event;
			break;
			
		case EVENT_TYPE_UNKNOWN:
			if ((running == 1) || (  (running ==2) && *(stream_current_position(mt->stream))  <=0x77)  ) {
				assert (voice_event_type (status>>4) != VOICE_EVENT_UNKNOWN);
				if (!decode_event_voice (mt, event, status))
					return NULL;
				return event;
			} else if (running == 2) {
				if (!decode_event_mode (mt, event, status))
					return NULL;
				return event;
			}
			printe ("Error:Unknown midi event:%i\n",e_type);
			printe ("%i:%i\t:%#x\n",mt->stream->r_offset, mt->stream->size,*(mt->stream->buffer+mt->stream->r_offset));
			return NULL;
			break;

		case EVENT_TYPE_MODE:
			if (!decode_event_mode (mt, event, 0))
				return NULL;
			status = (0xB0)|(event->event.mode_event->channel); 
			running = 2;
			return event;
			break;

		case EVENT_TYPE_META:
			if (!decode_event_meta (mt, event))
				return NULL;
			running = 0;
			return event;
			break;

		case EVENT_TYPE_VOICE:
			if (!decode_event_voice (mt, event, 0))
				return NULL;
			status = (MIDI_VOICE_EVENTS[(event->event.voice_event->type)][1]<<4)|(event->event.voice_event->channel);
			running = 1;
			return event;
			break;

		default:
			return NULL;
	}
	
}

MIDI_EVENT * parse_track_events (MIDI_TRACK * mt)
{
	MIDI_EVENT * event, * base = NULL, *event2 = NULL;
	int first = 1;
	while (1) {
		event = parse_track_event (mt);
		if (!event)
			break;
		if (first) {
			base = event2 = event;
			first = 0;
		} else {
			event2->next = event;
			event2 = event;
		}

		/* break when we reach the end of the stream
		 * and verify that we read exactly as many bytes as we
		 * had to */
		if (stream_end (mt->stream)) {
			assert (mt->stream->size == mt->stream->r_offset);
			break;
		}

	}
	event2->next = NULL;

	/* make sure the last event decoded was the End-Of-Track meta event */
	if  (event2->type == EVENT_TYPE_META && event2->event.meta_event->type == META_EVENT_EOT)
	{
	}else {
		printe ("EOT marker not found\n");
	}
	return base;
}


/* Write a chunk to a pre-allocated buffer.
 * The size of the buffer = chunk_size (chunk)
 */
int write_chunk (char * buffer, MIDI_CHUNK * chunk)
{
	if (!validate_chunk (chunk))
		return 0;
	
	if (chunk->type == HEADER_CHUNK)
		memcpy (buffer, "MThd", 4);
	else if (chunk->type == TRACK_CHUNK)
		memcpy (buffer, "MTrk", 4);

	write_int_reverse (buffer+4, chunk->length, 4);
	memcpy (buffer+8, chunk->data, chunk->length);
	return 1;
}



int write_chunk_to_stream (STREAM * stream, MIDI_CHUNK * chunk)
{
	if (!validate_chunk (chunk)) return 0;

	if (chunk->type == HEADER_CHUNK)
		stream_write (stream, "MThd", 4);
	else if (chunk->type == TRACK_CHUNK)
		stream_write (stream, "MTrk", 4);

	stream_write_int_reverse (stream, chunk->length, 4);
	stream_write (stream, chunk->data, chunk->length);
	return 1;
}


int write_track_chunk (STREAM * stream, MIDI_TRACK * mt)
{
	MIDI_CHUNK chunk;
	int result;
	if (!make_track_chunk(mt, &chunk)) return 0;
	result = write_chunk_to_stream (stream, &chunk);
	xfree (chunk.data);
	return result;
}

int write_header_chunk (STREAM * stream, MIDI_FILE * mf)
{
	MIDI_CHUNK chunk;
	int result;
	if (!make_header_chunk (mf, &chunk)) return 0;
	result = write_chunk_to_stream (stream, &chunk);
	xfree (chunk.data);
	return result;
}


/* take an allocated but unitialized chunk and fill it */
int make_header_chunk (MIDI_FILE * mf, MIDI_CHUNK * chunk)
{
	chunk->type = HEADER_CHUNK;
	chunk->length = 6;
	chunk->data = xmalloc(6);
	if (!chunk->data) return 0;
	assert (mf->division == DIVISION_TQN || mf->division == DIVISION_TPF);
	write_int_reverse (chunk->data, mf->format, 2);
	write_int_reverse (chunk->data+2, mf->tracks, 2);
	if (mf->division == DIVISION_TQN) {
		assert (! (mf->tpqn & 0x8000));
		write_int_reverse (chunk->data+4, mf->tpqn, 2);
	} else if (mf->division == DIVISION_TPF) {
		assert (mf->fps < 0x80);
		chunk->data[4] = 0x80|(unsigned char)mf->fps;
		chunk->data[5] = (unsigned char)mf->tpf;
	}
	return 1;
}

int make_track_chunk (MIDI_TRACK * mt, MIDI_CHUNK * chunk)
{
	chunk->type = TRACK_CHUNK;
	chunk->length = mt->stream->size;
	chunk->data = stream_copy_buffer (mt->stream);
	return 1;
}

/* read a chunk -- XXX:only Track and Header chunks are supported */
int read_chunk (MIDI_FILE * mf, MIDI_CHUNK * chunk)
{
	if (memcmp(mf->buffer, "MThd", 4)==0)
		chunk->type = HEADER_CHUNK;
	else if (memcmp(mf->buffer, "MTrk", 4)==0)
		chunk->type = TRACK_CHUNK;
	else {
		chunk->type = UNKNOWN_CHUNK;
		return 0;
	}
	
	chunk->length = read32(mf->buffer+4);
	if (!(chunk->length))
		return 0;
	
	if (chunk->data)
		xfree (chunk->data);
	
	chunk->data = xmalloc (chunk->length);
	if (!chunk->data)return 0;
	memcpy (chunk->data, mf->buffer+8, chunk->length);
	mf->buffer += 8 + chunk->length;
	return 1;
}

MIDI_EVENT *  parse_track_chunk (MIDI_CHUNK * chunk, MIDI_TRACK * mt)
{
	if (chunk->type != TRACK_CHUNK) {
		printe ("Track Chunk expected but not found\n");
		return NULL;
	}

	if (mt->stream)
		stream_free (mt->stream);
	mt->stream = stream_create (5);
	stream_write (mt->stream, chunk->data, chunk->length);	
	return (parse_track_events (mt));
}

int parse_header_chunk (MIDI_FILE * mf)
{
	MIDI_CHUNK chunk;
	char * buffer;
	unsigned int divisions;

	chunk.data = NULL;
	if (!read_chunk(mf,&chunk)) {
		printe ("Error reading header chunk\n");
		return 0;
	}
	if (chunk.type != HEADER_CHUNK) {
		printe ("Header chunk expected but not found\n");
		return 0;
	}
	
	buffer = chunk.data;
	mf->format = read16(buffer);
	mf->tracks = read16(buffer+2);

	if (mf->format==0 && mf->tracks !=1) {
		printe ("Error: format error\n");
		return 0;
	}
	
	divisions = read16(buffer+4);
	if (divisions & 0x8000) {
		mf->division = DIVISION_TPF;
		mf->tpf = divisions & 0xFF;
		mf->fps = (divisions & 0x7F00)>>8;
		switch (mf->fps) {
			case 29: mf->fps = 30;
			case 24:case 25:case 30: /* valid fps values*/
			default:
					 printf("Error in fps:%i\n",mf->fps);
					 return 0;
		}
		
	} else {
		mf->division = DIVISION_TQN;
		mf->tpqn = divisions & 0x7FFF;
	}

	return 1;
	
}
#ifdef DO_MAIN
void print_midi_file (MIDI_FILE * mf)
{
	printf("Format:\t%i\nTracks:\t%i\n",mf->format,mf->tracks);
	if (mf->division == DIVISION_TQN)
		printf ("Division: Ticks Per Quarter Note (%i)\n",mf->tpqn);
	else {
		printf ("Division: Ticks Per SMTPE Frame (%i)\n",mf->tpf);
		printf ("          SMTPE Frames per second (%i)\n",mf->fps);
	}
}
void print_octaves(signed int amount)
{
	char o = '+';
	int octave = amount;
	fprintf(stderr,"octaves:%i\n",amount);
	if (amount <0) {
		octave = -amount;
		o='-';
	}
	for(;octave--;)printf("%c",o);
}
void print_commans(int amount)
{
	int i=0;
	for (i=0;i<amount/60;++i)
		printf(",");
}
void doit(MIDI_FILE * _m,MIDI_EVENT ** events)
{
	MIDI_FILE mf;
	MIDI_TRACK mt;
	STREAM *s1;
	FILE * stream;
	MIDI_EVENT * event;
	int j= 0;
	unsigned long time = 0,prev_time = 0,prev=0;
	
	mf = *_m;
	s1 = stream_create (10);
	write_header_chunk (s1, &mf);
	printf ("Header chunk written\n");
	for (;events[0];events++) {
		event = events[0];
		mt.stream = stream_create(6);
		while (event) {
			if (event->type == EVENT_TYPE_VOICE) {
				signed char k;
				signed char o;
				VOICE_EVENT * ve;
				static char notes[12] = {'S','r','R','g','G','m','M','P','d','D','n','N'};
				char s[10] = "";
				ve = event->event.voice_event;
				k = ve->data1 - 0x3c;
				//int base = k % 12;
				int base = ve->data1 % 12;
				if (base >11 || base < 0){
					fprintf(stderr,"Error--**--:%i\n",base);
				}
				k = k - (base%12);
				if (k/12 > 0) {
					o = '+';
				}
				if ((signed char)(k/12) < 0) {
					o = '-';
				}
				if (k/12 == 0) {
					o =' ';
				}
				if (ve->type == VOICE_EVENT_NOTE_ON) {
					fprintf(stderr,"%i\n",ve->data1);
					if (prev != time) {
						printf("%c",notes[base]);
						if (o!=' ')
							print_octaves(k/12);
						print_commans(time - prev_time);
						printf("\t#%i %i %i\n",ve->channel,time-prev_time,time);
						/*prev_time = event->delta_time + time;*/
						prev_time = time;
					}
					prev = time;
				}

				//encode_event (&mt, event);
			}
			time += event->delta_time;
			event = event->next;
		}
		//write_track_chunk (s1, &mt);
		printf ("Track %i written\n",++j);
		time = 0;
	}
	assert (mf.tracks==j);
	stream = fopen("raw.mid","w");
	fwrite(s1->buffer, 1, s1->size, stream);
	fclose(stream);
}
/*stupid hack to get the size of a file
  or verify that it exists*/
int get_file_size(char *fname)
{
	int r;
	FILE *s;
	if (!fname)
		return -1;
	s=fopen(fname,"rb");
	if (!s)return -1;
 	fseek(s,0,SEEK_END);
	r=ftell(s);
 	fclose(s);
 	return r;
}
char *read_file(char *filename)
{
	long i;
	FILE *stream;
	char *buffer;
	i=get_file_size(filename);
 	printf("file_size:%s:%i\n",filename,i);
	if (i<=0)
		return (char *)NULL;
	stream=fopen(filename,"rb");
	if (!stream)
			return (char *)NULL;
	buffer=(char *)xmalloc(i+1);
	fread(buffer,1,i,stream);
	fclose(stream);
	buffer[i]=0;
	return buffer;
}
int main(int argc, char ** argv)
{
	char * buffer;
	MIDI_FILE mf;
	MIDI_CHUNK mc;
	MIDI_TRACK mt;
	MIDI_EVENT * event;
	MIDI_EVENT ** events;
	int i;
	if (!argv[1])
		return 0;
	buffer = read_file(argv[1]);
	mf.buffer = mf.data = buffer;
	mf.pos =0;
	mf.size = get_file_size (argv[1]);
	if (!parse_header_chunk (&mf))
		return 0;
	events = xmalloc(sizeof(MIDI_EVENT *)*mf.tracks+1);
	if (!events) return 0;
	i=13;
	for (i=0;i<mf.tracks;i++) {
		mc.data = NULL;
		if (!read_chunk (&mf, &mc)) {
			printf ("Unable to read track chunk\n");
			return 0;
		}
		mt.stream = NULL;
		printf ("Parsing track %i\n",i+1);
		event = parse_track_chunk (&mc, &mt);
		events[i] = event;
		printf ("Track %i sucessfully decoded\n",i+1);
	}
	events[mf.tracks]=NULL;
	doit(&mf,events);
	print_midi_file(&mf);
	printf(mf.buffer);
	return 1;
}
#endif /* DO_MAIN */
