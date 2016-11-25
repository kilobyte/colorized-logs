/*********************************************************************/
/* file: path.c - stuff for the path feature                         */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                    coded by peter unold 1992                      */
/*                  recoded by Jeremy C. Jack 1994                   */
/*********************************************************************/
/* the path is implemented as a fix-sized queue. It gets a bit messy */
/* here and there, but it should work....                            */
/*********************************************************************/
#include "tintin.h"
#include "protos/action.h"
#include "protos/alias.h"
#include "protos/globals.h"
#include "protos/hash.h"
#include "protos/llist.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/path.h"
#include "protos/variables.h"

static bool return_flag = true;


void mark_command(const char *arg, struct session *ses)
{
    if (ses==nullsession)
        return tintin_puts("#No session active => NO PATH!", ses);

    kill_list(ses->path);
    ses->path = init_list();
    ses->path_length = 0;
    ses->no_return = 0;
    if (ses->mesvar[MSG_PATH])
        tintin_puts("#Beginning of path marked.", ses);
}

void map_command(const char *arg, struct session *ses)
{
    if (ses==nullsession)
        return tintin_printf(ses, "#No session active => NO PATH!");

    char cmd[BUFFER_SIZE];
    get_arg_in_braces(arg, cmd, 1);
    prepare_actionalias(cmd, cmd, ses);
    check_insert_path(cmd, ses);
}

void savepath_command(const char *arg, struct session *ses)
{
    if (ses==nullsession)
        return tintin_printf(ses, "#No session active => NO PATH TO SAVE!");

    char alias[BUFFER_SIZE], result[BUFFER_SIZE], *r=result;
    get_arg_in_braces(arg, alias, 1);
    if (!*arg)
        return tintin_eprintf(ses, "#Syntax: savepath <alias>");

    struct listnode *ln = ses->path;

    if (!ses->path_length)
    {
        tintin_eprintf(ses, "#No path to save!");
        return;
    }

    r+=snprintf(r, BUFFER_SIZE, "%calias {%s} {", tintin_char, alias);
    while ((ln = ln->next))
    {
        int dirlen = strlen(ln->left);
        if (r-result + dirlen >= BUFFER_SIZE - 10)
        {
            tintin_eprintf(ses, "#Error - buffer too small to contain alias");
            break;
        }
        else
            r += sprintf(r, "%s%s", ln->left, ln->next?";":"");
    }
    *r++='}', *r=0;
    parse_input(result, true, ses);
}


void path2var(char *var, struct session *ses)
{
    if (ses==nullsession)
    {
        tintin_printf(ses, "#No session active => NO PATH!");
        *var=0;
        return;
    }

    char *r;
    struct listnode *ln = ses->path;
    int dirlen, len = 0;

    if (!ses->path_length)
    {
        *var=0;
        return;
    }

    len = 0;
    r=var;

    while ((ln = ln->next))
    {
        dirlen = strlen(ln->left);
        if (dirlen + len < BUFFER_SIZE - 10)
        {
            r+=sprintf(r, isatom(ln->left)? "%s" : "{%s}" , ln->left);
            len += dirlen + 1;
            if (ln->next)
                *r++=' ';
        }
        else
        {
            tintin_eprintf(ses, "#Error - buffer too small to contain alias");
            *r++=0;
            break;
        }
    }
}


void path_command(const char *arg, struct session *ses)
{
    if (ses==nullsession)
        return tintin_puts("#No session active => NO PATH!", ses);

    struct listnode *ln = ses->path;
    char mypath[BUFFER_SIZE];

    char *r=mypath+sprintf(mypath, "#Path:  ");
    while ((ln = ln->next))
    {
        int dirlen = strlen(ln->left);
        if (r-mypath + dirlen > (COLS?COLS:BUFFER_SIZE)-10)
        {
            r[-1]=0;
            tintin_puts(mypath, ses);
            r=mypath+sprintf(mypath, "#Path:  ");
        }
        r+=sprintf(r, "%s;", ln->left);
    }
    r[-1]=0;
    tintin_puts(mypath, ses);
}

void return_command(const char *arg, struct session *ses)
{
    if (ses==nullsession)
        return tintin_puts("#No session active => NO PATH!", ses);

    int n;
    char *err, how[BUFFER_SIZE];

    get_arg_in_braces(arg, how, 1);

    if (ses->no_return==MAX_PATH_LENGTH)
    {
        tintin_puts1("#Don't know how to return from here!", ses);
        return;
    }
    if (!ses->path_length)
    {
        tintin_eprintf(ses, "#No place to return from!");
        return;
    }

    if (!*how)
        n=1;
    else if (!strcmp(how, "all") || !strcmp(how, "ALL"))
        n=ses->path_length;
    else
    {
        n=strtol(how, &err, 10);
        if (*err || n<0)
        {
            tintin_eprintf(ses, "#return [<num>|all], got {%s}", how);
            return;
        }
        if (!n)     /* silently ignore "#return 0" */
            return;
    }
    if (n>ses->path_length)
        n=ses->path_length;

    while (n--)
    {
        struct listnode *ln = ses->path;
        char command[BUFFER_SIZE];

        if (ses->no_return==MAX_PATH_LENGTH)
            break;
        ses->path_length--;
        if (ses->no_return)
            ses->no_return++;
        while (ln->next)
            (ln = ln->next);
        strcpy(command, ln->right);
        return_flag = false; /* temporarily turn off path tracking */
        parse_input(command, false, ses);
        return_flag = true;  /* restore path tracking */
        deletenode_list(ses->path, ln);
    }
}

void unmap_command(const char *arg, struct session *ses)
{
    if (ses==nullsession)
        return tintin_puts("#No session active => NO PATH!", ses);

    if (!ses->path_length)
        return tintin_puts("#No move to forget!", ses);

    struct listnode *ln = ses->path;

    ses->path_length--;
    while (ln->next)
        (ln = ln->next);
    deletenode_list(ses->path, ln);
    if (ses->mesvar[MSG_PATH])
        tintin_puts("#Ok.  Forgot that move.", ses);
}

void check_insert_path(const char *command, struct session *ses)
{
    char *ret;

    if (!return_flag)
        return;

    if (!(ret=get_hash(ses->pathdirs, command)))
        return;
    if (ses->path_length != MAX_PATH_LENGTH)
        ses->path_length++;
    else if (ses->path_length)
        deletenode_list(ses->path, ses->path->next);
    addnode_list(ses->path, command, ret, 0);
    if (ses->no_return)
        ses->no_return--;
}

void pathdir_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);

    if (*left && *right)
    {
        set_hash(ses->pathdirs, left, right);
        if (ses->mesvar[MSG_PATH])
            tintin_printf(ses, "#Ok.  {%s} is marked in #path. {%s} is it's #return.",
                    left, right);
        pdnum++;
        return;
    }
    show_hashlist(ses, ses->pathdirs, left,
        "#These PATHDIRS have been defined:",
        "#That PATHDIR (%s) is not defined.");
}

void unpathdir_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];

    arg = get_arg_in_braces(arg, left, 1);
    prepare_actionalias(left, left, ses);
    delete_hashlist(ses, ses->pathdirs, left,
        ses->mesvar[MSG_PATH]? "#Ok. $%s is no longer recognized as a pathdir." : 0,
        ses->mesvar[MSG_PATH]? "#THAT PATHDIR (%s) IS NOT DEFINED." : 0);
}
