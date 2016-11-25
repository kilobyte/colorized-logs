/*********************************************************************/
/* file: antisub.c - functions related to the substitute command     */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/action.h"
#include "protos/globals.h"
#include "protos/llist.h"
#include "protos/print.h"
#include "protos/parse.h"


/*******************************/
/* the #antisubstitute command */
/*******************************/
void antisubstitute_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode *myantisubs, *ln;

    myantisubs = ses->antisubs;
    arg = get_arg_in_braces(arg, left, 1);

    if (!*left)
    {
        tintin_puts("#THESE ANTISUBSTITUTES HAS BEEN DEFINED:", ses);
        show_list(myantisubs);
    }
    else
    {
        if ((ln = searchnode_list(myantisubs, left)) != NULL)
            deletenode_list(myantisubs, ln);
        insertnode_list(myantisubs, left, left, 0, ALPHA);
        antisubnum++;
        if (ses->mesvar[MSG_SUBSTITUTE])
            tintin_printf(ses, "Ok. Any line with {%s} will not be subbed.", left);
    }
}


void unantisubstitute_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode *myantisubs, *ln, *temp;
    bool flag = false;

    myantisubs = ses->antisubs;
    temp = myantisubs;
    arg = get_arg_in_braces(arg, left, 1);
    while ((ln = search_node_with_wild(temp, left)) != NULL)
    {
        if (ses->mesvar[MSG_SUBSTITUTE])
            tintin_printf(ses, "#Ok. Lines with {%s} will now be subbed.", ln->left);
        deletenode_list(myantisubs, ln);
        flag = true;
    }
    if (!flag && ses->mesvar[MSG_SUBSTITUTE])
        tintin_printf(ses, "#THAT ANTISUBSTITUTE (%s) IS NOT DEFINED.", left);
}


bool do_one_antisub(const char *line, struct session *ses)
{
    struct listnode *ln;
    pvars_t vars;

    ln = ses->antisubs;

    while ((ln = ln->next))
        if (check_one_action(line, ln->left, &vars, false, ses))
            return true;
    return false;
}
