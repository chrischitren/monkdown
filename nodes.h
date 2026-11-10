#ifndef NODES_H
#define NODES_H

/* Maximum attribute value size (including null terminator) */
#define MAX_ATTRIBUTESZ 512

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

/* HELPER FUNCTIONS */
char *gettypestr(int type);
int countnodes(Node *n);
Node *nextnodewithattributekey(Node *n, char *key);

/* NODE HANDLING */
Node *createnode(char *_content, int _contentlen, Node *_parent, int _type);
int clearnodecontent(Node *n);
int freenodes(Node *n);

/* ATTRIBUTE HANDLING */
int addattribute(Node *n, char *key, char *value, int valuelen); 
int removeattribute(Node *n, char *key);
int freeattribute(Attribute *a);
Attribute *getattributebykey(Node *n, char *key);
Attribute *writeattributebykey(char *dest, int destsz, Node *n, char *key);
int getattributestring(char *dest, int destsz, Node *n);

/* DEBUGGING */
void printnodes(Node *n, int depth, int verbose);
void printcharspecial(char c);

#endif
