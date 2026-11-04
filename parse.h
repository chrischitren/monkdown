#ifndef PARSE_H
#define PARSE_H

/* Maximum number of "blocks" in a single document */
#define MAX_DOCSZ 512
/* Maximum size of a single block */
#define READBUF_SZ 4096
/* Maximum attribute value size (including null terminator) */
#define MAX_ATTRIBUTESZ 256

enum nodetype {
	TYPE_PLAIN = 0, /* plain text!!! */
	TYPE_H1, TYPE_H2, TYPE_H3, TYPE_H4, TYPE_H5, TYPE_H6,
	TYPE_IMG, TYPE_BLOCKQUOTE, TYPE_PRE, TYPE_UL,/* TYPE_OL, */
	TYPE_P, TYPE_LI, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE,
	TYPE_ESCAPED, TYPE_MAXTYPE /* used for indexing */
};

typedef struct attribute Attribute;
struct attribute {
	char *key;
	char *value;
};

typedef struct node Node;
struct node {
	Node *parent;
	Node **children;
	int numchildren;
	char *content;
	int contentlen;
	Attribute **attributes;
	int numattributes;
	int type;
	int isterminal;
};


/* TOP-LEVEL FUNCTIONS */
Node *parsemd(FILE *fp);
int countnodes(Node *n);
char *gettypestr(int type);

/* NODE HANDLING */
Node *createnode(char *_content, int _contentlen, Node *_parent, int _type);
int clearnodecontent(Node *n);
int freenodes(Node *n);

/* ATTRIBUTE HANDLING */
int addattribute(Node *n, char *key, char *value, int valuelen); 
int removeattribute(Node *n, char *key);
int freeattribute(Attribute *a);
Attribute *getattributebykey(char *dest, int destsz, Node *n, char *key);
int getattributestring(char *dest, int destsz, Node *n);

/* DEBUGGING */
void printnodes(Node *n, int depth, int verbose);
void printcharspecial(char c);

#endif
