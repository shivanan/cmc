#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "util.h"

void * xmalloc (size_t size)
{
	void * result;
	if (!size)
		bail ("Zero byte allocation requested\n");
	result = malloc(size);
	if (!result)
		bail ("Memory allocation failed\n");
	return result;
}

void * xrealloc (void * ptr, size_t size)
{
	void * result;
	if (!size)
		bail ("Zero byte realloction requested\n");
	result = realloc (ptr, size);
	if (!result)
		bail ("Reallocation failed\n");
	return result;
}

void xfree (void * ptr)
{
	if (!ptr)
		bail ("Attempting to free NULL pointer");
	free (ptr);
}

char * xstrdup (char * str)
{
	char * result;
	size_t len = strlen(str);
	result = xmalloc (len+1);
	memcpy (result, str, len);
	result[len]='\0';
	return result;
}


void bail(const char * text,...)
{
	va_list args;
	va_start (args, text);
	vfprintf (stderr, text, args);
	va_end (args);
	exit(1);
}



char * instruments[INSTRUMENT_COUNT] = {
/* 0 */  "Piano1",    /* 1 */  "Piano2",   /* 2 */  "Piano3",   /* 3 */  "Hnkey",     /* 4 */  "ElPiano1",
/* 5 */  "ElPiano2",  /* 6 */  "Harpsi",   /* 7 */  "Clavi",    /* 8 */  "Celesta",   /* 9 */  "Glock",
/* 10 */ "Musicbox",  /* 11 */ "Vibe",     /* 12 */ "Marimba",  /* 13 */ "Xilophone", /* 14 */ "TubeBell",
/* 15 */ "Santur",	  /* 16 */ "ElOrgan1", /* 17 */ "ElOrgan2", /* 18 */ "ElOrgan3",  /* 19 */ "Chrcorgan",
/* 20 */ "ReedOrgan", /* 21 */ "Accordn",  /* 22 */ "Harmonic", /* 23 */ "Bandneon",  /* 24 */ "AcouGtr1",
/* 25 */ "AcouGtr2",  /* 26 */ "JazzGtr",  /* 27 */ "CleanGtr", /* 28 */ "MuteGtr",   /* 29 */ "DriveGtr",
/* 30 */ "LeadGtr",   /* 31 */ "HarmoGtr", /* 32 */ "AcouBass", /* 33 */ "FingBass",  /* 34 */ "PickBass",
/* 35 */ "Fretless",  /* 36 */ "SlapBass1",/* 37 */ "SlapBass2",/* 38 */ "SynBass1",  /* 39 */ "SynBass2",
/* 40 */ "Violin",    /* 41 */ "Viola",    /* 42 */ "Cello",    /* 43 */ "ContraBass",/* 44 */ "TempStr",
/* 45 */ "PizzStr",   /* 46 */ "Harp",     /* 47 */ "Timpani",  /* 48 */ "Strings",   /* 49 */ "SlowStr",
/* 50 */ "SynStr1",   /* 51 */ "SynStr2",  /* 52 */ "ChoirAhs", /* 53 */ "VoiceOhs",  /* 54 */ "SynVox",
/* 55 */ "OrcheHit",  /* 56 */ "Trumpet",  /* 57 */ "Trombone", /* 58 */ "Tuba",      /* 59 */ "MuteTrmp",
/* 60 */ "FrHorn",    /* 61 */ "BrsSect",  /* 62 */ "SynBrs1",  /* 63 */ "SynBrs2",   /* 64 */ "SoprSax",
/* 65 */ "AltoSax",   /* 66 */ "TenrSax",  /* 67 */ "BariSax",  /* 68 */ "Oboe",      /* 69 */ "EnglHorn",
/* 70 */ "Bassson",   /* 71 */ "Clarinet", /* 72 */ "Piccolo",  /* 73 */ "Flute",     /* 74 */ "Recorder",
/* 75 */ "Panflute",  /* 76 */ "BottBlow", /* 77 */ "Shaku",    /* 78 */ "Whistle",   /* 79 */ "Occarina",
/* 80 */ "Square",    /* 81 */ "Saw",      /* 82 */ "Callilope",/* 83 */ "Chiffer",   /* 84 */ "Charang",
/* 85 */ "Solo Vox",  /* 86 */ "5th Saw",  /* 87 */ "BassLead", /* 88 */ "Fantasia",  /* 89 */ "Warm Pad",
/* 90 */ "poly Synth",/* 91 */ "SpaceFox", /* 92 */ "BowGlass", /* 93 */ "MetalPad",  /* 94 */ "HaloPad",
/* 95 */ "SweepPad",  /* 96 */ "Ice Rain", /* 97 */ "SndTrack", /* 98 */ "Crystal",   /* 99 */ "Athmospaere",
/* 100*/ "Brightns",  /* 101*/ "Goblin",   /* 102*/ "EchoDrop", /* 103*/ "StarThem",  /* 104*/ "Sitar",
/* 105*/ "Banjo",     /* 106*/ "Shamisen", /* 107*/ "Koto",     /* 108*/ "Kalimba",   /* 109*/ "BagPipe",
/* 110*/ "Kokyu",     /* 111*/ "Shanai",   /* 112*/ "TnklBell", /* 113*/ "Agogo",     /* 114*/ "SeelDrm",
/* 115*/ "WoodBlk",   /* 116*/ "Taika",    /* 117*/ "MelodTom", /* 118*/ "SynDrum",   /* 119*/ "RevCymbal",
/* 120*/ "GuitNoise", /* 121*/ "KeyClick", /* 122*/ "SeaShore", /* 123*/ "Birds",     /* 124*/ "Telephone",
/* 125*/ "Helicopter",/* 126*/ "Aplase",   /* 127*/ "GunShot"};

unsigned char instrument_number (char * instrument)
{
	int i=0;
	while (i<INSTRUMENT_COUNT) {
		if (!strcasecmp (instrument, instruments[i]))
			return (unsigned char)i;
		i++;
	}
	return INSTRUMENT_COUNT;
}

