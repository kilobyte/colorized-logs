/*********************************************************************/
/* file: substitute.c - functions related to the substitute command  */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/action.h"
#include "protos/llist.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/variables.h"

extern pvars_t *pvars;
extern int subnum;
extern char *match_start,*match_end;
extern char *_;

/***************************/
/* the #substitute command */
/***************************/
static void parse_sub(char *arg,int gag,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    struct listnode *mysubs, *ln;
    int flag=0;

    mysubs = ses->subs;
    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);

    if (!*left && !*right)
        strcpy(left, "*");
    if (!*right)
    {
        while ((mysubs = search_node_with_wild(mysubs, left)) != NULL)
            if (gag)
            {
                if (!strcmp(mysubs->right, EMPTY_LINE))
                {
                    if (!flag)
                        tintin_printf(ses,"#THESE GAGS HAVE BEEN DEFINED:");
                    tintin_printf(ses, "{%s~7~}", mysubs->left);
                    flag=1;
                }
            }
            else
            {
                if (!flag)
                    tintin_printf(ses,"#THESE SUBSTITUTES HAVE BEEN DEFINED:");
                flag=1;
                shownode_list(mysubs);
            }
        if (!flag && ses->mesvar[2])
        {
            if (strcmp(left,"*"))
                tintin_printf(ses, "#THAT %s IS NOT DEFINED.", gag? "GAG":"SUBSTITUTE");
            else
                tintin_printf(ses, "#NO %sS HAVE BEEN DEFINED.", gag? "GAG":"SUBSTITUTE");
        }
        prompt(ses);
    }
    else
    {
        if ((ln = searchnode_list(mysubs, left)) != NULL)
            deletenode_list(mysubs, ln);
        insertnode_list(mysubs, left, right, 0, ALPHA);
        subnum++;
        if (ses->mesvar[2])
        {
            if (strcmp(right, EMPTY_LINE))
                tintin_printf(ses, "#Ok. {%s} now replaces {%s}.", right, left);
            else
                tintin_printf(ses, "#Ok. {%s} is now gagged.", left);
        }
    }
}

void substitute_command(char *arg, struct session *ses)
{
    parse_sub(arg, 0, ses);
}

void gag_command(char *arg, struct session *ses)
{
    char temp[BUFFER_SIZE];

    if (!*arg)
    {
        parse_sub("", 1, ses);
        return;
    }
    get_arg_in_braces(arg, temp+1, 1);
    temp[0]='{';
    strcat(temp, "} {"EMPTY_LINE"}");
    parse_sub(temp, 1, ses);
}

/*****************************/
/* the #unsubstitute command */
/*****************************/
static void unsub(char *arg, int gag, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode *mysubs, *ln, *temp;
    int flag;

    flag = FALSE;
    mysubs = ses->subs;
    temp = mysubs;
    arg = get_arg_in_braces(arg, left, 1);
    while ((ln = search_node_with_wild(temp, left)) != NULL)
    {
        if (gag && strcmp(ln->right,EMPTY_LINE))
        {
            temp=ln;
            continue;
        }
        if (ses->mesvar[2])
        {
            if (!strcmp(ln->right,EMPTY_LINE))
                tintin_printf(ses, "#Ok. {%s} is no longer gagged.", ln->left);
            else
                tintin_printf(ses, "#Ok. {%s} is no longer substituted.", ln->left);
        }
        deletenode_list(mysubs, ln);
        flag = TRUE;
        /*  temp=ln; */
    }
    if (!flag && ses->mesvar[2])
        tintin_printf(ses,"#THAT SUBSTITUTE (%s) IS NOT DEFINED.",left);
}

void unsubstitute_command(char *arg, struct session *ses)
{
    unsub(arg, 0, ses);
}

void ungag_command(char *arg, struct session *ses)
{
    unsub(arg, 1, ses);
}

#define APPEND(srch)    if (rlen+len > BUFFER_SIZE-1)           \
                            len=BUFFER_SIZE-1-rlen;             \
                        memcpy(result+rlen,srch,len);               \
                        rlen+=len;

void do_all_sub(char *line, struct session *ses)
{
    struct listnode *ln;
    pvars_t vars,*lastpvars;
    char result[BUFFER_SIZE],tmp1[BUFFER_SIZE],tmp2[BUFFER_SIZE],*l;
    int rlen,len;

    lastpvars=pvars;
    pvars=&vars;

    ln = ses->subs;

    while ((ln = ln->next))
        if (check_one_action(line, ln->left, &vars, 0, ses))
        {
            if (!strcmp(ln->right, EMPTY_LINE))
            {
                strcpy(line, EMPTY_LINE);
                return;
            };
            substitute_vars(ln->right, tmp1);
            substitute_myvars(tmp1, tmp2, ses);
            rlen=match_start-line;
            memcpy(result, line, rlen);
            len=strlen(tmp2);
            APPEND(tmp2);
            while (*match_end)
                if (check_one_action(l=match_end, ln->left, &vars, 1, ses))
                {
                    /* no gags possible here */
                    len=match_start-l;
                    APPEND(l);
                    substitute_vars(ln->right, tmp1);
                    substitute_myvars(tmp1, tmp2, ses);
                    len=strlen(tmp2);
                    APPEND(tmp2);
                }
                else
                {
                    len=strlen(l);
                    APPEND(l);
                    break;
                }
            memcpy(line, result, rlen);
            line[rlen]=0;
        }

    pvars=lastpvars;
}

void changeto_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (!_)
    {
        tintin_eprintf(ses, "#ERROR: #change is allowed only inside an action/promptaction");
        return;
    }

    get_arg_in_braces(arg, left, 1);
    substitute_vars(left, temp);
    substitute_myvars(temp, left, ses);
    strcpy(_, left);
}

void gagthis_command(char *arg, struct session *ses)
{
    if (!_)
    {
        tintin_eprintf(ses, "#ERROR: #gagthis is allowed only inside an action/promptaction");
        return;
    }

    strcpy(_, EMPTY_LINE);
}
