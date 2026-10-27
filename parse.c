#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parse.h"

/* INLINE PARSING (RECURSIVE) */
int rscansplit(Node *doc);

int scansplit(Node *n);

int scanemph(char *str, int *outertype);
int scancode(char *str);
int scanlink(char *str);

void splitesc(Node *parent, int pos, int len); 
void splitlink(Node *parent, int pos, int len, int type);
void splitemph(Node *parent, int pos, int len, int type);
void splitcode(Node *parent, int pos, int len);

/* BLOCK-LEVEL PARSING (NOT RECURSIVE) */
int parseblocklevel(Node *doc);
int handleh(Node *n);
int handleul(Node *n);
int handleblockquote(Node *n);

/* DOCUMENT BLOCK IDENTIFICATION */
int readtonodes(FILE *fp, Node *_parent);
int getblocktype(char *s, int sz);

/* NODE HANDLING */
Node *createnode(char *_content, int _contentlen, Node *_parent, int _type);
int clearnodecontent(Node *n);
int addattribute(Node *n, const char *key, char *str, int len);
int freenodes(Node *n);

/* DEBUGGING */
void printnodes(Node *n, int depth, int verbose);
void printscanresults(Node *n, int outputs[MAX_DOCSZ][3], int osz);
void printcharspecial(char c);


/****************************************************************************/
/* TOP-LEVEL FUNCTIONS (defined in parse.h)                                 */
/****************************************************************************/

Node *parsemd(FILE *fp) {
	Node *docnode;
	int maxdepth = MAX_DOCSZ;
	docnode = createnode("", 0, NULL, TYPE_MAXTYPE);
	readtonodes(fp, docnode);
	parseblocklevel(docnode);
/*	printnodes(docnode, 0, 1);*/
	while (maxdepth > 0 && rscansplit(docnode) != 0) {
		maxdepth--;
	}
/*	printnodes(docnode, 0, 1);*/
	return(docnode);
}

int freerecursive(Node *n) {
	return(freenodes(n));
}

int countnodes(Node *n) {
	int i;
	int count = 1;
	if (n->numchildren == 0) {
		return(count);
	}
	for(i = 0; i < n->numchildren; i++) {
		count += countnodes(n->children[i]);
	}

	return(count);
}

char *gettypestr(int type) {
	switch (type) {
		case TYPE_PLAIN: return("PLAIN"); break;
		case TYPE_H1: return("h1"); break;
		case TYPE_H2: return("h2"); break;
		case TYPE_H3: return("h3"); break;
		case TYPE_H4: return("h4"); break;
		case TYPE_H5: return("h5"); break;
		case TYPE_H6: return("h6"); break;
		case TYPE_IMG: return("img"); break;
		case TYPE_BLOCKQUOTE: return("blockquote"); break;
		case TYPE_PRE: return("pre"); break;
		case TYPE_UL: return("ul"); break;
		case TYPE_P: return("p"); break;
		case TYPE_LI: return("li"); break;
		case TYPE_EM: return("em"); break;
		case TYPE_STRONG: return("strong"); break;
		case TYPE_A: return("a"); break;
		case TYPE_CODE: return("code"); break;	
		case TYPE_ESCAPED: return("ESC"); break;
		case TYPE_MAXTYPE: return("MAXTYPE"); break;
		default: return("INVALID TYPE"); break;
	}
}

/****************************************************************************/
/* INLINE PARSING (RECURSIVE)                                               */
/****************************************************************************/

int rscansplit(Node *doc) {
	int i, hits = 0;
	
	for (i = 0; i < doc->numchildren; i++) {
		if (doc->children[i] != NULL) {
			hits += rscansplit(doc->children[i]);
		}
	}
	
	if (doc->numchildren == 0 && doc->isterminal == 0) {
		hits += scansplit(doc);
	}
	
	return(hits);
}


int scansplit(Node *n) {
	int i, j = 0;
	int scanresult = -1;
	int scantype;
	int endpos, nextstart;
	int outputs[MAX_DOCSZ][3];
	int osz = 0;

	/* Somehow we would like to check every type for only its own control
	   characters. We need to know what must be escaped in each context. */
	const char *controlchars = "[!`*\\";

	for (i = 0; i < n->contentlen; i++) {
		if (n->content[i] == '\\' && n->content[i+1] != '\0') {
			if (strchr(controlchars, n->content[i+1]) != NULL) {
				outputs[j][0] = i;
				outputs[j][1] = 2;
				outputs[j][2] = TYPE_ESCAPED;
				j++;
				i += 1;
				continue;
			}
		}
		switch(n->content[i]) {
			case '[':
				scanresult = scanlink(n->content+i);
				scantype = TYPE_A;
				break;
			case '!':
				scanresult = scanlink(n->content+i);
				scantype = TYPE_A;
				break;
			case '`':	
				scanresult = scancode(n->content+i);
				scantype = TYPE_CODE;
				break;
			case '*':
				scanresult = scanemph(n->content+i, &scantype);
				break;
			default:
				scanresult = -1;
				scantype = TYPE_PLAIN;
				break;
		}
		if (scanresult != -1) {	
			outputs[j][0] = i;
			outputs[j][1] = scanresult;
			outputs[j][2] = scantype;
			i += scanresult;
			j++;
		}
	}

	outputs[j][0] = n->contentlen;

	osz = j;

	/* if no hits were found */
	if (osz == 0) {
		n->isterminal = 1;
		return(0);
	}

	if (outputs[0][0] != 0) {
		createnode(n->content, outputs[0][0], n, TYPE_PLAIN);
	}

	/* otherwise, loop through the hits and start splitting the node */
	for (j = 0; j < osz; j++) {
		switch (outputs[j][2]) {
			case TYPE_A:
				splitlink(n, outputs[j][0], outputs[j][1], outputs[j][2]);
				break;
			case TYPE_EM:
			case TYPE_STRONG:
				splitemph(n, outputs[j][0], outputs[j][1], outputs[j][2]);
				break;
			case TYPE_CODE:
				splitcode(n, outputs[j][0], outputs[j][1]);
				break;
			case TYPE_ESCAPED:
				splitesc(n, outputs[j][0], outputs[j][1]);
			default:
				break;
		}
		endpos = outputs[j][0]+outputs[j][1];
		nextstart = outputs[j+1][0];
		if (endpos < nextstart) {
			createnode(n->content+endpos, nextstart-endpos, n, TYPE_PLAIN);
		}
	}
	
	/* debug printing */
/*	printscanresults(n, outputs, osz);*/
	
	clearnodecontent(n);

	return(osz);
}

void splitesc(Node *parent, int pos, int len) {
	Node *escnode;
	escnode = createnode(parent->content+pos+1, 1, parent, TYPE_PLAIN);
	escnode->isterminal = 1;
}

void splitlink(Node *parent, int pos, int len, int type) {
	int isimage = 0;
	char *text;/*= parent->content+pos+1;*/
	int textlen;
	char *url;/*= strstr(parent->content+pos+1, "](") + 2;*/
	int urllen;
	char *separator;	
	Node *anode = NULL;
	Node *imgnode = NULL;

	separator = strstr(parent->content+pos, "](");

	if (*(parent->content+pos) == '!') {
		isimage = 1;
	}

	text = parent->content+pos+1+isimage;
	textlen = (int) (separator-text);
	url = separator+2;
	urllen = len - (int) (separator-text) - 4 - isimage;
	
	anode = createnode(text, textlen, parent, TYPE_A);
	
	addattribute(anode, "href", url, urllen);

	if (isimage) {
		clearnodecontent(anode);
		imgnode = createnode("", 0, anode, TYPE_IMG);
		addattribute(imgnode, "src", url, urllen);
		addattribute(imgnode, "alt", text, textlen);
		imgnode->isterminal = 1;
		anode->isterminal = 1;
	}
}

void splitemph(Node *parent, int pos, int len, int type) {
	if (type == TYPE_EM) {
		createnode(parent->content+pos+1, len-2, parent, TYPE_EM);
	} else { 
		createnode(parent->content+pos+2, len-4, parent, TYPE_STRONG);
	}
} 

void splitcode(Node *parent, int pos, int len) {
	int i;

	Node *newnode;
	
	newnode = createnode(parent->content+pos+1, len-2, parent, TYPE_CODE);
	
	for (i = 0; i < len-2; i++) {
		if (newnode->content[i] == '\\' && newnode->content[i+1] == '`') {
			memmove(newnode->content+i, newnode->content+i+1, len-2-i);
		}
	}
	
	newnode->isterminal = 1;
}


int scanemph(char *str, int *outertype) {
	int i = 1;
	int j;
	int depth = 1;
	int startdepth = 1;

	if (str[0] != '*' || strlen(str) < 2) {
		return(-1);
	}
	
	*outertype = TYPE_EM;

	if (str[1] == '*') {
		*outertype = TYPE_STRONG;
		startdepth = 2;
		depth = 2;
		i++;
		if (str[2] == '*') {
			startdepth = 3;
			depth = 3;
			i++;
		}
	}

	while (str[i] != '\0') {
		if (str[i] == '\\' && str[i+1] != '\0') {
			i += 2;
			continue;
		} 

		if (str[i] != '*') {
			i++;
			continue;
		}

		/* Find the run length we've come across */
		if (str[i] == '*') {
			j = 1;
			while (str[i+j] == '*' && j < 3 && str[i+j] != '\0') {
				j++;
			}

			if (startdepth != 3) {
				if (depth == 3) {
					if (j == 3) {
						return(i+3);
					} else {
						if (j == startdepth) {
							return(i+startdepth);
						} else {
							depth = startdepth;
							i += j;
							continue;
						}
					}
				} else {
					if (j == 3) {
						return(i + startdepth);
					} else {
						if (j == startdepth) {
							return(i + startdepth);
						} else {
							depth = 3;
							i += j;
							continue;
						}
					}
				}
			} else {
				if (depth == 3) {
					if (j == 3) {
						*outertype = TYPE_STRONG;
						return(i+3);
					} else {
						depth = 3-j;
						i += j;
						continue;
					}
				} else {
					if (j == 3 || j == depth) {
						*outertype = (depth == 2) ? TYPE_STRONG : TYPE_EM;	
						return(i + depth);
					} else {
						depth = 3;
						i += j;
						continue;
					}
				}
			}
		}
	}
	
	return(-1);
}

int scancode(char *str) {
	int i = 1;
	if (str[0] != '`' || strlen(str) < 2) {
		return(-1);
	}
	while (str[i] != '\0')  {
		if (str[i] == '\\' && str[i+1] == '`') {
			i += 2;
			continue;
		}
		if (str[i] == '`') {
			return(i+1);
		}
		i++;
	}
	return(-1);
}

int scanlink(char *str) {
	int i = 0;
	int inside = 0;

	if (str[0] == '!' && str[1] != '[') {
		return(-1);
	}

	if (strlen(str) < 6) {
		return(-1);
	}

	while (str[i] != '\0') {
		if (str[i] == '\\' && str[i+1] != '\0') {
			i += 2;
			continue;
		}
		if (inside == 0 && str[i] == '[') {
			inside = 1;
		}
		if (inside == 1 && strncmp(str+i, "](", 2) == 0) {
			inside = 2;
		}
		if (inside == 2 && str[i] == ')') {
			return(i+1);
		}
		i++;
	}
	return(-1);
}



/****************************************************************************/
/* BLOCK-LEVEL PARSING (NOT RECURSIVE)                                      */
/****************************************************************************/

/* Removes leading characters from block-level types (blockquote, ul, pre) */
int parseblocklevel(Node *doc) {
	int i;
	for (i = 0; i < doc->numchildren; i++) {
		if (doc->children[i] != NULL) {
			switch(doc->children[i]->type) {
				case TYPE_BLOCKQUOTE:
					handleblockquote(doc->children[i]);
					break;
				case TYPE_UL:
					handleul(doc->children[i]);
					break;
				case TYPE_PRE:
					doc->children[i]->isterminal = 1;
					break;
				default:
					break;
			}
			if (doc->children[i]->type >= TYPE_H1
				&& doc->children[i]->type <= TYPE_H6) {
				handleh(doc->children[i]);
			}
		}
	}
	return(0);
}


int handleh(Node *n) {
	int i = n->type - TYPE_H1 + 1;
	if (n->content[i] == ' ') {
		i++;
	}
	memmove(n->content, n->content+i, n->contentlen - i);
	n->contentlen = n->contentlen-i;
	n->content[n->contentlen] = '\0';
	return(0);
}


int handleul(Node *n) {
	int j;
	int i = 0;
	int laststart = 0;

	if (n->content[0] != '-') {
		printf("\x1b[31m[parse] [handleul] "
				"passed node's content does not begin with '-':\n"
				"\x1b[91m%s\x1b[0m\n", n->content);
		return(1);
	}

	if (n->content[1] == ' ') {
		laststart = 2;
	} else {
		laststart = 1;
	}
	
	/* Split <ul> chunk into <li> nodes (if a line starts with a hyphen) */
	while (i <= n->contentlen) {
		if (n->content[i] == '\n') {
			if (strncmp(n->content+i, "\n- ", 3) == 0) {
				createnode(n->content+laststart, i-laststart, n, TYPE_LI);
				laststart = i+3;
			} else if (strncmp(n->content+i, "\n-", 2) == 0) {
				createnode(n->content+laststart, i-laststart, n, TYPE_LI);
				laststart = i+2;
			}
		}
		if (n->content[i] == '\0') {
			createnode(n->content+laststart, i-laststart, n, TYPE_LI);
		}
		i++;
	}
	
	/* Split every <li> node into <p> nodes that contain each line */
	for (j = 0; j < n->numchildren; j++) {
		if (n->children[j] != NULL) {
			i = 0;
			laststart = 0;
			while (i <= n->children[j]->contentlen) {
				if (n->children[j]->content[i] == '\n'
							|| n->children[j]->content[i] == '\0') {
					createnode(n->children[j]->content+laststart,
								i-laststart, n->children[j], TYPE_P);
					laststart = i+1;
				}
				i++;
			}
			n->children[j]->content = 
							realloc(n->children[j]->content, sizeof(char)*1);
			n->children[j]->contentlen = 0;
			n->children[j]->content[0] = '\0';
		}
	}
	n->content = realloc(n->content, sizeof(char)*1);
	n->contentlen = 0;
	n->content[0] = '\0';
	return(0);
}


int handleblockquote(Node *n) {
	int i = 0;
	int laststart = 0;
	int writeskip = 0;
	int rightalign = 0;
	Node *linenode;
	
	if (n->content[0] != '>') {
		printf("\x1b[31m[parse] [handleblockquote] "
				"passed node's content does not begin with '>':\n"
				"\x1b[91m%s\x1b[0m\n", n->content);
		return(1);
	}
	while (i <= n->contentlen) {
		if (n->content[i] == '\n' || n->content[i] == '\0') {
			if (strncmp(n->content+laststart, ">> ", 3) == 0) {
				writeskip = 3;
				rightalign = 1;
			} else if (strncmp(n->content+laststart, ">>", 2) == 0) {
				writeskip = 2;
				rightalign = 1;
			} else if (strncmp(n->content+laststart, "> ", 2) == 0) {
				writeskip = 2;
			} else if (n->content[laststart] == '>') {
				writeskip = 1;
			} else {
				writeskip = 0;
			}
			if (laststart != i) {
				linenode = createnode(n->content+laststart+writeskip,
									i-laststart-writeskip, n, TYPE_P);
				if (rightalign == 1) {
					addattribute(linenode, "class", "right", strlen("right"));
				}
			}
			laststart = i+1;
		}
		i++;
	}
	n->content = realloc(n->content, sizeof(char)*1);
	n->contentlen = 0;
	n->content[0] = '\0';
	return(0);
}

/****************************************************************************/
/* DOCUMENT BLOCK IDENTIFICATION                                            */
/****************************************************************************/

/* Open a file and write the whole thing to nodes, chunk-wise */
int readtonodes(FILE *fp, Node *_parent) {
	char buffer[READBUF_SZ];
	int readlen;
	
	/* Outer loop to skip blank lines */
	while (fgets(buffer, READBUF_SZ, fp) != NULL) {
		if (buffer[0] == '\n') {
			continue;
		} else {
			readlen = strlen(buffer);
			/* If we're entering a code block, loop until we find the end */
			if (strncmp("```", buffer, 3) == 0) {
				readlen = 0;
				while(fgets(buffer+readlen, READBUF_SZ-readlen, fp) != NULL) {
					if (strncmp("```", buffer+readlen, 3) == 0) {
						readlen--;
						buffer[readlen] = '\0';
						break;
					} else {
						readlen = strlen(buffer);
					}
				}
				createnode(buffer, readlen, _parent, TYPE_PRE);
			} else { /* otherwise, loop until we find a blank line (or EOF) */
				while (fgets(buffer+readlen, READBUF_SZ-readlen, fp) != NULL) {
					if (buffer[readlen] == '\n') {
						readlen--;
						buffer[readlen] = '\0';
						break;
					} else {
						readlen = strlen(buffer);
					}
				}
				createnode(buffer, readlen,    /* content, contentlen */
					_parent,                    /* parent             */
					getblocktype(buffer, readlen)); /* type (computed)    */
			}
		}
	}
	return(0);
}

int getblocktype(char *s, int sz) {
	int i = 0;
	switch (s[0]) {
		case '>':
			return(TYPE_BLOCKQUOTE);
			break;
		case '-':
			return(TYPE_UL);
			break;
		case '#':
			while (i < sz && i < 6 && s[i] == '#') {
				i++;
			}
			return(TYPE_H1 + i - 1);
			break;
		default:
			return(TYPE_P);
			break;
	}
	return(TYPE_MAXTYPE);
}

/****************************************************************************/
/* NODE HANDLING                                                            */
/****************************************************************************/

/* Create a pointer to a Node that contains string content. */
/* Content string is null terminated by this routine. */
Node *createnode(char *_content, int _contentlen, Node *_parent, int _type) {
	Node *n;
	
	/* Initialize the return variable */
	n = malloc(sizeof(Node));
	
	n->parent = _parent;

	if (_parent != NULL) {
			_parent->children = realloc(_parent->children,
									sizeof(Node*) * (_parent->numchildren+1));
			_parent->children[_parent->numchildren] = n;
			_parent->numchildren += 1;
	}

	n->children = NULL;
	n->numchildren = 0;
	
	/* Write the string content to the allocated Node */
	n->content = malloc(sizeof(char) * (_contentlen+1));
	memcpy(n->content, _content, _contentlen);
	n->content[_contentlen] = '\0';
	
	/* Write metadata */
	n->attributes = NULL;
	n->contentlen = _contentlen;
	n->type = _type;
	n->isterminal = 0;
	
	/* Give back the pointer to the newly allocated Node */
	return(n);
}



int clearnodecontent(Node *n) {
	if (n->content != NULL) {
		n->content = realloc(n->content, sizeof(char)*1);
		n->contentlen = 0;
		n->content[0] = '\0';
		return(0);
	} else {
		printf("\x1b[31m[parse] [clearnodecontent] "
				"Node content is NULL\x1b[0m\n");
		return(1);
	}
}


int addattribute(Node *n, const char *key, char *str, int len) {
	char *buffer;
	int buffersz;
	buffersz = len+strlen(key)+6;
	buffer = malloc(sizeof(char) * buffersz);
	/* 'key="str" 0' (where 0 is null terminator) */
	/*     12   345  extra characters needed      */

	strcpy(buffer, " ");
	strcat(buffer, key);
	strcat(buffer, "=\"");
	strncat(buffer, str, len);
	strcat(buffer, "\"");

	if (n->attributes == NULL) {
		n->attributes = malloc(sizeof(char) * buffersz);
		strcpy(n->attributes, buffer);
	} else {
		n->attributes = realloc(n->attributes, 
						sizeof(char)*(buffersz+strlen(n->attributes)+1) );
		strcat(n->attributes, buffer);
	}
	free(buffer);
	return(0);
}


/* Depth-first search of a Node tree, freeing as we go. */
int freenodes(Node *n) {
	int i = 0;
	
	for (i = 0; i < n->numchildren; i++) {
		if (n->children[i] != NULL) {
			freenodes(n->children[i]);
		}
	}

	if(n->attributes != NULL) {
		free(n->attributes);
	}
	
	free(n->children);
	free(n->content);
	free(n);

	return(0);
}


/****************************************************************************/
/* DEBUGGING                                                                */
/****************************************************************************/

/* DEBUG: Print the nodes recursively and all their metadata. */
void printnodes(Node *n, int depth, int verbose) {
	int i;
	char padstring[64];

	memset(padstring, ' ', 64);
	for (i = 0; i < depth; i++) {
		padstring[i*4] = '|';
	}
	padstring[4*depth] = '\0';
	
	if (!(n->isterminal)) {
		printf("%s+---- %s", padstring, gettypestr(n->type));
	} else {
		printf("%s+---- \x1b[36m%s\x1b[0m", padstring, gettypestr(n->type));
	}
	if (n->attributes != NULL) {
		printf("\x1b[32m%s\x1b[0m", n->attributes);
	}
	if (n->contentlen == 0) {
		printf("\x1b[0m\n%s| \x1b[93m", padstring);
	}
	for (i = 0; i < n->contentlen; i++) {
		if (i%65 == 0) {
			printf("\x1b[0m\n%s| \x1b[93m", padstring);
		}
		printcharspecial(*(n->content + i));
	}
	printf("\x1b[0m\n");

	for (i = 0; i < n->numchildren; i++) {
		if (n->children[i] != NULL) {
			printnodes(n->children[i], depth+1, verbose);
		}
	}

	printf("%s+----\n%s\n", padstring, padstring);
}

void printscanresults(Node *n, int outputs[MAX_DOCSZ][3], int osz) {
	int i,k,l;

	printf("\x1b[93m");
	for (i = 0; i <= n->contentlen; i++) {
		if ((i%90 == 0 || i == n->contentlen) && i != 0) {
			for (k = 0; k < osz; k++) { /* for each of the scans */
				printf("\n\x1b[32m");
				if (i == n->contentlen) {
					l = i - (i%90);
				} else {
					l = i-90;
				}
				for (; l < i; l++) {
					if (l >= outputs[k][0]
						&& l < outputs[k][0]+outputs[k][1]) {
						printf("^");
					} else {
						printf(" ");
					}
				}
				if (i == n->contentlen) {
					printf("%s", gettypestr(outputs[k][2]));
				}
			}
			printf("\n\x1b[93m");
		}
		printcharspecial(n->content[i]);
	}	
	printf("\x1b[0m\n------------------------------"
			"------------------------------"
			"------------------------------\n");
}

void printcharspecial(char c) {
	switch (c) {
		case '\t': 
			printf("\x1b[31mt\x1b[93m");
			break;
		case '\n':
			printf("\x1b[31mn\x1b[93m");
			break;
		case ' ':
			printf("\x1b[31m_\x1b[93m");
			break;
		case '\0':
			printf("\x1b[31m0\x1b[93m");
		default:
			printf("%c", c);
			break;
	}
}
