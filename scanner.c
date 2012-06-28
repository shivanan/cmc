/* 
 * a simple lexer\tokenizer for music notation
 * 
 */

#include <string.h>
#include "scanner.h"
#include "stream.h"

int isalpha(char d)
{
	if (d>='a') if (d<='z') return 1;
    if (d>='A') if (d<='Z') return 1;
    if (d=='_') return 1;
    return 0;
}

int isnum(char d)
{
    return ((d>='0')&&(d<='9'))?1:0;
}

int isalphanum(char d)
{
	return (isalpha(d)||isnum(d));
}

int is_valid_note (char n)
{
	return ( n=='S'||
			n=='r' ||
			n=='R' ||
			n=='g' ||
			n=='G' ||
			n=='m' ||
			n=='M' ||
			n=='P' ||
			n=='D' ||
			n=='d' ||
			n=='n' ||
			n=='N');
}


static void nextchar(SCANNER *scanner)
{
	stream_add_char (scanner->token, scanner->ahead);
	scanner->colcount++;
	if (!stream_read_char (scanner->text, &(scanner->ahead)))
		scanner->tokenid = NONE;
}


/* scan in the next character without copying the current one to the
 * token buffer */
static void nextchar_dont_consume(SCANNER *scanner)
{
	scanner->colcount++;
	if (!stream_read_char (scanner->text, &(scanner->ahead)))
		scanner->tokenid = NONE;
}
static int iswhitespace(SCANNER *scanner, char d)
{
	/*if ((d==13)||(d==10))scanner->linecount++,scanner->colcount=0;*/
	if (d==10) scanner->linecount++, scanner->colcount=0;
	return ( (d==' ')||(d==9)||(d==13)||(d==10));
}

static void eatwhitespace(SCANNER *scanner)
{
	stream_read_char (scanner->text, &(scanner->ahead));
	while (iswhitespace (scanner,scanner->ahead))
		stream_read_char (scanner->text, &(scanner->ahead));
}

static void scancomment(SCANNER *scanner)
{
	nextchar (scanner);
	while(1)
	{
		nextchar (scanner);
		if (scanner->ahead==13||scanner->ahead==10||scanner->ahead==0) break;
	}
	scanner->tokenid = COMMENT;
	nextchar (scanner);

}

static void scanstring(SCANNER *scanner)
{
	scanner->tokenid = STRING;
	nextchar_dont_consume(scanner);
	while(1)
	{
		if (scanner->ahead=='"')
		{
			nextchar_dont_consume(scanner);
			break;
		} else
		if (scanner->ahead == '\\')
		{
			stream_read_char (scanner->text, &(scanner->ahead));
			switch (scanner->ahead)
			{
				case 'n': scanner->ahead = '\n';/*nextchar (scanner)*/;break;
				case 'r': scanner->ahead = '\r';/*nextchar (scanner)*/;break;
				case 'b': scanner->ahead = '\b';/*nextchar (scanner)*/;break;
				case 't': scanner->ahead = '\t';/*nextchar (scanner)*/;break;
				case '\\':scanner->ahead = '\\';/*nextchar (scanner)*/;break;
				case '"': scanner->ahead = '"' ;/*nextchar (scanner)*/;break;
				case '\0':break;
				default:
					fprintf (stderr,"Unrecognized control character:\\%c\n",scanner->ahead);
			}
			
		}
		nextchar (scanner);
		if (scanner->ahead==0) break;
	}
	stream_add_char (scanner->token, '\0');
}



static void scannumber(SCANNER *scanner)
{
	scanner->tokenid=NUMBER;
	nextchar(scanner);

	while (isnum(scanner->ahead))nextchar(scanner);
	
	if (scanner->ahead=='.')
		if (isnum(scanner->text->buffer[0]))
		{
			scanner->tokenid=FLOAT;
			nextchar (scanner);
			while (isnum (scanner->ahead))
				nextchar(scanner);
		}

}
static void scancolon(SCANNER *scanner)
{
	if (scanner->state == STATE_DIRECTIVE) {
		scanner->tokenid=COLON;
		nextchar (scanner);
		return;
	}
	scanner->tokenid = LYRIC;
	nextchar_dont_consume (scanner);
	/*while (scanner->ahead != ':' && scanner->ahead != NONE) 
		nextchar(scanner);
	nextchar_dont_consume (scanner);
	stream_add_char (scanner->token , '\0');*/
	while(1)
	{
		if (scanner->ahead==':')
		{
			nextchar_dont_consume(scanner);
			break;
		} else
		if (scanner->ahead == '\\')
		{
			stream_read_char (scanner->text, &(scanner->ahead));
			switch (scanner->ahead)
			{
				case 'n': scanner->ahead = '\n';/*nextchar (scanner)*/;break;
				case 'r': scanner->ahead = '\r';/*nextchar (scanner)*/;break;
				case 'b': scanner->ahead = '\b';/*nextchar (scanner)*/;break;
				case 't': scanner->ahead = '\t';/*nextchar (scanner)*/;break;
				case '\\':scanner->ahead = '\\';/*nextchar (scanner)*/;break;
				case '"': scanner->ahead = '"' ;/*nextchar (scanner)*/;break;
				case ':': scanner->ahead = ':' ;                       break;
				case '\0':break;
				default:
					fprintf (stderr,"Unrecognized control character:\\%c\n",scanner->ahead);
			}
			
		}
		nextchar (scanner);
		if (scanner->ahead==0) break;
	}
	stream_add_char (scanner->token, '\0');
}
static void scanequal(SCANNER *scanner)
{
	if (scanner->state == STATE_NOTATION)
		scanner->tokenid = COMMA;
	else
		scanner->tokenid=EQUAL;
	nextchar(scanner);
}
static void scanbraceopen(SCANNER *scanner)
{
	scanner->tokenid=BRACEOPEN;
	nextchar(scanner);
	scanner->state = STATE_DIRECTIVE;
}
static void scanbraceclose(SCANNER *scanner)
{
	scanner->tokenid=BRACECLOSE;
	nextchar(scanner);
	scanner->state = STATE_NOTATION;
}
static void scan_note (SCANNER * scanner)
{
	scanner->tokenid = NOTE;
	nextchar (scanner);
	if (scanner->ahead == '+') {
		while (scanner->ahead =='+')
			nextchar(scanner);
	} else if (scanner->ahead == '-') {
		while (scanner->ahead == '-')
			nextchar (scanner);
	}
	stream_add_char (scanner->token, '\0');
}
#define register_(x,y) if (!strcasecmp(scanner->token->buffer,x))scanner->tokenid=y
static void scanident(SCANNER *scanner)
{
	scanner->tokenid = IDENTIFIER;
	nextchar(scanner);
	
	while (isalphanum (scanner->ahead)||(scanner->ahead == '-'))  nextchar(scanner);
	stream_add_char (scanner->token ,'\0');

	/*TODO: Change this ridiculous thing.
	 *      Make it case insensitive
	 */
	register_ ("instrument", INSTRUMENT);
	register_ ("tempo", TEMPO);
	register_ ("base", BASE);
	register_ ("volume", VOLUME);
	register_ ("pan", PAN);
    
}
static void scancomma(SCANNER *scanner)
{
	scanner->tokenid=COMMA;
	nextchar (scanner);
}
static void scanstar (SCANNER * scanner)
{
	scanner->tokenid = STAR;
	nextchar (scanner);
}
static void scanpipe (SCANNER * scanner)
{
	scanner->tokenid = PIPE;
	nextchar (scanner);
}
static void scannull(SCANNER *scanner)
{
	scanner->tokenid=NONE;
}
void scandef(SCANNER *scanner)
{
	scanner->tokenid=ERROR;
	nextchar (scanner);
}

void _nexttoken(SCANNER *scanner)
{
	stream_write_reset (scanner->token);
	scanner->count = 0;
	/*scanner->ahead = *(stream_current_position (scanner->text)); */

	
	if (iswhitespace (scanner, scanner->ahead)) eatwhitespace (scanner);
	if (isnum (scanner->ahead)) scannumber (scanner); else
	if (scanner->state == STATE_NOTATION && is_valid_note (scanner->ahead)) scan_note (scanner); else
	if (scanner->state == STATE_DIRECTIVE && isalphanum (scanner->ahead)) scanident (scanner); else
	switch (scanner->ahead)
	{
		case ':': scancolon(scanner);		break;
		case '=': scanequal(scanner);      	break;
		case '{': scanbraceopen(scanner);  	break;
		case '}': scanbraceclose(scanner); 	break;
		case '#': scancomment(scanner);		break;
 		case 0  : scannull(scanner);		break;
 		case '"': scanstring(scanner);		break;
		case ',': scancomma(scanner);		break;
		case '*': scanstar (scanner);       break;
		case '|': scanpipe (scanner);       break;
		default:  scandef(scanner);
	}
	stream_add_char (scanner->token, '\0');
}

void nexttoken(SCANNER *scanner)
{
	_nexttoken(scanner);
	if (scanner->tokenid==COMMENT)
		while ((scanner->tokenid==COMMENT)&&(scanner->tokenid!=NONE))
			_nexttoken(scanner);
}
void scanner_init (SCANNER * scanner, const char * text)
{
	scanner->token = stream_create (1);
	scanner->state = STATE_NOTATION;
	scanner->text = stream_create_from_buffer ( text, strlen(text));
	scanner->linecount = 1;
	scanner->colcount = 0;
	stream_add_char (scanner->text, '\0');
	stream_read_char (scanner->text, &(scanner->ahead));
}

/* print an error message and ext */
static void print_error (SCANNER * scanner)
{
	fprintf (stderr, "%s: %i Unexpected token:%s",PROG_NAME,
		                          scanner->linecount,scanner->token->buffer);
	exit(0);
}
void match (SCANNER * scanner, TOKEN_TYPE token)
{
	if (scanner->tokenid != token)
		print_error (scanner);
	nexttoken (scanner);
}

/* match the current token against a type without consuming the next
 * token from the input stream */
void match_stay (SCANNER * scanner, TOKEN_TYPE token)
{
	if (scanner->tokenid != token)
		print_error (scanner);
}
