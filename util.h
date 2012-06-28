#ifndef UTIL_H_

#define UTIL_H_
#define INSTRUMENT_COUNT 0x80
void bail(const char * text,...);
void * xmalloc (size_t size);
void * xrealloc (void * ptr, size_t size);
void xfree (void * ptr);
char * xstrdup (char * str);
extern char * instruments[INSTRUMENT_COUNT];
unsigned char instrument_number (char * instrument);

#endif /* UTIL_H_ */
