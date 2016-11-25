/*********************************************************************/
/* file: alias.c - funtions related the the alias command            */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/glob.h"
#include "protos/globals.h"
#include "protos/hash.h"
#include "protos/llist.h"
#include "protos/print.h"
#include "protos/parse.h"


void show_hashlist(struct session *ses, struct hashtable *h, const char *pat, const char *msg_all, const char *msg_none)
{
    struct listnode *templist;

    if (!*pat)
    {
        tintin_printf(ses, msg_all);
        templist=hash2list(h, "*");
    }
    else
        templist=hash2list(h, pat);
    show_list(templist);
    if (*pat && !templist->next)
        tintin_printf(ses, msg_none, pat);
    zap_list(templist);
}

void delete_hashlist(struct session *ses, struct hashtable *h, const char *pat, const char *msg_ok, const char *msg_none)
{
    struct listnode *templist;

    if (is_literal(pat))
    {
        if (delete_hash(h, pat))
        {
            if (msg_ok)
                tintin_printf(ses, msg_ok, pat);
        }
        else
        {
            if (msg_none)
                tintin_printf(ses, msg_none, pat);
        }
        return;
    }
    templist=hash2list(h, pat);
    for (struct listnode *ln=templist->next; ln; ln=ln->next)
    {
        if (msg_ok)
            tintin_printf(ses, msg_ok, ln->left);
        delete_hash(h, ln->left);
    }
    if (msg_none && !templist->next)
        tintin_printf(ses, msg_none, pat);
    zap_list(templist);
}


/**********************/
/* the #alias command */
/**********************/
void alias_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE], *ch;

    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);

    if (*left && *right)
    {
        if ((ch=strchr(left, ' ')))
        {
            tintin_eprintf(ses, "#ERROR: aliases cannot contain spaces! Bad alias: {%s}", left);
            if (ch==left)
                return;
            *ch=0;
            tintin_printf(ses, "#Converted offending alias to {%s}.", left);
        }
        set_hash(ses->aliases, left, right);
        if (ses->mesvar[MSG_ALIAS])
            tintin_printf(ses, "#Ok. {%s} aliases {%s}.", left, right);
        alnum++;
        return;
    }
    show_hashlist(ses, ses->aliases, left,
        "#Defined aliases:",
        "#No match(es) found for {%s}.");
}

/************************/
/* the #unalias command */
/************************/
void unalias_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];

    arg = get_arg_in_braces(arg, left, 1);
    delete_hashlist(ses, ses->aliases, left,
        ses->mesvar[MSG_ALIAS]? "#Ok. {%s} is no longer an alias." : 0,
        ses->mesvar[MSG_ALIAS]? "#No match(es) found for {%s}" : 0);
}
