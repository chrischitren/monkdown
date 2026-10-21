#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "parse.h"

void printhtmlrecursive(Node *n, FILE *fp);
void writeescapedhtml(FILE *fp, char *str);

int main() {
	Node *doc;
	FILE *wfp;
	const char *readdir = "README.md";
	char writedir[256];
	struct stat filestat;

	strcpy(writedir, readdir);
	strcpy(strstr(writedir, ".md"), ".html");
	
	printf("\x1b[93m%s\x1b[0m\n", writedir);

	if (stat(writedir, &filestat) == 0) {
		printf("\x1b[31m[write] [main] "
				"refusing to overwrite \"%s\"\x1b[0m\n", writedir);
		return(1);
	}

	if ((wfp = fopen(writedir, "w")) == NULL) {
		printf("\x1b[31m[write] [main] "
				"failed to open for writing: \"%s\"\x1b[0m\n", writedir);
		return(1);
	}
		
	doc = parsemd("README.md");
		
	printhtmlrecursive(doc, wfp);

	freerecursive(doc);
	
	fclose(wfp);

	return(0);
}

void printhtmlrecursive(Node *n, FILE *fp) {
	int i = 0;
	int tag = 1;

	if (n->type == TYPE_PLAIN 
			|| n->type == TYPE_ESCAPED 
			|| n->type == TYPE_MAXTYPE) {
		tag = 0;
	}
	
	if (tag) {
		if (n->attributes != NULL) {
			fprintf(fp, "<%s%s>", type_str[n->type], n->attributes);
		} else {
			fprintf(fp, "<%s>", type_str[n->type]);
		}
	}

	if (n->contentlen > 0) {
		writeescapedhtml(fp, n->content);
	}

	for (i = 0; i < n->numchildren; i++) {
		if (n->children[i] != NULL) {
			printhtmlrecursive(n->children[i], fp);
		}
	}
	if (tag && n->type != TYPE_IMG) {
		fprintf(fp, "</%s>", type_str[n->type]);
	}
}

void writeescapedhtml(FILE *fp, char *str) {
	int i = 0;

	while (*(str+i) != '\0') {
		switch(*(str+i)) {	
			case '&':
				fprintf(fp, "%s", "&amp;");
				break;
			case '<':
				fprintf(fp, "%s", "&lt;");
				break;
			case '>':
				fprintf(fp, "%s", "&gt;");
				break;
			case '"':
				fprintf(fp, "%s", "&quot;");
				break;
			case '\'':
				fprintf(fp, "%s", "&#39;");
				break;
			case '\t':
				fprintf(fp, "    ");
				break;
			default:
				fprintf(fp, "%c", *(str+i));
				break;
		}
		i++;
	}
}
