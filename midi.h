/* Library to read and write standard arbitrary midi files
 * HShivanan (June 2003)
 */

#ifndef _MIDI_H_
#define _MIDI_H_

#include "stream.h"

#define HEADER_CHUNK  1
#define TRACK_CHUNK   2
#define UNKNOWN_CHUNK 3

#define DIVISION_TQN  1
#define DIVISION_TPF  2
#define DIVISION_NONE 3


/* The code relies on the specific numeric values given here */
#define EVENT_TYPE_VOICE   1
#define EVENT_TYPE_MODE    2
#define EVENT_TYPE_SYSEX   3
#define EVENT_TYPE_META    4
#define EVENT_TYPE_UNKNOWN 0

#define VOICE_EVENT_NOTE_OFF			0
#define VOICE_EVENT_NOTE_ON				1
#define VOICE_EVENT_POLYPHONIC_PRESSURE	2
#define VOICE_EVENT_CONTROLLER   		3
#define VOICE_EVENT_PROGRAM				4
#define VOICE_EVENT_CHANNEL_PRESSURE	5
#define VOICE_EVENT_PITCH_BEND			6

#define VOICE_EVENT_COUNT               7

#define VOICE_EVENT_UNKNOWN             7

/* Controllers -- Not all have been defined */
#define CONTROLLER_BANK_SELECT_MSB       0x00
#define CONTROLLER_BANK_SELECT_LSB       0x20
#define CONTROLLER_MODULATION_WHEEL      0x01
#define CONTROLLER_BREATH                0x02
#define CONTROLLER_FOOT                  0x04
#define CONTROLLER_PORTAMENTO_TIME       0x05
#define CONTROLLER_DATA_ENTRY_MSB        0x06
#define CONTROLLER_CHANNEL_VOLUME        0x07
#define CONTROLLER_BALANCE               0x08
#define CONTROLLER_PAN                   0x0A
#define CONTROLLER_EXPRESSION            0x0B
#define CONTROLLER_EFFECT1               0x0C
#define CONTROLLER_EFFECT2               0x0D
/* 0x10-0x13 General Purpose */
/* 0x20-0x3F LSB for controllers 0x00-0x1F */
#define CONTROLLER_DAMPER_PEDAL          0x40
#define CONTROLLER_PORTAMENTO_SWITCH     0x41
#define CONTROLLER_SOSTENUTO             0x42
#define CONTROLLER_SOFT_PEDAL            0x43
#define CONTROLLER_LEGATO                0x44
#define CONTROLLER_HOLD2                 0x45
#define CONTROLLER_PORTAMENTO            0x54
#define CONTROLLER_DATA_INCREMENT        0x60
#define CONTROLLER_DATA_DECREMENT        0x61
#define CONTROLLER_NRPN_LSB              0x62
#define CONTROLLER_NRPN_MSB              0x63
#define CONTROLLER_RPN_LSB               0x64
#define CONTROLLER_RPN_MSG               0x65

extern const unsigned char  MIDI_VOICE_EVENTS[][];

/* channel mode events */
#define MODE_EVENT_SOUND_OFF           0
#define MODE_EVENT_RESET_CONTROLLERS   1
#define MODE_EVENT_LOCAL_CONTROL       2
#define MODE_EVENT_NOTES_OFF           3
#define MODE_EVENT_OMNI_OFF            4
#define MODE_EVENT_OMNI_ON             5
#define MODE_EVENT_MONO_ON             6
#define MODE_EVENT_POLY_ON             7

#define MODE_EVENT_COUNT 8
#define MODE_EVENT_UNKNOWN MODE_EVENT_COUNT

extern const unsigned char  MIDI_MODE_EVENTS[][];

/* meta text events -- the values assigned are specifically chosen */

#define META_EVENT_SEQUENCE			0x0
#define META_EVENT_TEXT				0x1
#define META_EVENT_COPYRIGHT		0x2
#define META_EVENT_TRACK_NAME		0x3
#define META_EVENT_INSTRUMENT_NAME	0x4
#define META_EVENT_LYRIC			0x5
#define META_EVENT_MARKER			0x6
#define META_EVENT_CUE				0x7
#define META_EVENT_CHANNEL_PREFIX   0x8
#define META_EVENT_EOT              0x9
#define META_EVENT_SET_TEMPO        0xA
#define META_EVENT_SMTPE_OFFSET     0xB
#define META_EVENT_TIME_SIGNATURE   0xC
#define META_EVENT_KEY_SIGNATURE    0xD
#define META_EVENT_SPECIFIC         0xE
#define META_EVENT_UNKNOWN          0xF

#define META_EVENT_COUNT 15

extern const unsigned char MIDI_META_EVENTS[][];

/* MIDI error messages */
#define MIDI_ERROR_UNKNOWN_META_EVENT  0x1 /* an unknown meta event was encountered */
#define MIDI_ERROR_CHUNK_EMPTY         0x2 /* the chunk's data field was NULL */
#define MIDI_ERROR_CHUNK_TYPE          0x3 /* chunk was not MTrk or MThd */
#define MIDI_ERROR_VOICE_1             0x4 /* Voice Data 1 value is out-of-bounds */
#define MIDI_ERROR_VOICE_2			   0x5 /* Voice Data 2 value is out-of-bounds */
#define MIDI_ERROR_UNKNOWN_VOICE	   0x6 /* An unknown voice event was encountered while decoding */
#define MIDI_ERROR_UNKNOWN_MODE        0x7 /* An unknown mode event was encountered while decoding */
#define MIDI_ERROR_UNKNOWN_EVENT       0x8 /* An unknown midi event -- Usually a decoding loop error */
#define MIDI_ERROR_NO_EOT              0x9 /* The EOT marker was not found */
#define MIDI_ERROR_NOT_TRACK_CHUNK     0xa /* A track chunk was expected but not found */
#define MIDI_ERROR_NOT_HEADER_CHUNK    0xb /* A header chunk was expected but not found */
#define MIDI_ERROR_HEADER              0xc /* Error reading the header chunk */
#define MIDI_ERROR_BAD_FORMAT		   0xd /* Unrecognized MIDI format */

extern const char * MIDI_ERROR_STR[0xe];
extern void (*midi_error_fun)(int,...);

#define validate_text_event_type(x) ( (x)>=0x01 && (x)<=0x07 )

struct midivoiceevent_t
{
	unsigned char type;
	unsigned char channel;
	unsigned char data1;
	unsigned char data2;
};

struct midimetaevent_t
{
	unsigned char type;
	unsigned char unknown_type;
	unsigned long length;
	unsigned char * data;
};

struct midisysexevent_t
{
	unsigned char type;
	unsigned long length;
	unsigned char * data;
};

struct midimodeevent_t
{
	unsigned char type;
	unsigned char channel;
	unsigned char data;
};

typedef struct midivoiceevent_t VOICE_EVENT;
typedef struct midimetaevent_t  META_EVENT;
typedef struct midisysexevent_t SYSEX_EVENT;
typedef struct midimodeevent_t MODE_EVENT;


struct midievent_t
{
	int type;
	unsigned long delta_time;
	union {
		META_EVENT  * meta_event;
		SYSEX_EVENT * sysex_event;
		VOICE_EVENT * voice_event;
		MODE_EVENT  * mode_event;
	} event;
	struct midievent_t * next;
};

struct midichunk_t
{
	int  type;
	size_t length;
	char * data;
};

struct midifile_t
{
	int pos;
	size_t size;
	char * buffer;
	char * data; /* pointer to beginning of original data */
	unsigned int format;
	size_t tracks;
	unsigned int tpqn; /* ticks per quarter note */
	unsigned int tpf;  /* ticks per SMTPE frame */
	unsigned int fps;  /* SMTPE frames per second */
	unsigned int division; /* Either DIVISION_TQN or DIVISION_TPF */
};

struct miditrack_t
{
	STREAM * stream; /* arbitrary data written using one of the encode_event functions */
	struct miditrack_t * next; /* TODO: this is not actually used */
};

typedef struct midichunk_t   MIDI_CHUNK;
typedef struct midifile_t    MIDI_FILE;
typedef struct miditrack_t   MIDI_TRACK;
typedef struct midievent_t	 MIDI_EVENT;


/* Macros to read and write ints in BIG-ENDIAN format
 * These should be portable */
#define read32(x) (unsigned char)((x)[0])<<24 | (unsigned char)((x)[1])<<16 | \
	              (unsigned char)((x)[2])<<8 | (unsigned char)((x)[3])

#define read16(x) ((((unsigned char)((x)[0]))<< 8)  |  ((unsigned char)((x)[1])))


#define print4(buffer) printf ("%c%c%c%c\n",(buffer)[0],(buffer)[1],(buffer)[2],(buffer)[3])

#define chunk_size(chunk) ((chunk)->length+8) /*4 bytes for the magic number and 4 for the length */

/* function prototypes */
int  validate_chunk             (MIDI_CHUNK * mc);
void free_event_list            (MIDI_EVENT * event);

void encode_event_voice         (MIDI_TRACK * mt, VOICE_EVENT * event);
void encode_event_mode          (MIDI_TRACK * mt, MODE_EVENT  * event);
void encode_event_sysex         (MIDI_TRACK * mt, SYSEX_EVENT * event);
void encode_event_meta          (MIDI_TRACK * mt, META_EVENT  * event);
int  encode_event               (MIDI_TRACK * mt, MIDI_EVENT * event);
int  encode_voice               (MIDI_TRACK * mt, unsigned long delta_time,
                                 unsigned char channel, unsigned char type,
                                 unsigned char data1, unsigned char data2);

int  encode_meta                (MIDI_TRACK * mt, unsigned long delta_time, unsigned char type,
                                 unsigned char unknown_type, unsigned long length,
                 				 unsigned char * data);

int  encode_sysex               (MIDI_TRACK * mt, unsigned long delta_time, unsigned char type,
                                 unsigned long length, unsigned char * data);

int  encode_mode                (MIDI_TRACK * mt, unsigned long delta_time, unsigned char type,
                                 unsigned char channel, unsigned char data);

int decode_event_voice          (MIDI_TRACK * mt, MIDI_EVENT * event, unsigned char status);
int decode_event_mode           (MIDI_TRACK * mt, MIDI_EVENT * event, unsigned char status);
int decode_event_sysex          (MIDI_TRACK * mt, MIDI_EVENT * event);
int decode_event_meta           (MIDI_TRACK * mt, MIDI_EVENT * event);

MIDI_EVENT * parse_track_event  (MIDI_TRACK * mt);
MIDI_EVENT * parse_track_events (MIDI_TRACK * mt);
MIDI_EVENT * parse_track_chunk  (MIDI_CHUNK * chunk, MIDI_TRACK * mt);
int parse_header_chunk          (MIDI_FILE * mf);

int make_header_chunk           (MIDI_FILE * mf, MIDI_CHUNK * chunk);
int make_track_chunk            (MIDI_TRACK * mt, MIDI_CHUNK * chunk);
int write_chunk                 (char * buffer, MIDI_CHUNK * chunk);
int read_chunk                  (MIDI_FILE * mf, MIDI_CHUNK * chunk);

/* these functions require the use of a STREAM object */
int write_header_chunk          (STREAM * stream, MIDI_FILE * mf);
int write_track_chunk           (STREAM * stream, MIDI_TRACK * mt);
#endif /* _MIDI_H_ */
