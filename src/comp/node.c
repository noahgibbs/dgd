# include "comp.h"
# include "str.h"
# include "array.h"
# include "object.h"
# include "xfloat.h"
# include "data.h"
# include "interpret.h"
# include "macro.h"
# include "token.h"
# include "node.h"

# define NODE_CHUNK	128

typedef struct _nodelist_ {
    struct _nodelist_ *next;		/* next in linked list */
    node n[NODE_CHUNK];			/* node array */
} nodelist;

static nodelist *list;			/* linked list of all node chunks */
static nodelist *flist;			/* list of free node chunks */
static int chunksize = NODE_CHUNK;	/* part of list chunk used */
int nil_node;				/* N_NIL or N_INT */

/*
 * NAME:	node->init()
 * DESCRIPTION:	initialize node handling
 */
void node_init(flag)
bool flag;
{
    nil_node = (flag) ? N_NIL : N_INT;
}

/*
 * NAME:	node->new()
 * DESCRIPTION:	create a new node
 */
node *node_new(line)
unsigned int line;
{
    register node *n;

    if (chunksize == NODE_CHUNK) {
	register nodelist *l;

	if (flist != (nodelist *) NULL) {
	    l = flist;
	    flist = l->next;
	} else {
	    l = CALLOC(nodelist, 1);
	}
	l->next = list;
	list = l;
	chunksize = 0;
    }
    n = &list->n[chunksize++];
    n->type = N_INT;
    n->flags = 0;
    n->line = line;
    return n;
}

/*
 * NAME:	node->int()
 * DESCRIPTION:	create an integer node
 */
node *node_int(num)
Int num;
{
    register node *n;

    n = node_new(tk_line());
    n->type = N_INT;
    n->flags = F_CONST;
    n->mod = T_INT;
    n->l.number = num;

    return n;
}

/*
 * NAME:	node->float()
 * DESCRIPTION:	create a float node
 */
node *node_float(flt)
xfloat *flt;
{
    register node *n;

    n = node_new(tk_line());
    n->type = N_FLOAT;
    n->flags = F_CONST;
    n->mod = T_FLOAT;
    NFLT_PUT(n, *flt);

    return n;
}

/*
 * NAME:	node->nil()
 * DESCRIPTION:	create a nil node
 */
node *node_nil()
{
    register node *n;

    n = node_new(tk_line());
    n->type = nil_node;
    n->flags = F_CONST;
    n->mod = nil_type;
    n->l.number = 0;

    return n;
}

/*
 * NAME:	node->str()
 * DESCRIPTION:	create a string node
 */
node *node_str(str)
string *str;
{
    register node *n;

    n = node_new(tk_line());
    n->type = N_STR;
    n->flags = F_CONST;
    n->mod = T_STRING;
    str_ref(n->l.string = str);
    n->r.right = (node *) NULL;

    return n;
}

/*
 * NAME:	node->fcall()
 * DESCRIPTION:	create a function call node
 */
node *node_fcall(mod, func, call)
int mod;
char *func;
Int call;
{
    register node *n;

    n = node_new(tk_line());
    n->type = N_FUNC;
    n->mod = mod;
    n->l.ptr = func;
    n->r.number = call;

    return n;
}

/*
 * NAME:	node->mon()
 * DESCRIPTION:	create a node for a monadic operator
 */
node *node_mon(type, mod, left)
int type, mod;
node *left;
{
    register node *n;

    n = node_new(tk_line());
    n->type = type;
    n->mod = mod;
    n->l.left = left;
    n->r.right = (node *) NULL;

    return n;
}

/*
 * NAME:	node->bin()
 * DESCRIPTION:	create a node for a binary operator
 */
node *node_bin(type, mod, left, right)
int type, mod;
node *left, *right;
{
    register node *n;

    n = node_new(tk_line());
    n->type = type;
    n->mod = mod;
    n->l.left = left;
    n->r.right = right;

    return n;
}

/*
 * NAME:	node->toint()
 * DESCRIPTION:	convert node type to integer constant
 */
void node_toint(n, i)
register node *n;
Int i;
{
    if (n->type == N_STR) {
	str_del(compenv, n->l.string);
    }
    n->type = N_INT;
    n->flags = F_CONST;
    n->l.number = i;
}

/*
 * NAME:	node->tostr()
 * DESCRIPTION:	convert node type to string constant
 */
void node_tostr(n, str)
register node *n;
string *str;
{
    str_ref(str);
    if (n->type == N_STR) {
	str_del(compenv, n->l.string);
    }
    n->type = N_STR;
    n->flags = F_CONST;
    n->l.string = str;
}

/*
 * NAME:	node->free()
 * DESCRIPTION:	free all nodes
 */
void node_free()
{
    register nodelist *l;
    register int i;

    i = chunksize;
    for (l = list; l != (nodelist *) NULL; ) {
	register node *n;
	register nodelist *f;

	n = &l->n[i];
	do {
	    --n;
	    if (n->type == N_STR) {
		/*
		 * only strings are deleted here
		 */
		str_del(compenv, n->l.string);
	    }
	} while (--i > 0);
	i = NODE_CHUNK;
	f = l;
	l = l->next;
	f->next = flist;
	flist = f;
    }
    list = (nodelist *) NULL;
    chunksize = NODE_CHUNK;
}

/*
 * NAME:	node->clear()
 * DESCRIPTION:	cleanup after node handling
 */
void node_clear()
{
    register nodelist *l;

    node_free();
    for (l = flist; l != (nodelist *) NULL; ) {
	register nodelist *f;

	f = l;
	l = l->next;
	CFREE(f);
    }
    flist = (nodelist *) NULL;
}
