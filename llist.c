/* $Id: llist.c,v 2.3 1998/11/25 17:14:00 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: llist.c - linked-list datastructure                         */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#include "tintin.h"
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void insertnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext, int mode);
int match(char *regex, char *string);
extern void kill_routes(struct session *ses);
extern void check_all_promptactions(char *line, struct session *ses);
extern void prompt(struct session *ses);
extern void tintin_printf(struct session *ses,char *format,...);
extern void user_done(void);


/***************************************/
/* init list - return: ptr to listhead */
/***************************************/
struct listnode *init_list(void)
{
    struct listnode *listhead;

    if ((listhead = (struct listnode *)(malloc(sizeof(struct listnode)))) == NULL)
    {
        fprintf(stderr, "couldn't alloc listhead\n");
        user_done();
        exit(1);
    }
    listhead->next = NULL;
    return (listhead);
}

/************************************************/
/* kill list - run throught list and free nodes */
/************************************************/
void kill_list(struct listnode *nptr)
{
    struct listnode *nexttodel;

    if (nptr == NULL)
    {
        fprintf(stderr, "NULL PTR\n");
        return;
    }
    nexttodel = nptr->next;
    free(nptr);

    for (nptr = nexttodel; nptr; nptr = nexttodel)
    {
        nexttodel = nptr->next;
        free(nptr->left);
        free(nptr->right);
        free(nptr->pr);
        free(nptr);
    }
}
/********************************************************************
**   This function will clear all lists associated with a session  **
********************************************************************/
void kill_all(struct session *ses, int mode)
{
    switch (mode)
    {
    case CLEAN:
        if (ses != NULL)
        {
            kill_list(ses->aliases);
            ses->aliases = init_list();
            kill_list(ses->actions);
            ses->actions = init_list();
            kill_list(ses->prompts);
            ses->prompts = init_list();
            kill_list(ses->myvars);
            ses->myvars = init_list();
            kill_list(ses->highs);
            ses->highs = init_list();
            kill_list(ses->subs);
            ses->subs = init_list();
            kill_list(ses->antisubs);
            ses->antisubs = init_list();
            kill_list(ses->path);
            ses->path = init_list();
            kill_list(ses->pathdirs);
            ses->pathdirs = init_list();
            kill_routes(ses);
            tintin_printf(ses,"#Lists cleared.");
            prompt(NULL);
        }
        else
        {
            tintin_printf(0,"#Can't clean the common lists (yet).");
            prompt(NULL);
        }
        break;

    case END:
        if (ses != NULL)
        {
            kill_list(ses->aliases);
            kill_list(ses->actions);
            kill_list(ses->prompts);
            kill_list(ses->myvars);
            kill_list(ses->highs);
            kill_list(ses->subs);
            kill_list(ses->antisubs);
            kill_list(ses->path);
            kill_list(ses->pathdirs);
            kill_routes(ses);
        }				/* If */
        break;
    }				/* Switch */

}
/***********************************************/
/* make a copy of a list - return: ptr to copy */
/***********************************************/
struct listnode *copy_list(struct listnode *sourcelist,int mode)
{
    struct listnode *resultlist;

    resultlist = init_list();
    while ((sourcelist = sourcelist->next))
        insertnode_list(resultlist, sourcelist->left, sourcelist->right,
                        sourcelist->pr, mode);

    return (resultlist);
}

/*****************************************************************/
/* create a node containing the ltext, rtext fields and stuff it */
/* into the list - in lexicographical order, or by numerical     */
/* priority (dependent on mode) - Mods by Joann Ellsworth 2/2/94 */
/*****************************************************************/
void insertnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext, int mode)
{
    struct listnode *nptr, *nptrlast, *newnode;

    if ((newnode = (struct listnode *)(malloc(sizeof(struct listnode)))) == NULL)
    {
        user_done();
        fprintf(stderr, "couldn't malloc listhead");
        exit(1);
    }
    newnode->left = (char *)malloc(strlen(ltext) + 1);
    newnode->right = (char *)malloc(strlen(rtext) + 1);
    newnode->pr = prtext? (char *)malloc(strlen(prtext) + 1) : 0;
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
            if (strcmp(prtext, nptr->pr) < 0)
            {
                newnode->next = nptr;
                nptrlast->next = newnode;
                return;
            }
            else if (strcmp(prtext, nptr->pr) == 0)
            {
                while ((nptrlast) && (nptr) &&
                        (strcmp(prtext, nptr->pr) == 0))
                {
                    if (strcmp(ltext, nptr->left) <= 0)
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
    }				/*  Switch  */
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
            free(listhead->left);
            free(listhead->right);
            free(listhead->pr);
            free(listhead);
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
struct listnode *searchnode_list(struct listnode *listhead, char *cptr)
{
    int i;

    while ((listhead = listhead->next))
    {
        if ((i = strcmp(listhead->left, cptr)) == 0)
            return listhead;
        /* CHANGED to fix bug when list isn't alphabetically sorted
           else if(i>0)
           return NULL;
         */
    }
    return NULL;
}
/********************************************************/
/* search for a node that has cptr as a beginning       */
/* return: ptr to node on succes / NULL on failure      */
/* Mods made by Joann Ellsworth - 2/2/94                */
/********************************************************/
struct listnode *searchnode_list_begin(struct listnode *listhead, char *cptr, int mode)
{
    int i;

    switch (mode)
    {
    case PRIORITY:
        while ((listhead = listhead->next))
        {
            if ((i = strncmp(listhead->left, cptr, strlen(cptr))) == 0 &&
                    (*(listhead->left + strlen(cptr)) == ' ' ||
                     *(listhead->left + strlen(cptr)) == '\0'))
                return listhead;
        }
        return NULL;
        break;

    case ALPHA:
        while ((listhead = listhead->next))
        {
            if ((i = strncmp(listhead->left, cptr, strlen(cptr))) == 0 &&
                    (*(listhead->left + strlen(cptr)) == ' ' ||
                     *(listhead->left + strlen(cptr)) == '\0'))
                return listhead;
            else if (i > 0)
                return NULL;
        }
        return NULL;
        break;
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
        shownode_list_action(listhead);
}

struct listnode *search_node_with_wild(struct listnode *listhead, char *cptr)
{
    /* int i; */
    while ((listhead = listhead->next))
    {
        /* CHANGED to fix silly globbing behavior
           if(check_one_node(listhead->left, cptr))
         */
        if (match(cptr, listhead->left))
            return listhead;
    }
    return NULL;
}

int check_one_node(char *text, char *action)
{
    char *temp, temp2[BUFFER_SIZE], *tptr;

    while (*text && *action)
    {
        if (*action == '*')
        {
            action++;
            temp = action;
            tptr = temp2;
            while (*temp && *temp != '*')
                *tptr++ = *temp++;
            *tptr = '\0';
            if (strlen(temp2) == 0)
                return TRUE;
            while (strncmp(temp2, text, strlen(temp2)) != 0 && *text)
                text++;
        }
        else
        {
            temp = action;
            tptr = temp2;
            while (*temp && *temp != '*')
                *tptr++ = *temp++;
            *tptr = '\0';
            if (strncmp(temp2, text, strlen(temp2)) != 0)
                return FALSE;
            else
            {
                text += strlen(temp2);
                action += strlen(temp2);
            }
        }
    }
    if (*text)
        return FALSE;
    else if ((*action == '*' && !*(action + 1)) || !*action)
        return TRUE;
    return FALSE;
}

/*********************************************************************/
/* create a node containing the ltext, rtext fields and place at the */
/* end of a list - as insertnode_list(), but not alphabetical        */
/*********************************************************************/
void addnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext)
{
    struct listnode *newnode;

    if ((newnode = (struct listnode *)malloc(sizeof(struct listnode))) == NULL)
    {
        user_done();
        fprintf(stderr, "couldn't malloc listhead");
        exit(1);
    }
    newnode->left = (char *)malloc(strlen(ltext) + 1);
    newnode->right = (char *)malloc(strlen(rtext) + 1);
    if (prtext)
    {
        newnode->pr = (char *)malloc(strlen(prtext) + 1);
        strcpy(newnode->pr, prtext);
    }
    else
        newnode->pr=0;
    newnode->next = NULL;
    strcpy(newnode->left, ltext);
    strcpy(newnode->right, rtext);
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
    return (count);
}
