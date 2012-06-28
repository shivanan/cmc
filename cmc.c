 /*
 * Started July 2003 - HS
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream.h"
#include "midi.h"
#include "util.h"
#include "scanner.h"
#include <assert.h>

#ifdef HAVE_ISATTY
#	include <unistd.h>
#endif


#define MAX_TRACK_COUNT 4
#define EXTRA_CHANNEL 5 

#define DEF_INSTRUMENT 0x0
#define DEFAULT_SPEED 30


static unsigned int file_count = 0;
static char * in_files[MAX_TRACK_COUNT];

/* global parameters set by the command-line */
static char * output_file = NULL;
static unsigned long divisions = 96;
static char * instrument = NULL;
static int portamento = 1;
static int include_thalam = 0;
static int speed = DEFAULT_SPEED;
void simple_usage()
{
	fprintf (stderr, "%s: usage %s [notation_files] [-o midi_file]\n",PROG_NAME,PROG_NAME);
}

void print_usage() 
{
	simple_usage();
	fprintf (stderr, "Basic Options:\n");
	fprintf (stderr, "  -o, --output <file>              Specify the output filename (dump to stdout by default)\n");
	fprintf (stderr, "  --dump-instruments               Dump an instrument list to stdout and exit\n");
	fprintf (stderr, "  -V, --version                    Show program version info and exit\n");
	fprintf (stderr, "  -h, --help                       Show this screen and exit\n");
	fprintf (stderr, "Output Options:\n");
	fprintf (stderr, "  -d, --divisions                  Ticks per Quarter Note\n");
	/* TODO: Speed */
	fprintf (stderr, "  -s, --speed <value>              Default tempo\n");
	fprintf (stderr, "  -i, --instrument <instrument>    Default instruments to use\n");
	fprintf (stderr, "  -p, --portamento                 Generate portamento events when required\n");
	fprintf (stderr, "  --no-portamento                  Don't generate portamento events ever\n");
/* Thalam support is extremely crappy so it's been temporarily removed */
#ifdef HAVE_THALAM
	fprintf (stderr, "Thalam Options:\n");
	fprintf (stderr, "  -t, --thalam                     Include a thalam track\n");
	fprintf (stderr, "  --thalam-channel <number>        Channel number to be used for thalam\n");
	fprintf (stderr, "  --thalam-instrument <instrument> Instrument to be used for thalams\n");
#endif
}

void dump_instruments ()
{
	int i = 0;
	while (i < INSTRUMENT_COUNT) {
		printf ("%3d %s\n", i, instruments[i]);
		i++;
	}
}

#define FLAG(txt,var,val) if (!strcmp(txt,*argv)) {\
								var = val;\
								argv++;\
								continue;\
							}

#define VARSTR(txt,var) if (!strcmp(txt,*argv)) { \
							argv++; \
							if (!*argv) \
								bail("The %s option requires an argument\n",txt); \
							var = xstrdup(*argv); \
							argv++; \
							continue; \
	                    }
#define VARINT(txt,var) if (!strcmp(txt,*argv)) {\
							argv++; \
							if (!*argv) \
								bail("The %s option requires an argument\n",txt); \
							var = strtoul(*argv,NULL,10); \
							argv++; \
							continue; \
	                    }
int parse_args (int argc, char ** argv)
{
	argv++;
	while ((*argv)!=NULL) {
		if (**argv == '-') {
			VARSTR("-o",output_file);
			VARSTR("--output",output_file);

			VARINT("-d",divisions);
			VARINT("--divisions",divisions);

			VARINT("-s",speed);
			VARINT("--speed",speed);

			VARSTR("-i",instrument);
			VARSTR("--instrument",instrument);

			FLAG("-p",portamento,1);
			FLAG("--portamento",portamento,1);

			FLAG("--no-portamento",portamento,0);

#ifdef HAVE_THALAM
		    FLAG("-t",include_thalam,1);
			FLAG("--thalam",include_thalam,1);
#endif

			if ((!strcmp("-h",*argv))||(!strcmp("--help",*argv))) {
				print_usage();
				return 0;
			}
			if (!strcmp("--dump-instruments",*argv)) {
				dump_instruments();
				return 0;
			}

			bail ("Unrecognized option:%s\n",*argv);
		}else {
			if (file_count >= MAX_TRACK_COUNT) 
				bail("Too many tracks. You can only specify %i\n",MAX_TRACK_COUNT);
			in_files[file_count++] = *argv;
			argv++;
		}
	}
	return 1;
}
void encode_lyric (MIDI_TRACK * mt, char * lyric, size_t len)
{
	MIDI_EVENT me;
	me.delta_time = 0;
	me.type = EVENT_TYPE_META;
	me.event.meta_event = malloc(sizeof(META_EVENT));
	me.event.meta_event->type = META_EVENT_LYRIC;
	me.event.meta_event->length = len;
	me.event.meta_event->data = lyric;
	encode_event (mt, &me);
}

unsigned char note_map2 (char * n)
{
	static char notes[12] = {'S','r','R','g','G','m','M','P','d','D','n','N'};
	int i;
	for (i=0;i<12;i++)
		if (*n == notes[i]) {
			unsigned char base = (unsigned char)i;
			int octave = 0;
			if (*(n+1)=='+' || *(n+1)=='-'){
				if (*(n+1)=='+') octave++;
				else octave--;
			}
			return (unsigned char)((base+0x3C) + (unsigned char)octave*(unsigned char)12);
		}
	return 0xFF;
}
/* the token after the '{' should be ready when
 * this function is called */
void parse_directive (SCANNER * scanner, MIDI_TRACK * mt, unsigned char channel)
{
	while (scanner->tokenid != BRACECLOSE) {
		switch (scanner->tokenid) {
			case INSTRUMENT: {
								 unsigned char instr;
								 nexttoken (scanner);
								 match (scanner, EQUAL);
								 match_stay (scanner, STRING);
								 instr = instrument_number (scanner->token->buffer);
								 if (instr>=INSTRUMENT_COUNT){
									 fprintf (stderr, "Unknown Instrument:%s\n",
											 scanner->token->buffer);
									 fprintf (stderr, "Using default instrument:%#x\n",DEF_INSTRUMENT);
									 instr = DEF_INSTRUMENT;
								 }
								 encode_voice (mt, 0, channel, VOICE_EVENT_PROGRAM, instr, 0);
								 break;
								 
							 }
			case VOLUME: {
							 long volume;
							 nexttoken (scanner);
							 match (scanner, EQUAL);
							 match_stay (scanner, NUMBER);
							 volume = strtol (scanner->token->buffer, NULL, 10);
							 if (!(volume>=0 && volume <= 0x7F)) {
								 fprintf (stderr, "%s: Invalid volume (should be between 0 and 127):%li\n",
										 PROG_NAME,volume);
								 exit (0);
							 }
							 encode_voice (mt, 0, channel, VOICE_EVENT_CONTROLLER, CONTROLLER_CHANNEL_VOLUME, (unsigned char)volume);
							 break;
						 }
			case PAN:    {
							 long pan;
							 nexttoken (scanner);
							 match (scanner, EQUAL);
							 match_stay (scanner, NUMBER);
							 pan = strtol (scanner->token->buffer, NULL, 10);
							 if (!(pan>=0 && pan<=0x7F)) { /* TODO: What is the limit for "pan" events?*/
								 fprintf (stderr, "%s: Invalid pan value (should be between 0 and 127):%li\n",
										 PROG_NAME,pan);
								 exit (0);
							 }
							 encode_voice (mt, 0, channel, VOICE_EVENT_CONTROLLER, CONTROLLER_PAN, (unsigned char)pan);
							 break;
						 }
			default:
				fprintf (stderr, "Error in directive:%i\n",scanner->linecount);
				exit(0);
		}
		nexttoken (scanner);
	}
	nexttoken(scanner);
}
#define BEATS 8
void encode_track (MIDI_TRACK * mt, char * notes, MIDI_TRACK * extra, unsigned char channel)
{
	unsigned long delta_time = 0;
	SCANNER scanner;
	int beat = 0;
	unsigned char instr = 0;
	int DT = speed;
	
	if (instrument) {
		instr = instrument_number(instrument);
		fprintf(stderr,"Instrument:%s,%#x\n",instrument,instr);
	}
	scanner_init (&scanner, notes);
	encode_voice (mt, 0, channel, VOICE_EVENT_PROGRAM, instr, 0);
	if (extra) {
	 	encode_voice (extra, 0, EXTRA_CHANNEL, VOICE_EVENT_PROGRAM, 104, 0);
 		encode_voice (extra, 0, EXTRA_CHANNEL, VOICE_EVENT_CONTROLLER, CONTROLLER_CHANNEL_VOLUME, (unsigned char)45);
	}
	/*encode_event (mt, &event);*/
	nexttoken (&scanner);
	while (scanner.tokenid != NONE) {
		unsigned char c;
		static char prev=0;
		static int note_shift = 0;
		if (scanner.tokenid == LYRIC) {
			encode_lyric (mt, scanner.token->buffer, scanner.token->size);
			nexttoken(&scanner);
			continue;
		}
		if (scanner.tokenid == STAR) {
			note_shift = 1;
			nexttoken (&scanner);
			continue;
		}
		if (scanner.tokenid == BRACEOPEN) {
			nexttoken (&scanner);
			parse_directive (&scanner, mt, channel);
			continue;
		}
		delta_time += DT;
		beat++;
		if ( ((beat-1) % BEATS == 0) && extra) {
			encode_voice (extra, beat==1?0:(BEATS-1)*DT, EXTRA_CHANNEL, VOICE_EVENT_NOTE_ON,note_map2("S"),0x40);
			encode_voice (extra, 0, EXTRA_CHANNEL, VOICE_EVENT_NOTE_ON,  note_map2("P"), 0x40);
			encode_voice (extra, 0, EXTRA_CHANNEL, VOICE_EVENT_NOTE_ON,  note_map2("S+"),0x40);
			encode_voice (extra, DT, EXTRA_CHANNEL,VOICE_EVENT_NOTE_OFF, note_map2("S"), 0x40);
			encode_voice (extra, 0, EXTRA_CHANNEL, VOICE_EVENT_NOTE_OFF, note_map2("P"), 0x40);
			encode_voice (extra, 0, EXTRA_CHANNEL, VOICE_EVENT_NOTE_OFF, note_map2("S+"),0x40);
		}
		if (scanner.tokenid == COMMA) {
			nexttoken (&scanner);
			continue;
		}
		if (scanner.tokenid != NOTE) {
			fprintf (stderr ,"Error:%i Unrecognized token:%s\n",scanner.linecount,scanner.token->buffer);
			exit(0);
		}
		c = note_map2 (scanner.token->buffer);
		nexttoken (&scanner);
		if (c==0xFF) {
			fprintf (stderr, "Invalid Note:%c(%#x)\n",*notes,(unsigned char)*notes);
			exit(0);
		}
		if (note_shift && portamento) {
			encode_voice (mt, delta_time, channel, VOICE_EVENT_CONTROLLER, CONTROLLER_PORTAMENTO_SWITCH, 0x7F);
			delta_time = 0;
			encode_voice (mt, delta_time, channel, VOICE_EVENT_CONTROLLER, 0x25, 0x01);
			encode_voice (mt, delta_time, channel, VOICE_EVENT_CONTROLLER, 0x5, 0x50);
		}else {
			encode_voice (mt, delta_time, channel, VOICE_EVENT_CONTROLLER, CONTROLLER_PORTAMENTO_SWITCH, 0x0);
			delta_time = 0;
		}
		encode_voice (mt, delta_time, channel, VOICE_EVENT_NOTE_ON, c, 0x40);
		if (prev) {
			encode_voice (mt, delta_time, channel, VOICE_EVENT_NOTE_OFF, prev, 0x40);
		}
		prev =c;
		note_shift = 0;
		delta_time = 0;
	}
	encode_voice (mt, delta_time, channel, VOICE_EVENT_CONTROLLER, CONTROLLER_PORTAMENTO_SWITCH, 0x0);
	encode_meta (mt, delta_time, META_EVENT_EOT, 0, 0, NULL);
	if (extra)
		encode_meta (extra, delta_time, META_EVENT_EOT, 0, 0, NULL);
}
void encode_file (char ** track_text, size_t track_count)
{
	MIDI_FILE mf;
	MIDI_TRACK extra, mt;
	int i;
	STREAM * output;
	unsigned char channel = 0;
	mf.tracks = track_count + ((include_thalam==1)?1:0);
	mf.format = 1;
	mf.division = DIVISION_TQN;
	mf.tpqn = divisions;
	output= stream_create (10);
	write_header_chunk (output, &mf);
	for (i=0;i<track_count-1;i++) {
		MIDI_TRACK mt;
		mt.stream = stream_create (6);
		encode_track (&mt,*(track_text++),NULL,channel++);
		write_track_chunk (output, &mt);
		stream_free (mt.stream);
	}
	
	mt.stream    = stream_create (6);
	if (include_thalam) {
		extra.stream = stream_create (6);
		encode_track (&mt, *(track_text),&extra, channel);
		write_track_chunk(output, &mt);
		write_track_chunk(output,&extra);
		stream_free (mt.stream);
		stream_free (extra.stream);
	} else {
		encode_track (&mt, *(track_text), NULL, channel);
		write_track_chunk(output, &mt);
		stream_free (mt.stream);
	}
	
	if (!output_file || !strcmp(output_file,"-"))
		stream_write_to_io (output, stdout);
	else
		stream_write_to_file (output, output_file);
	stream_free (output);
	
}
int main(int argc, char ** argv)
{
	STREAM * note_s;
	char * tracks[MAX_TRACK_COUNT];
	int track_count = 0;
	if (parse_args (argc, argv) == 0)
		return 0;
	if (!file_count) {
#ifdef HAVE_ISATTY
		/* if no text was piped into the program,
		 * just display usage info and exit */
		if (isatty (STDIN_FILENO)) {
			simple_usage();
			return 1;
		}
#endif
		note_s = stream_create (1);
		if (!stream_copy_from_io (note_s,stdin))
			return 0;
		tracks[0] = stream_copy_buffer (note_s);
		track_count = 1;
		stream_free (note_s);
	} else {
		while (file_count--) {
			STREAM * tr = stream_load_from_file (in_files[file_count]);
			if (!tr)
				bail ("Unable to open file:%s\n",in_files[file_count]);
			stream_add_char (tr, '\0');
			tracks[track_count++] = stream_copy_buffer (tr);
			stream_free (tr);
		}
		
/*note_s = stream_load_from_file (argv[1]);
		argv++;*/
	}
	encode_file (tracks, track_count);
	while (track_count-->0)
		free (tracks[track_count]);
/*	encode_notes2 (note_s->buffer,argv[1]?argv[1]:"-");*/
	return 1;
}
