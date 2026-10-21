## Monkdown

I realized that I had learned a lot from [my first attempt to parse Markdown](https://github.com/chrischitren/site/), but that the project was something of a Rube-Goldbergian abomination of ad hoc solutions to ill-defined problems. In the messy process of learning, all in parallel, what *could* be done, what *should* be done, and what *must* be done, I made something that more closely resembled a collage than a tool. This kind of product is a natural consequence of working with constant access to information.

In pursuit of spiritual enlightenment, or perhaps clarity of mind (if there is a distinction between the two), I decided that I had probably learned enough to solve the problem myself. I defined the discipline I would follow:

- I can only search [this C reference manual](https://en.cppreference.com/c/).
- I can only write code by hand in a text editor (vim).
- Everything must compile, with no errors, to the standard set by `gcc -Wpedantic -Wall -std=c89`.

Like all creative works, it is derivative of the things I exposed myself to during the learning phase, but it is not *plagiaristic;* it is wholly my own construction and thus is wholly representative of my capacity to design. The final product is more extensible, coherent, and probably reliable than the spaghetti of my first attempt, but it is still the product of a single fallible human, and is quite myopic in scope. For one, I am chronically bad at error handling. I have too much faith in edge cases being rare. It is not "production" code because it is not robust or well-tested, and it does not conform to a well-defined specification. It is art code.

### Nodes

The primary struct that allows me to maintain clarity is the Node. Nodes have parents and children and are linked to form a document tree. The fields of a Node that correspond to document content, as well as its list of children, are dynamically allocated to reduce memory usage. A Node has the following fields:

```
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
```

Since `content` and `attributes` are malloc'd strings, they are mutable, and can be manipulated as needed while parsing and writing. This removes a lot of processing overhead. In my first attempt, I was intent on storing as little as possible in each node of the tree, instead choosing to store positions and run-lengths in the source file; a consequence of this decision was that manipulations of the document tree could only happen at the moment of writing, or by implementation of some new flag or indicator stored in each node. For example, I want to change links to `.md` files to be links to `.html` ones. In the new framework, this is a trivial find-and-replace operation that can happen at any time. In my first attempt, it *had* to be done at the moment of writing because the nodes themselves could only reference the source. The code for writing outputs became cumbersome and tangled with context-aware functions to handle all sorts of special cases.

Ideally, the Parser does not "know" that its outputs will eventually become HTML. It "understands" a grammar that is a definition of the mapping between source strings and document trees. Similarly, the Writer does not "know" that the document tree was built by parsing markdown; it wants Nodes that possess metadata and contents, and it makes decisions about what HTML tags should be used based on the metadata, mutates Node contents to its liking, and writes. If we wanted to handle another structured format for representing text with links and styles, the Parser should not have to be modified to accommodate it; a new Writer should be written that takes the same internal, "complete" representation of whatever ideas Markdown is capable of expressing, and maps it to the specification of the new structured format.

### Parsing

The parser works in two stages: a bird's-eye-view separation of the document into contiguous **blocks**, and **inline parsing** of the text content of blocks. Blocks correspond to `<pre>` (code blocks), `<ul>` (unordered lists), `<h1>` and co. (headings), and plain text.

### Writing

This part doesn't quite exist yet. 

