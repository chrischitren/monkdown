#ifndef PARSE_H
#define PARSE_H


/* Maximum number of "blocks" in a single document */
#define MAX_DOCSZ 512
/* Maximum size of a single block */
#define READBUF_SZ 16384

/* TOP-LEVEL FUNCTIONS */
Node *parsemd(FILE *fp);

#endif
