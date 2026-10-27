#ifndef WRITE_H
#define WRITE_H

#include "parse.h"

/* FILLING AN HTML TEMPLATE (PAGE BUILDING) */
int writehtmlfromtemplate(FILE *wfp, FILE *templatefp,
					char *title, Node *header, Node *main, Node *footer);

#endif
