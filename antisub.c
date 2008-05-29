/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: antisub.c - functions related to the substitute command     */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/action.h"
#include "protos/llist.h"
#include "protos/main.h"
#include "protos/parse.h"

extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */
extern int antisubnum;

/*******************************/
/* the #antisubstitute command */
/*******************************/
void antisubstitute_command(char *arg, struct session *ses)
{
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
