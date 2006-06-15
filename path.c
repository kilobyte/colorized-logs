/* $Id: path.c,v 2.2 1998/10/11 18:36:36 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
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
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include "tintin.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void check_insert_path(char *command, struct session *ses, int force);
int return_flag = TRUE;

extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern void prompt(struct session *ses);
extern void kill_list(struct listnode *nptr);
extern struct listnode *search_node_with_wild(struct listnode *listhead, char *cptr);
extern struct listnode *searchnode_list(struct listnode *listhead, char *cptr);
extern struct listnode *init_list(void);
extern struct listnode *deletenode_list(struct listnode *listhead, struct listnode *nptr);
extern struct listnode *addnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext);
extern struct session *nullsession;
extern void insertnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext, int mode);
extern int isatom(char *arg);
extern int isatom_inline(char *arg,struct session *ses);
extern void isatom_command(char *line,struct session *ses);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern void show_list(struct listnode *listhead);
extern void shownode_list(struct listnode *nptr);
extern void tintin_puts(char *cptr, struct session *ses);
extern void tintin_puts1(char *cptr, struct session *ses);
extern void tintin_printf(struct session *ses,char *format,...);


extern char tintin_char;
extern int pdnum;
extern int mesvar[];
extern int COLS,LINES;

void mark_command(struct session *ses)
{
    if (ses!=nullsession)
    {
        kill_list(ses->path);
        ses->path = init_list();
        ses->path_length = 0;
        ses->no_return = 0;
        if (mesvar[10])
            tintin_puts("#Beginning of path marked.", ses);
    }
    else
        tintin_puts("#No session active => NO PATH!", ses);
}

void map_command(char *arg,struct session *ses)
{
    if (ses!=nullsession)
    {
        get_arg_in_braces(arg, arg, 1);
        prepare_actionalias(arg, arg, ses);
        check_insert_path(arg, ses, 0);
    }
    else
        tintin_printf(ses,"No session active => NO PATH!");
}

void path_command(char *arg,struct session *ses)
{
    if (ses!=nullsession)
    {
        get_arg_in_braces(arg, arg, 1);
        prepare_actionalias(arg, arg, ses);
        check_insert_path(arg, ses, 1);
    }
    else
        tintin_printf(ses,"No session active => NO PATH!");
}

void savepath_command(char *arg, struct session *ses)
{
    if (ses!=nullsession)
    {
        get_arg_in_braces(arg, arg, 1);
        if (*arg)
        {
            char result[BUFFER_SIZE];
            struct listnode *ln = ses->path;
            int dirlen, len = 0;

            if (!ses->path_length)
            {
                tintin_puts("#No path to save!",ses);
                return;
            }

            sprintf(result, "%calias {%s} {", tintin_char, arg);
            len = strlen(result);
            while ((ln = ln->next))
            {
                dirlen = strlen(ln->left);
                if (dirlen + len < BUFFER_SIZE - 10)
                {
                    strcat(result, ln->left);
                    len += dirlen + 1;
                    if (ln->next)
                        strcat(result, ";");
                }
                else
                {
                    tintin_puts("#Error - buffer too small to contain alias", ses);
                    break;
                }
            }
            strcat(result, "}");
            parse_input(result,1,ses);
        }
        else
            tintin_puts("#savepath <alias>", ses);
    }
    else
        tintin_printf(ses,"No session active => NO PATH TO SAVE!");
}


void path2var(char *var, struct session *ses)
{
    if (ses!=nullsession)
    {
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
                r+=sprintf(r, isatom(ln->left)? "%s" : "{%s}" ,ln->left);
                len += dirlen + 1;
                if (ln->next)
                    *r++=' ';
            }
            else
            {
                tintin_puts("#Error - buffer too small to contain alias", ses);
                *r++=0;
                break;
            }
        }
    }
    else
    {
        tintin_printf(ses,"No session active => NO PATH!");
        *var=0;
    }
}


void showpath_command(struct session *ses)
{
    if (ses!=nullsession)
    {
        int len = 0, dirlen;
        struct listnode *ln = ses->path;
        char mypath[BUFFER_SIZE];

        strcpy(mypath, "#Path:  ");
        while ((ln = ln->next))
        {
            dirlen = strlen(ln->left);
            if (dirlen + len > COLS-10)
            {
                if (len)
                    mypath[len+7]=0;
                tintin_puts(mypath, ses);
                strcpy(mypath, "#Path:  ");
                len = 0;
            }
            strcat(mypath, ln->left);
            strcat(mypath+dirlen, ";");
            len += dirlen + 1;
        }
        if (len)
            mypath[len+7]=0;
        tintin_puts(mypath, ses);
    }
    else
        tintin_puts("No session active => NO PATH!", ses);
}

void return_command(char *arg,struct session *ses)
{
    int n;
    char *err;
    
    get_arg_in_braces(arg, arg, 1);
    
    if (ses!=nullsession)
    {
        if (ses->no_return==MAX_PATH_LENGTH)
        {
            tintin_puts1("#Don't know how to return from here!", ses);
            return;
        }
        if (!ses->path_length)
        {
            tintin_puts("#No place to return from!", ses);
            return;
        }
        
        if (!*arg)
            n=1;
        else
        if (!strcmp(arg,"all") || !strcmp(arg,"ALL"))
            n=ses->path_length;
        else
        {
            n=strtol(arg,&err,10);
            if (*err || n<0)
            {
                tintin_printf(ses, "#return [<num>|all], got {%s}", arg);
                return;
            }
            if (!n)     /* silently ignore "#return 0" */
                return;
        };
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
            return_flag = FALSE;	/* temporarily turn off path tracking */
            parse_input(command,0,ses);
            return_flag = TRUE;	/* restore path tracking */
            deletenode_list(ses->path, ln);
        }
    }
    else
        tintin_puts("#No session active => NO PATH!", ses);
}

void unmap_command(struct session *ses)
{
    if (ses!=nullsession)
        if (ses->path_length)
        {
            struct listnode *ln = ses->path;

            ses->path_length--;
            while (ln->next)
                (ln = ln->next);
            deletenode_list(ses->path, ln);
            tintin_puts("#Ok.  Forgot that move.", ses);
        }
        else
            tintin_puts("#No move to forget!", ses);
    else
        tintin_puts("#No session active => NO PATH!", ses);
}

void check_insert_path(char *command, struct session *ses, int force)
{
    struct listnode *ln;
    char *ret;

    if (!return_flag)
        return;

    if ((ln = searchnode_list(ses->pathdirs, command)))
        ret=ln->right;
    else
    {
        if (!force)
            return;
        ret="-no return-";
        ses->no_return=MAX_PATH_LENGTH+1;
    }
    if (ses->path_length != MAX_PATH_LENGTH)
        ses->path_length++;
    else if (ses->path_length)
        deletenode_list(ses->path, ses->path->next);
    addnode_list(ses->path, command, ret, 0);
    if (ses->no_return)
        ses->no_return--;
}

void pathdir_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    struct listnode *mypathdirs, *ln;

    mypathdirs = ses->pathdirs;
    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);

    if (!*left)
    {
        tintin_printf(ses,"#These PATHDIRS have been defined:");
        show_list(mypathdirs);
        prompt(ses);
    }
    else if (*left && !*right)
    {
        if ((ln = search_node_with_wild(mypathdirs, left)) != NULL)
        {
            while ((mypathdirs = search_node_with_wild(mypathdirs, left)) != NULL)
                shownode_list(mypathdirs);
            prompt(ses);
        }
        else if (mesvar[10])
            tintin_printf(ses,"#That PATHDIR (%s) is not defined.",left);
    }
    else
    {
        if ((ln = searchnode_list(mypathdirs, left)) != NULL)
            deletenode_list(mypathdirs, ln);
        insertnode_list(mypathdirs, left, right, 0, ALPHA);
        if (mesvar[10])
            tintin_printf(ses, "#Ok.  {%s} is marked in #path. {%s} is it's #return.",
                    left, right);
        pdnum++;
    }
}
