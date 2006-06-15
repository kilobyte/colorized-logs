/* $Id: history.c,v 1.2 1998/10/11 18:36:35 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: history.c - functions for the history stuff                 */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include "tintin.h"
#include <stdlib.h>

void insert_history(char *buffer, struct session *ses);

extern char *space_out(char* s);
extern char *mystrdup(char *s);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern int is_abrev(char *s1, char *s2);
extern void check_all_promptactions(char *line, struct session *ses);
extern void prompt(struct session *ses);
extern void tintin_puts(char *cptr, struct session *ses);
extern void tintin_printf(struct session *ses,char *format,...);

/************************/
/* the #history command */
/************************/
void history_command(struct session *ses)
{
    if (ses)
    {
        int i;

        for (i = HISTORY_SIZE - 1; i >= 0; i--)
            if (ses->history[i])
                tintin_printf(ses, "%2d %s ", i, ses->history[i]);
    }
    else
        tintin_printf(ses,"#NO SESSION ACTIVE => NO HISTORY DUMMY!");
    prompt(NULL);
}



void do_history(char *buffer, struct session *ses)
{
    char result[BUFFER_SIZE], *cptr;

    cptr = space_out(buffer);

    if (*cptr)
    {

        if (*cptr == '!')
        {
            if (*(cptr + 1) == '!')
            {
                if (ses->history[0])
                {
                    strcpy(result, ses->history[0]);
                    strcat(result, cptr + 2);
                    strcpy(buffer, result);
                }
            }
            else if (isdigit(*(cptr + 1)))
            {
                int i = atoi(cptr + 1);

                if (i >= 0 && i < HISTORY_SIZE && ses->history[i])
                {
                    strcpy(result, ses->history[i]);
                    strcat(result, cptr + 2);
                    strcpy(buffer, result);
                }
            }
            else
            {
                int i;

                for (i = 0; i < HISTORY_SIZE && ses->history[i]; i++)
                    if (is_abrev(cptr + 1, ses->history[i]))
                    {
                        strcpy(buffer, ses->history[i]);
                        break;
                    }
            }

        }
    }
    insert_history(buffer, ses);
}

/***********************************************/
/* insert buffer into a session`s history list */
/***********************************************/
void insert_history(char *buffer, struct session *ses)
{
    int i;

    /* CHANGED to fix an annoying memory leak, these were never getting freed */
    if (ses->history[HISTORY_SIZE - 1])
        free(ses->history[HISTORY_SIZE - 1]);

    for (i = HISTORY_SIZE - 1; i > 0; i--)
        ses->history[i] = ses->history[i - 1];

    ses->history[0] = mystrdup(buffer);
}

/************************************************************/
/* do all the parse stuff for !XXXX history commands        */
/************************************************************/
struct session *parse_history_command(char *command, char *arg, struct session *ses)
{
    if (ses)
    {

        if ((*(command + 1) == '!' || !*(command + 1)) && ses->history[0])
            return parse_input(ses->history[0],1,ses); /* we're already not in verbatim */

        else if (isdigit(*(command + 1)))
        {
            int i = atoi(command + 1);

            if (i >= 0 && i < HISTORY_SIZE && ses->history[i])
            {
                return parse_input(ses->history[i],1,ses);
            }
        }
    }
    tintin_puts("#I DON'T REMEMBER THAT COMMAND.", ses);

    return ses;
}
