/*********************************************************************/
/* file: llist.c - linked-list datastructure                         */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/glob.h"
#include "protos/hash.h"
#include "protos/llist.h"
#include "protos/print.h"
#include "protos/routes.h"
#include "protos/utils.h"


/***************************************/
/* init list - return: ptr to listhead */
/***************************************/
struct listnode* init_list(void)
{
    struct listnode *listhead;

    if ((listhead = TALLOC(struct listnode)) == NULL)
        syserr("couldn't alloc listhead");
    listhead->next = NULL;
    return listhead;
}

/***********************************************/
/* kill list - run through list and free nodes */
/***********************************************/
void kill_list(struct listnode *nptr)
{
    struct listnode *nexttodel;

    if (nptr == NULL)
        syserr("NULL PTR");
    nexttodel = nptr->next;
    LFREE(nptr);

    for (nptr = nexttodel; nptr; nptr = nexttodel)
    {
        nexttodel = nptr->next;
        SFREE(nptr->left);
        SFREE(nptr->right);
        SFREE(nptr->pr);
        LFREE(nptr);
    }
}


/***********************************************************/
/* zap only the list structure, without freeing the string */
/***********************************************************/
void zap_list(struct listnode *nptr)
{
    struct listnode *nexttodel;

    if (nptr == NULL)
        syserr("NULL PTR");
    nexttodel = nptr->next;
    LFREE(nptr);

    for (nptr = nexttodel; nptr; nptr = nexttodel)
    {
        nexttodel = nptr->next;
        LFREE(nptr);
    }
}


/********************************************************************
**   This function will clear all lists associated with a session  **
********************************************************************/
void kill_all(struct session *ses, bool no_reinit)
{
    if (!ses) // can't happen
        return;

    kill_hash(ses->aliases);
    kill_list(ses->actions);
    kill_list(ses->prompts);
    kill_hash(ses->myvars);
    kill_list(ses->highs);
    kill_list(ses->subs);
    kill_list(ses->antisubs);
    kill_list(ses->path);
    kill_hash(ses->pathdirs);
    kill_hash(ses->binds);
    kill_routes(ses);
    if (no_reinit)
        return;

    ses->aliases = init_hash();
    ses->actions = init_list();
    ses->prompts = init_list();
    ses->myvars = init_hash();
    ses->highs = init_list();
    ses->subs = init_list();
    ses->antisubs = init_list();
    ses->path = init_list();
    ses->binds = init_hash();
    ses->pathdirs = init_hash();
    tintin_printf(ses, "#Lists cleared.");
}

/***********************************************/
/* make a copy of a list - return: ptr to copy */
/***********************************************/
struct listnode* copy_list(struct listnode *sourcelist, int mode)
{
    struct listnode *resultlist;

    resultlist = init_list();
    while ((sourcelist = sourcelist->next))
        insertnode_list(resultlist, sourcelist->left, sourcelist->right,
                        sourcelist->pr, mode);

    return resultlist;
}

/******************************************************************/
/* compare priorities of a and b in a semi-lexicographical order: */
/* strings generally sort in ASCIIbetical order, however numbers  */
/* sort according to their numerical values.                      */
/******************************************************************/
static int prioritycmp(const char *a, const char *b)
{
    int res;

not_numeric:
    while (*a && *a==*b && !isadigit(*a))
    {
        a++;
        b++;
    }
    if (!a && !b)
        return 0;
    if (!isadigit(*a) || !isadigit(*b))
        return (*a<*b)? -1 : (*a>*b)? 1 : 0;
    while (*a=='0')
        a++;
    while (*b=='0')
        b++;
    res=0;
    while (isadigit(*a))
    {
        if (!isadigit(*b))
            return 1;
        if (*a!=*b && !res)
            res=(*a<*b)? -1 : 1;
        a++;
        b++;
    }
    if (isadigit(*b))
        return -1;
    if (res)
        return res;
    goto not_numeric;
}

/*****************************************************************/
/* create a node containing the ltext, rtext fields and stuff it */
/* into the list - in lexicographical order, or by numerical     */
/* priority (dependent on mode) - Mods by Joann Ellsworth 2/2/94 */
/*****************************************************************/
void insertnode_list(struct listnode *listhead, const char *ltext, const char *rtext, const char *prtext, llist_mode_t mode)
{
    struct listnode *nptr, *nptrlast, *newnode;
    int lo, ln;

    if ((newnode = (TALLOC(struct listnode))) == NULL)
        syserr("couldn't malloc listhead");
    newnode->left = (char *)MALLOC(strlen(ltext) + 1);
    newnode->right = (char *)MALLOC(strlen(rtext) + 1);
    newnode->pr = prtext? (char *)MALLOC(strlen(prtext) + 1) : 0;
    strcpy(newnode->left, ltext);
    strcpy(newnode->right, rtext);
    if (prtext)
        strcpy(newnode->pr, prtext);

    nptr = listhead;
    switch (mode)
    {
    case PRIORITY:
        while ((nptrlast = nptr) && (nptr = nptr->next))
        {
            if (prioritycmp(prtext, nptr->pr) < 0)
            {
                newnode->next = nptr;
                nptrlast->next = newnode;
                return;
            }
            else if (prioritycmp(prtext, nptr->pr) == 0)
            {
                while ((nptrlast) && (nptr) &&
                        (prioritycmp(prtext, nptr->pr) == 0))
                {
                    if (prioritycmp(ltext, nptr->left) <= 0)
                    {
                        newnode->next = nptr;
                        nptrlast->next = newnode;
                        return;
                    }
                    nptrlast = nptr;
                    nptr = nptr->next;
                }
                nptrlast->next = newnode;
                newnode->next = nptr;
                return;
            }
        }
        nptrlast->next = newnode;
        newnode->next = NULL;
        return;
        break;

    case LENGTH:
        ln=strlen(ltext);
        while ((nptrlast = nptr) && (nptr = nptr->next))
        {
            lo=strlen(nptr->left);
            if (ln<lo)
            {
                newnode->next = nptr;
                nptrlast->next = newnode;
                return;
            }
            else if (ln==lo)
            {
                while ((nptrlast) && (nptr) &&
                        (ln==lo))
                {
                    if (prioritycmp(ltext, nptr->left) <= 0)
                    {
                        newnode->next = nptr;
                        nptrlast->next = newnode;
                        return;
                    }
                    nptrlast = nptr;
                    nptr = nptr->next;
                }
                nptrlast->next = newnode;
                newnode->next = nptr;
                return;
            }
        }
        nptrlast->next = newnode;
        newnode->next = NULL;
        return;
        break;

    case ALPHA:
        while ((nptrlast = nptr) && (nptr = nptr->next))
        {
            if (strcmp(ltext, nptr->left) <= 0)
            {
                newnode->next = nptr;
                nptrlast->next = newnode;
                return;
            }
        }
        nptrlast->next = newnode;
        newnode->next = NULL;
        return;
        break;
    }
}

/*****************************/
/* delete a node from a list */
/*****************************/
void deletenode_list(struct listnode *listhead, struct listnode *nptr)
{
    struct listnode *lastnode = listhead;

    while ((listhead = listhead->next))
    {
        if (listhead == nptr)
        {
            lastnode->next = listhead->next;
            SFREE(listhead->left);
            SFREE(listhead->right);
            SFREE(listhead->pr);
            LFREE(listhead);
            return;
        }
        lastnode = listhead;
    }
    return;
}

/********************************************************/
/* search for a node containing the ltext in left-field */
/* return: ptr to node on succes / NULL on failure      */
/********************************************************/
struct listnode* searchnode_list(struct listnode *listhead, char *cptr)
{
    int i;

    while ((listhead = listhead->next))
    {
        if ((i = strcmp(listhead->left, cptr)) == 0)
            return listhead;
    }
    return NULL;
}

/*************************************/
/* show contents of a node on screen */
/*************************************/
void shownode_list(struct listnode *nptr)
{
    tintin_printf(0, "~7~{%s~7~}={%s~7~}", nptr->left, nptr->right);
}

void shownode_list_action(struct listnode *nptr)
{
    tintin_printf(0, "~7~{%s~7~}={%s~7~} @ {%s}", nptr->left, nptr->right, nptr->pr);
}

/*************************************/
/* list contents of a list on screen */
/*************************************/
void show_list(struct listnode *listhead)
{
    while ((listhead = listhead->next))
        shownode_list(listhead);
}

void show_list_action(struct listnode *listhead)
{
    while ((listhead = listhead->next))
        if (strcmp(listhead->left, K_ACTION_MAGIC))
            shownode_list_action(listhead);
}

struct listnode* search_node_with_wild(struct listnode *listhead, char *cptr)
{
    while ((listhead = listhead->next))
    {
        if (match(cptr, listhead->left))
            return listhead;
    }
    return NULL;
}


/*********************************************************************/
/* create a node containing the ltext, rtext fields and place at the */
/* end of a list - as insertnode_list(), but not alphabetical        */
/*********************************************************************/
void addnode_list(struct listnode *listhead, const char *ltext, const char *rtext, const char *prtext)
{
    struct listnode *newnode;

    if ((newnode = TALLOC(struct listnode)) == NULL)
        syserr("couldn't malloc listhead");
    newnode->left = mystrdup(ltext);
    if (rtext)
        newnode->right = mystrdup(rtext);
    else
        newnode->right = 0;
    if (prtext)
        newnode->pr = mystrdup(prtext);
    else
        newnode->pr = 0;
    newnode->next = NULL;
    while (listhead->next != NULL)
        (listhead = listhead->next);
    listhead->next = newnode;
}

int count_list(struct listnode *listhead)
{
    int count = 0;
    struct listnode *nptr;

    nptr = listhead;
    while ((nptr = nptr->next))
        ++count;
    return count;
}
