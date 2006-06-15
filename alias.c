/* $Id: alias.c,v 1.3 1998/10/11 18:36:33 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: alias.c - funtions related the the alias command            */
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

extern char *get_arg_in_braces();
extern void show_aliases();
extern void prompt();
extern struct listnode *search_node_with_wild(struct listnode *listhead, char *cptr);
extern struct listnode *searchnode_list(struct listnode *listhead, char *cptr);
extern void tintin_printf(struct session *ses,char *format,...);
extern void shownode_list(struct listnode *nptr);
extern void deletenode_list(struct listnode *listhead, struct listnode *nptr);
extern void insertnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext, int mode);
extern void show_list(struct listnode *listhead);


extern int alnum;
extern int mesvar[6];

/**********************/
/* the #alias command */
/**********************/
void alias_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    struct listnode *myaliases, *ln;

    myaliases = ses->aliases;
    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);

    if (!*left)
    {
        tintin_printf(ses,"#Defined aliases:");
        show_list(myaliases);
        prompt(ses);
    }
    else if (*left && !*right)
    {
        if ((ln = search_node_with_wild(myaliases, left)) != NULL)
        {
            while ((myaliases = search_node_with_wild(myaliases, left)) != NULL)
                shownode_list(myaliases);
            prompt(ses);
        }
        else if (mesvar[0])
            tintin_printf(ses, "#No match(es) found for {%s}.", left);
    }
    else
    {
        if ((ln = searchnode_list(myaliases, left)) != NULL)
            deletenode_list(myaliases, ln);
        insertnode_list(myaliases, left, right, 0, ALPHA);
        if (mesvar[0])
            tintin_printf(ses,"#Ok. {%s} aliases {%s}.", left, right);
        alnum++;
    }
}

/************************/
/* the #unalias command */
/************************/
void unalias_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode *myaliases, *ln, *temp;
    int flag;

    flag = FALSE;
    myaliases = ses->aliases;
    temp = myaliases;
    arg = get_arg_in_braces(arg, left, 1);
    while ((ln = search_node_with_wild(temp, left)) != NULL)
    {
        flag = TRUE;
        if (mesvar[0])
            tintin_printf(ses,"#Ok. {%s} is no longer an alias.", ln->left);
        /* temp=ln; */
        deletenode_list(myaliases, ln);
    }
    if (!flag && mesvar[0])
        tintin_printf(ses, "#No match(es) found for {%s}", left);
}
