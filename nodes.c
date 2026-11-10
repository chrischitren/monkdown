#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nodes.h"

/****************************************************************************/
/* HELPER FUNCTIONS                                                         */
/****************************************************************************/

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

int countnodes(Node *n) {
	int i;
	int count = 1;
	
	if (n == NULL) {
		return(0);
	}
	if (n->numchildren == 0) {
		return(count);
	}
	for(i = 0; i < n->numchildren; i++) {
		count += countnodes(n->children[i]);
	}

	return(count);
}

/* Returns the next Node that has an attribute with a matching key, or NULL if
   such a Node does not exist in the remainder of the Node tree. */
Node *nextnodewithattributekey(Node *n, char *key) {
	Node *startn;	
	Node *prevn;
	int i = 0;
	
	startn = n;
	prevn = n;

	/* first: check to make sure the node is not a childless orphan */
	if (n->parent == NULL && n->numchildren == 0) {
		if (getattributebykey(n, key) != NULL) {
			return(n);
		} else {
			return(NULL);
		}
	}
	
	/* search through the node tree by always trying to descend to max depth;
	   if nothing is found, ascend to the closest ancestor with more children
	   and search its next child. */
	while (n != NULL) {
		if (n->numchildren == 0) {
			if (n != prevn) {				
				if (getattributebykey(n, key) != NULL) {
					return(n);
				}
			}
			/* ascend and step forward */
			while (n->parent != NULL) {
				prevn = n;
				n = n->parent; /* ascend */
				/* find prevn in its parent's children: */
				for (i = 0; i < n->numchildren; i++) {
					if (n->children[i] == prevn) {
						break;
					}
				}
				if (i == n->numchildren-1) {
					/* prevn was the last child; cannot step forward
					   ascend and step foward */
					if (n->parent == NULL) {
						return(NULL);
					} else {
						continue;
					}
				} else if (i == n->numchildren) {
					printf("\x1b[31m[pages] [nextnodewithattributekey] "
							"A node is not its parent's child!\x1b[0m\n");
				} else {
					/* step forward */
					n = n->children[i+1];
					break;
				}
			}
		} else {
			/* descend */
			if (getattributebykey(n, key) != NULL) {
				if (startn == n) {
					n = n->children[0];
				} else {
					return(n);
				}
			} else {
				n = n->children[0];			
			}
		}
	}

	return(NULL);
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
	n->numattributes = 0;
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

/* Depth-first search of a Node tree, freeing as we go. */
int freenodes(Node *n) {
	int i = 0;
	
	if (n == NULL) {
		return(1);
	}

	for (i = 0; i < n->numchildren; i++) {
		if (n->children[i] != NULL) {
			freenodes(n->children[i]);
		}
	}

	if(n->attributes != NULL) {
		for (i = 0; i < n->numattributes; i++) {
			freeattribute(n->attributes[i]);
		}
		free(n->attributes);
	}
	
	free(n->children);
	free(n->content);
	free(n);

	return(0);
}

/****************************************************************************/
/* ATTRIBUTE HANDLING                                                       */
/****************************************************************************/

/* Create an Attribute (key-value pair) and append a pointer to it to a Node's
   attributes list. */
int addattribute(Node *n, char *key, char *value, int valuelen) {
	char val[MAX_ATTRIBUTESZ];
	Attribute *a = NULL;

	if (valuelen > MAX_ATTRIBUTESZ-1) {
		valuelen = MAX_ATTRIBUTESZ-1;
		printf("\x1b[31m[parse] [addattribute] "
			"truncating attribute value larger than "
			"MAX_ATTRIBUTESZ=%d\x1b[0m\n", MAX_ATTRIBUTESZ);
	}
	
	strncpy(val, value, valuelen);
	val[valuelen] = '\0';
	
	/* Initialize an Attribute */
	a = malloc(sizeof(Attribute));
	a->key = malloc(sizeof(char) * (strlen(key) + 1));
	a->value = malloc(sizeof(char) * (strlen(val) + 1));

	/* Fill the Attribute's fields with the passed data */
	strcpy(a->key, key);
	strcpy(a->value, val);

	/* Add the attribute to the Node's attributes list */
	if (n->attributes == NULL) {
		n->attributes = malloc(sizeof(Attribute*));
		n->attributes[0] = a;
		n->numattributes = 1;	
	} else {
		n->attributes = realloc(n->attributes, 
						sizeof(Attribute*) * (n->numattributes + 1));
		n->attributes[n->numattributes] = a;
		n->numattributes += 1;
	}
	
	return(0);
}

/* Search for an Attribute with a given key and remove it; shift the remaining
   Attributes to fill the slot and realloc the Node's attributes list. */
int removeattribute(Node *n, char *key) {
	int i, j;
	if (n->attributes == NULL) {
		return(1);
	}
	for (i = 0; i < n->numattributes; i++) {
		if(strcmp(n->attributes[i]->key, key) == 0) {
			freeattribute(n->attributes[i]);
			for (j = i+1; j < n->numattributes; j++) {
				n->attributes[j-1] = n->attributes[j];
			}
			n->attributes = realloc(n->attributes, 
						sizeof(Attribute*) * (n->numattributes - 1));
			n->numattributes -= 1;
			if (n->numattributes == 0) {
				free(n->attributes);
				n->attributes = NULL;
			}
			break;
		}
	}
	return(0);
}

/* Free an Attribute and the char arrays it points to. */
int freeattribute(Attribute *a) {
	if (a == NULL) {
		return(1);
	}
	if (a->key != NULL) {
		free(a->key);
	}
	if (a->value != NULL) {
		free(a->value);
	}
	free(a);
	return(0);
}

/* Search for a given key in a Node's attributes. If found, copy its value
   into a destination buffer. Returns a pointer to the matching Attribute,
   or NULL if not found. */
Attribute *getattributebykey(Node *n, char *key) {
	int i;
	for (i = 0; i < n->numattributes; i++) {
		if (strcmp(n->attributes[i]->key, key) == 0) {
			return(n->attributes[i]);
		}
	}
	return(NULL);
}

/* Search for a given key in a Node's attributes. If found, copy its value
   into a destination buffer. Returns a pointer to the matching Attribute,
   or NULL if not found. */
Attribute *writeattributebykey(char *dest, int destsz, Node *n, char *key) {
	int i;
	for (i = 0; i < n->numattributes; i++) {
		if (strcmp(n->attributes[i]->key, key) == 0) {
			if (strlen(n->attributes[i]->value)+1 > destsz) {
				printf("\x1b[31m[parse] [getattributebykey] "
					"attribute value truncated and likely malformed:\n"
					"destination buffer of size %d is too small\x1b[0m\n",
					destsz);
			}
			strncpy(dest, n->attributes[i]->value, destsz-1);
			return(n->attributes[i]);
		}
	}
	return(NULL);
}

/* Copy a string of form ' key1="value1" key2="value2"' into a destination
   buffer containing all the key-value pairs of a Node's attributes list.
   Copies a truncated string and returns 1 if the destination buffer is too
   small. Returns 0 on success. */
int getattributestring(char *dest, int destsz, Node *n) {
	int i = 0;
	int ncopied = 0;
	char buf[MAX_ATTRIBUTESZ];
	buf[0] = '\0';
	dest[0] = '\0';

	while (i < n->numattributes) {
		buf[0] = '\0';
		if (n->attributes[i] != NULL) {
			strcpy(buf, " ");
			strcat(buf, n->attributes[i]->key);
			strcat(buf, "=\"");
			strcat(buf, n->attributes[i]->value);
			strcat(buf, "\"");
		}
		if (ncopied + strlen(buf) < destsz) {
			strcat(dest, buf);
		} else {
			printf("\x1b[31m[parse] [getattributestring] "
				"attribute string truncated and likely malformed:\n"
				"destination buffer of size %d is too small\x1b[0m\n", destsz);
			strncat(dest, buf, destsz - ncopied - 1);
			return(1);
		}
		ncopied += strlen(buf);
		i++;
	}
	
	return(0);
}

/****************************************************************************/
/* DEBUGGING                                                                */
/****************************************************************************/

/* DEBUG: Print the nodes recursively and all their metadata. */
void printnodes(Node *n, int depth, int verbose) {
	int i;
	char padstring[64];
	char buf[MAX_ATTRIBUTESZ*8];

	getattributestring(buf, MAX_ATTRIBUTESZ*8, n);

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
		printf("\x1b[32m%s\x1b[0m", buf);
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
