/* $Id: antisub.c,v 1.3 1998/10/11 18:36:34 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: antisub.c - functions related to the substitute command     */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include "tintin.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern struct listnode *searchnode_list(struct listnode *listhead, char *cptr);
extern struct listnode *search_node_with_wild(struct listnode *listhead, char *cptr);
extern void tintin_puts(char *cptr, struct session *ses);
extern void prompt(struct session *ses);
extern void deletenode_list(struct listnode *listhead, struct listnode *nptr);
extern void insertnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext, int mode);
extern void tintin_printf(struct session *ses,char *format,...);
extern int check_one_action(char *line, char *action, pvars_t *vars, int inside, struct session *ses);
extern void show_list(struct listnode *listhead);

extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */
extern int antisubnum;

/*******************************/
/* the #antisubstitute command */
/*******************************/
void antisubstitute_command(char *arg, struct session *ses)
{
    /* char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE]; */
    char left[BUFFER_SIZE];
    struct listnode *myantisubs, *ln;

    myantisubs = ses->antisubs;
    arg = get_arg_in_braces(arg, left, 1);

    if (!*left)
    {
        tintin_puts("#THESE ANTISUBSTITUTES HAS BEEN DEFINED:", ses);
        show_list(myantisubs);
        prompt(ses);
    }
    else
    {
        if ((ln = searchnode_list(myantisubs, left)) != NULL)
            deletenode_list(myantisubs, ln);
        insertnode_list(myantisubs, left, left, 0, ALPHA);
        antisubnum++;
        if (ses->mesvar[2])
            tintin_printf(ses, "Ok. Any line with {%s} will not be subbed.", left);
    }
}


void unantisubstitute_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode *myantisubs, *ln, *temp;
    int flag;

    flag = FALSE;
    myantisubs = ses->antisubs;
    temp = myantisubs;
    arg = get_arg_in_braces(arg, left, 1);
    while ((ln = search_node_with_wild(temp, left)) != NULL)
    {
        if (ses->mesvar[2])
            tintin_printf(ses,"#Ok. Lines with {%s} will now be subbed.", ln->left);
        deletenode_list(myantisubs, ln);
        flag = TRUE;
        /* temp=ln; */
    }
    if (!flag && ses->mesvar[2])
        tintin_printf(ses,"#THAT ANTISUBSTITUTE (%s) IS NOT DEFINED.", left);
}




int do_one_antisub(char *line, struct session *ses)
{
    struct listnode *ln;
    pvars_t vars;

    ln = ses->antisubs;

    while ((ln = ln->next))
        if (check_one_action(line, ln->left, &vars, 0, ses))
            return TRUE;
    return FALSE;
}
