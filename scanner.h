#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <stdio.h>
#include "stream.h"
enum token_t
{
	NONE, COMMENT, STRING, NOTE, IDENTIFIER, NUMBER, COMMA, COLON,
	BRACEOPEN, BRACECLOSE, ERROR, LYRIC, FLOAT, EQUAL, STAR, PIPE, 
	/* special directives */
	INSTRUMENT, TEMPO, VOLUME, BASE, PAN
};
enum scanner_state_t {
	STATE_DIRECTIVE,
	STATE_NOTATION
};
struct scanner_t
{
	enum token_t tokenid;
	enum scanner_state_t state;
	int linecount;
	int colcount;
	char ahead;
	int count;
	STREAM * token;
	STREAM * text;
	char * filename;
};

typedef enum token_t TOKEN_TYPE;
typedef enum scanner_state_t SCANER_STATE;
typedef struct scanner_t SCANNER;

void nexttoken (SCANNER *scanner);
void scanner_init (SCANNER * scanner, const char * text);
void match (SCANNER * scanner, TOKEN_TYPE token);
void match_stay (SCANNER * scanner, TOKEN_TYPE token);
#endif /* _SCANNER_H_ */
