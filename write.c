#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parse.h"
#include "write.h"

/* WRITING SINGLE NODE TREES */
void writehtmlnoderecursive(FILE *wfp, Node *n);
void writehtmlstringescaped(FILE *wfp, char *str);


int writehtmlfromtemplate(FILE *wfp, char *templatedir,
					char *title, Node *header, Node *main, Node *footer) {
	
	FILE *templatefp;
	char c;
	int i;
	char controlbuffer[16];
	
	if ((templatefp = fopen(templatedir, "r")) == NULL) {
		printf("\x1b[31m[write] [writehtmlfromtemplate] "
				"failed to open template: \"%s\"\x1b[0m\n", templatedir);
		return(1);
	}
	
	while ((c = fgetc(templatefp)) != EOF) {
		if (c == '%') {
			c = fgetc(templatefp);
			i = 0;
			while (c != '%' && c != '\n' && c != EOF && i < 15) {
				controlbuffer[i] = c;
				i++;
				c = fgetc(templatefp);
			}
			controlbuffer[i] = '\0';
			if (c == '%') {
				if (strcmp(controlbuffer, "title") == 0) {
					fprintf(wfp, "%s", title);
				} else if (strcmp(controlbuffer, "main") == 0) {
					if (main != NULL) {
						writehtmlnoderecursive(wfp, main);
					}
				} else if (strcmp(controlbuffer, "header") == 0) {
					if (header != NULL) {
						writehtmlnoderecursive(wfp, header);
					}
				} else if (strcmp(controlbuffer, "footer") == 0) {
					if (footer != NULL) {
						writehtmlnoderecursive(wfp, footer);
					}
				} else {
					fprintf(wfp, "%%%s%%", controlbuffer);
				}
			} else {
				printf("In template \"%s\" at pos %ld:\n"
						"    ignoring misidentified or malformed key \"%s\"\n",
						templatedir, ftell(templatefp), controlbuffer);
				fprintf(wfp, "%%%s%c", controlbuffer, c);
			}
		} else {
			fputc(c, wfp);
		}
	}
	fclose(templatefp);
	return(0);
}

void writehtmlnoderecursive(FILE *wfp, Node *n) {
	int i = 0;
	int tag = 1;
	
	if (n->type == TYPE_PLAIN 
			|| n->type == TYPE_ESCAPED 
			|| n->type == TYPE_MAXTYPE) {
		tag = 0;
	}
	
	if (tag) {
		if (n->attributes != NULL) {
			fprintf(wfp, "<%s%s>", gettypestr(n->type), n->attributes);
		} else {
			fprintf(wfp, "<%s>", gettypestr(n->type));
		}
	}
	
	if (n->contentlen > 0) {
		writehtmlstringescaped(wfp, n->content);
	}

	for (i = 0; i < n->numchildren; i++) {
		if (n->children[i] != NULL) {
			writehtmlnoderecursive(wfp, n->children[i]);
		}
	}
	if (tag && n->type != TYPE_IMG) {
		fprintf(wfp, "</%s>", gettypestr(n->type));
	}
}

void writehtmlstringescaped(FILE *wfp, char *str) {
	int i = 0;

	while (*(str+i) != '\0') {
		switch(*(str+i)) {	
			case '&':
				fprintf(wfp, "%s", "&amp;");
				break;
			case '<':
				fprintf(wfp, "%s", "&lt;");
				break;
			case '>':
				fprintf(wfp, "%s", "&gt;");
				break;
			case '"':
				fprintf(wfp, "%s", "&quot;");
				break;
			case '\'':
				fprintf(wfp, "%s", "&#39;");
				break;
			case '\t':
				fprintf(wfp, "    ");
				break;
			default:
				fprintf(wfp, "%c", *(str+i));
				break;
		}
		i++;
	}
}
