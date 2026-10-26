#ifndef PARSE_H
#define PARSE_H

/* Maximum number of "blocks" in a single document */
#define MAX_DOCSZ 256
#define READBUF_SZ 4096

enum nodetype {
	TYPE_PLAIN = 0, /* plain text!!! */
	TYPE_H1, TYPE_H2, TYPE_H3, TYPE_H4, TYPE_H5, TYPE_H6,
	TYPE_IMG, TYPE_BLOCKQUOTE, TYPE_PRE, TYPE_UL,/* TYPE_OL, */
	TYPE_P, TYPE_LI, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE,
	TYPE_ESCAPED, TYPE_MAXTYPE /* used for indexing */
};
typedef struct node Node;

struct node {
	Node *parent;
	Node **children;
	int numchildren;
	char *content;
	char *attributes;
	int contentlen;
	int type;
	int isterminal;
};

/* TOP-LEVEL FUNCTIONS */
Node *parsemd(char *dir);
int freerecursive(Node *n);
int countnodes(Node *n);
char *gettypestr(int type);

#endif
