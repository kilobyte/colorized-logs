/*********************************************************************/
/* file: history.c - functions for the history stuff                 */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/globals.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/utils.h"

static void insert_history(const char *buffer);


/************************/
/* the #history command */
/************************/
void history_command(const char *arg, struct session *ses)
{
    for (int i = HISTORY_SIZE - 1; i >= 0; i--)
        if (history[i])
            tintin_printf(ses, "%2d %s ", i, history[i]);
}


void do_history(char *buffer, struct session *ses)
{
    char temp[BUFFER_SIZE];
    const char *cptr;

    if (!ses->verbatim && *(cptr=space_out(buffer)) && *cptr=='!')
    {
        if (*(cptr + 1) == '!' && history[0])
        {
            strcpy(temp, cptr+2);
            snprintf(buffer, BUFFER_SIZE, "%s%s", history[0], temp);
        }
        else if (isadigit(*(cptr + 1)))
        {
            int i = atoi(cptr + 1);

            if (i >= 0 && i < HISTORY_SIZE && history[i])
            {
                strcpy(temp, cptr+2);
                snprintf(buffer, BUFFER_SIZE, "%s%s", history[i], temp);
            }
        }
        else
            for (int i = 0; i < HISTORY_SIZE && history[i]; i++)
                if (is_abrev(cptr + 1, history[i]))
                {
                    strcpy(buffer, history[i]);
                    break;
                }
    }
    insert_history(buffer);
}

/***********************************************/
/* insert buffer into a session`s history list */
/***********************************************/
static void insert_history(const char *buffer)
{
    if (history[HISTORY_SIZE - 1])
        SFREE(history[HISTORY_SIZE - 1]);

    for (int i = HISTORY_SIZE - 1; i > 0; i--)
        history[i] = history[i - 1];

    history[0] = mystrdup(buffer);
}
