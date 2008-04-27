#include "tintin.h"
#include "protos.h"

#define DELETED_HASHENTRY ((char*)init_hash)
#define crc(x) ((unsigned int)crc32s(x))

/**********************************/
/* initialize an empty hash table */
/**********************************/
struct hashtable* init_hash()
{
    struct hashtable *h=TALLOC(struct hashtable);
    h->size=8;
    h->nent=0;
    h->nval=0;
    h->tab=CALLOC(8, struct hashentry);
    return h;
}


/*********************/
/* free a hash table */
/*********************/
void kill_hash(struct hashtable* h)
{
    int i;
    
    if (h->nval)
        for (i=0; i<h->size; i++)
        {
            if (h->tab[i].left && (h->tab[i].left!=DELETED_HASHENTRY))
            {
                SFREE(h->tab[i].left);
                SFREE(h->tab[i].right);
            }
        };
    CFREE(h->tab, h->size, struct hashentry);
    TFREE(h, struct hashtable);
}


static inline void add_hash_value(struct hashtable *h, char *left, char *right)
{
    int i;
    i=crc(left)%h->size;
    while (h->tab[i].left)
    {
        if (!i)
            i=h->size-1;
        i--;
    }
    h->tab[i].left = left;
    h->tab[i].right= right;
}



static inline void rehash(struct hashtable *h, int s)
{
    int i,gs;
    struct hashentry *gt;
    
    gt=h->tab;
    gs=h->size;
    h->tab=CALLOC(s, struct hashentry);
    h->nent=h->nval;
    h->size=s;
    for(i=0;i<gs;i++)
    {
        if (gt[i].left && gt[i].left!=DELETED_HASHENTRY)
            add_hash_value(h, gt[i].left, gt[i].right);
    }
    CFREE(gt, gs, struct hashentry);
}



/********************************************************************/
/* add a (key,value) pair to the hash table, rehashing if necessary */
/********************************************************************/
void set_hash(struct hashtable *h, char *key, char *value)
{
    int i,j;

    if (h->nent*5 > h->size*4)
        rehash(h, h->nval*3);
    j=-1;
    i=crc(key)%h->size;
    while (h->tab[i].left)
    {
        if (h->tab[i].left==DELETED_HASHENTRY)
            if (j==-1)
                j=i;
        if (!strcmp(h->tab[i].left, key))
        {
            SFREE(h->tab[i].right);
            h->tab[i].right=mystrdup(value);
            return;
        }
        if (!i)
            i=h->size;
        i--;
    }
    if (j!=-1)
        i=j;
    else
        h->nent++;
    h->tab[i].left = mystrdup(key);
    h->tab[i].right= mystrdup(value);
    h->nval++;
}


void set_hash_nostring(struct hashtable *h, char *key, char *value)
{
    int i,j;

    if (h->nent*5 > h->size*4)
        rehash(h, h->nval*3);
    j=-1;
    i=crc(key)%h->size;
    while (h->tab[i].left)
    {
        if (h->tab[i].left==DELETED_HASHENTRY)
            if (j==-1)
                j=i;
        if (!strcmp(h->tab[i].left, key))
        {
            h->tab[i].right=value;
            return;
        }
        if (!i)
            i=h->size;
        i--;
    }
    if (j!=-1)
        i=j;
    else
        h->nent++;
    h->tab[i].left = mystrdup(key);
    h->tab[i].right= value;
    h->nval++;
}



/****************************************************/
/* get the value for a given key, or 0 if not found */
/****************************************************/
char* get_hash(struct hashtable *h, char *key)
{
    int i;

    i=crc(key)%h->size;
    while (h->tab[i].left)
    {
        if (h->tab[i].left!=DELETED_HASHENTRY&&(!strcmp(h->tab[i].left, key)))
        {
            return h->tab[i].right;
        }
        if (!i)
            i=h->size;
        i--;
    }
    return 0;
}



/****************************************************/
/* delete the key and its value from the hash table */
/****************************************************/
int delete_hash(struct hashtable *h, char *key)
{
    int i;

    i=crc(key)%h->size;
    while (h->tab[i].left)
    {
        if (h->tab[i].left!=DELETED_HASHENTRY&&(!strcmp(h->tab[i].left, key)))
        {
            SFREE(h->tab[i].left);
            SFREE(h->tab[i].right);
            h->tab[i].left=DELETED_HASHENTRY;
            h->nval--;
            if (h->nval*5<h->size)
                rehash(h, h->size/2);
            return 1;
        }
        if (!i)
            i=h->size;
        i--;
    }
    return 0;
}



/*****************************************************/
/* merge two sorted llists (without heads!) into one */
/*****************************************************/
static struct listnode* merge_lists(struct listnode* a, struct listnode* b)
{
    struct listnode* c=0, *c0;
    
    if (!a)
        return b;
    if (!b)
        return a;
    if (strcmp(a->left,b->left)<=0)
    {
        c0=c=a;
        a=a->next;
    }
    else
    {
        c0=c=b;
        b=b->next;
    }
    while (a && b)
        if (strcmp(a->left,b->left)<=0)
        {
            c->next=a;
            c=a;
            a=a->next;
        }
        else
        {
            c->next=b;
            c=b;
            b=b->next;
        }
    if (a)
        c->next=a;
    else
        c->next=b;
    return c0;
}

/**************************************************************************/
/* create a sorted llist containing all entries of the table matching pat */
/**************************************************************************/
/* Rationale: hash tables are by definition unsorted.  When listing or    */
/* deleting from a list, we should show the entries in a sorted order,    */
/* however screen output is slow anyways, so we can sort it on the fly.   */
/**************************************************************************/
struct listnode* hash2list(struct hashtable *h, char *pat)
{
#define NBITS (sizeof(void*)*8)
    struct listnode *p[NBITS];     /* polynomial sort, O(n*log(n)) */
    struct listnode *l;
    int i,j;

    for (j=0;j<NBITS;j++)
        p[j]=0;
    for (i=0;i<h->size;i++)
        if (h->tab[i].left && (h->tab[i].left!=DELETED_HASHENTRY)
            && match(pat, h->tab[i].left))
        {
            if (!(l=TALLOC(struct listnode)))
                syserr("couldn't malloc listhead");
            l->left = h->tab[i].left;
            l->right= h->tab[i].right;
            l->pr   = 0;
            l->next = 0;
            for(j=0; p[j]; j++)     /* if j>=NBITS, we have a bug anyway */
            {
                l=merge_lists(p[j], l);
                p[j]=0;
            }
            p[j]=l;
        }
    l=0;
    for (j=0; j<NBITS; j++)
        l=merge_lists(p[j], l);
    p[0]=init_list();
    p[0]->next=l;
    return p[0];
}


/**************************/
/* duplicate a hash table */
/**************************/
struct hashtable* copy_hash(struct hashtable *h)
{
    int i;
    struct hashtable *g=init_hash();
    CFREE(g->tab, g->size, struct hashentry);
    g->size=(h->nval>4) ? (h->nval*2) : 8;
    g->tab=CALLOC(g->size, struct hashentry);
    
    for(i=0; i<h->size; i++)
        if (h->tab[i].left && h->tab[i].left!=DELETED_HASHENTRY)
            set_hash(g, h->tab[i].left, h->tab[i].right);
    return g;
}
