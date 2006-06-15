/* $Id: parse.c,v 2.8 1999/01/19 08:46:22 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: parse.c - some utility-functions                            */
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

#include <ctype.h>
#include <signal.h>
#include "tintin.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "commands.h"
typedef void (*t_command)(char*, struct session*);
typedef struct session *(*t_c_command)(char*, struct session*);

struct session *parse_tintin_command(char *command, char *arg,struct session *ses);
void do_speedwalk(char *cp, struct session *ses);
inline char *get_arg_with_spaces(char *s, char *arg);

extern int is_abrev(char *s1, char *s2);
inline int is_speedwalk_dirs(char *cp);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);
extern void tintin_printf(struct session *ses,char *format,...);
extern void tintin_eprintf(struct session *ses,char *format,...);
extern void write_line_mud(char *line, struct session *ses);
extern int do_goto(char *txt,struct session *ses);
extern void do_out_MUD_colors(char *line);
inline char *space_out(char *s);
inline char *get_arg_in_braces(char *s,char *arg,int flag);
void write_com_arg_mud(char *command, char *argument, int nsp, struct session *ses);
void prompt(struct session *ses);
extern char* get_hash(struct hashtable *h, char *key);
extern char* set_hash_nostring(struct hashtable *h, char *key, char *value);
extern struct hashtable *init_hash();
extern void end_command(char *arg, struct session *ses);
extern void unlink_command(char *arg, struct session *ses);
extern void check_insert_path(char *command, struct session *ses, int force);

extern struct session *sessionlist, *activesession, *nullsession;
extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */
extern char tintin_char, verbatim_char;
extern int term_echoing;
inline char *get_arg_stop_spaces(char *s, char *arg);
extern char *get_command(char *s, char *arg);
extern char *cryptkey;
extern char *get_arg_all(char *s, char *arg);
extern void tstphandler(int sig);
extern void debuglog(struct session *ses, const char *format, ...);
int in_alias=0;
extern int in_read;
extern int aborting;
struct hashtable *commands, *c_commands;

/**************************************************************************/
/* parse input, check for TINTIN commands and aliases and send to session */
/**************************************************************************/
struct session *parse_input(char *input,int override_verbatim,struct session *ses)
{
    char command[BUFFER_SIZE], arg[BUFFER_SIZE], result[BUFFER_SIZE], *al;
    int nspaces;

    if (ses->debuglogfile)
        debuglog(ses, "%s", input);
    if (!ses->server_echo && activesession == ses)
        term_echoing = 1;
    if (*input == '\0')
    {
        if (ses!=nullsession)
        {
            write_line_mud("", ses);
            if (ses->debuglogfile)
                debuglog(ses, "");
        }
        else
        {
            if (!in_read)
                write_com_arg_mud("", "", 0, ses);
        }
        return ses;
    }
    if ((*input==tintin_char) && is_abrev(input + 1, "verbatim"))
    {
        verbatim_command("",ses);
        if (ses->debuglogfile)
            debuglog(ses, "%s", input);
        return ses;
    }
    if (ses->verbatim && !override_verbatim && (ses!=nullsession))
    {
        write_line_mud(input, ses);
        if (ses->debuglogfile)
            debuglog(ses, "%s", input);
        return ses;
    }
    if (*input==verbatim_char && (ses!=nullsession))
    {
        input++;
        write_line_mud(input, ses);
        if (ses->debuglogfile)
            debuglog(ses, "%s", input-1);
        return ses;
    }

    while (*input)
    {
        while (*input == ';')
            input=space_out(input+1);
        if (pvars)
        {
            input = get_command(input, command);
            substitute_vars( command, result);
            substitute_myvars( result, command, ses);
            nspaces=0;
            while (*input==' ')
            {
                input++;
                nspaces++;
            }
            input = get_arg_all(input, arg);
            substitute_vars( arg, result);
            substitute_myvars( result, arg, ses);
        }
        else
        {
            input = get_command(input, result);            
            substitute_myvars( result, command, ses);
            nspaces=0;
            while (*input==' ')
            {
                input++;
                nspaces++;
            }
            input = get_arg_all(input, result);
            substitute_myvars( result, arg, ses);
        }

        if (in_alias)
        {
            if (!*input && *(*pvars)[0])
            {
                if (strlen(arg)+1+strlen((*pvars)[0]) > BUFFER_SIZE-10)
                {
                    if (!aborting)
                    {
                        tintin_eprintf(ses,"#ERROR: arguments too long in %s %s %s",command,arg,(*pvars)[0]);
                        aborting=1;
                    }
                }
                if (*arg)
                    strcat(arg," ");
                strcat(arg,(*pvars)[0]);
            };
            in_alias=0;
        }
        if (aborting)
        {
            aborting=0;
            return ses;
        }
        if (ses->debuglogfile)
            debuglog(ses, "%s", command);
    
        if (*command == tintin_char)
            ses = parse_tintin_command(command + 1, arg, ses);
        else if ((al = get_hash(ses->aliases, command)))
        {
            int i;
            char *cpsource;
            pvars_t vars,*lastpvars;

            strcpy(vars[0], arg);

            for (i = 1, cpsource = arg; i < 10; i++)
                cpsource=get_arg_in_braces(cpsource,vars[i],0);
            in_alias=1;
            lastpvars=pvars;
            pvars=&vars;
            strcpy(arg, al); /* alias can #unalias itself */
            ses = parse_input(arg,1,ses);
            pvars=lastpvars;
        }
        else if (ses->speedwalk && !*arg && is_speedwalk_dirs(command))
            do_speedwalk(command, ses);
        else
#ifdef GOTO_CHAR
            if ((*arg)||!do_goto(command,ses))
#endif
            {
                get_arg_with_spaces(arg, arg);
                write_com_arg_mud(command, arg, nspaces, ses);
            }
    }
    aborting=0;
    return (ses);
}

/************************************************************************/
/* return TRUE if commands only consists of lowercase letters N,S,E ... */
/************************************************************************/
inline int is_speedwalk_dirs(char *cp)
{
    int flag;

    flag = FALSE;
    while (*cp)
    {
        if (*cp != 'n' && *cp != 'e' && *cp != 's' && *cp != 'w' && *cp != 'u' && *cp != 'd' &&
                !isdigit(*cp))
            return FALSE;
        if (!isdigit(*cp))
            flag = TRUE;
        cp++;
    }
    return flag;
}

/**************************/
/* do the speedwalk thing */
/**************************/
void do_speedwalk(char *cp, struct session *ses)
{
    char sc[2];
    char *loc;
    int multflag, loopcnt, i;

    strcpy(sc, "x");
    while (*cp)
    {
        loc = cp;
        multflag = FALSE;
        while (isdigit(*cp))
        {
            cp++;
            multflag = TRUE;
        }
        if (multflag && *cp)
        {
            sscanf(loc, "%d%c", &loopcnt, sc);
            i = 0;
            while (i++ < loopcnt)
                write_com_arg_mud(sc, "", 0, ses);
        }
        else if (*cp)
        {
            sc[0] = *cp;
            write_com_arg_mud(sc, "", 0, ses);
        }
        /* Added the if to make sure we didn't move the pointer outside the
           bounds of the origional pointer.  Corrects the bug with speedwalking
           where if you typed "u7" tintin would go apeshit. (JE)
         */
        if (*cp)
            cp++;
    }
}


int do_goto(char *txt,struct session *ses)
{
    char *ch;

    if (!(ch=strchr(txt,GOTO_CHAR)))
        return 0;
    if (ch+1>=strchr(txt,0))
        return 0;
    if (ch!=txt)
    {
        *ch=' ';
        goto_command(txt,ses);
    }
    else
    {
        char tmp[BUFFER_SIZE];

        if (!(ch=get_hash(ses->myvars, "loc"))||(!*ch))
        {
            tintin_eprintf(ses,"#Cannot goto from $loc, it is not set!");
            return 1;
        }
        sprintf(tmp,"{%s} {%s}",ch,txt+1);
        goto_command(tmp,ses);
    }
    return 1;
}

/*************************************/
/* parse most of the tintin-commands */
/*************************************/
struct session *parse_tintin_command(char *command, char *arg,struct session *ses)
{
    struct session *sesptr;
    char *func;

    for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
        if (strcmp(sesptr->name, command) == 0)
        {
            if (*arg)
            {
                get_arg_in_braces(arg, arg, 1);
                parse_input(arg,1, sesptr);	/* was: #sessioname commands */
                return (ses);
            }
            else
            {
                activesession = sesptr;
                tintin_printf(ses, "#SESSION '%s' ACTIVATED.", sesptr->name);
                return (sesptr);
            }
        }
    if (isdigit(*command))
    {
        int i = atoi(command);

        if (i > 0)
        {
            get_arg_in_braces(arg, arg, 1);
            while (i-- > 0)
                ses = parse_input(arg,1, ses);
        }
        else
        {
            tintin_eprintf(ses,"#Cannot repeat a command a non-positive number of times.");
            prompt(ses);
        }
        return (ses);
    }
        
    else if ((func=get_hash(c_commands, command)))
    {
        ses=((t_c_command)func)(arg, ses);
    }
    
    else if ((func=get_hash(commands, command)))
    {
        ((t_command)func)(arg, ses);
    }
                
    else
    {
        tintin_eprintf(ses,"#UNKNOWN TINTIN-COMMAND: [%c%s]",tintin_char,command);
        prompt(ses);
    }
    return (ses);
}

void add_command(struct hashtable *h, char *command, t_command func)
{
    char cmd[BUFFER_SIZE];
    int n;

    if (get_hash(c_commands, command) || get_hash(commands, command))
    {
        fprintf(stderr, "Cannot add command: {%s}.\n", command);
        exit(1);
    }
    strcpy(cmd, command);
    for(n=strlen(cmd); n; n--)
    {
        cmd[n]=0;
        if (!get_hash(c_commands, cmd) && !get_hash(commands, cmd))
            set_hash_nostring(h, cmd, (char*)func);
    }
}

void init_parse()
{
    commands=init_hash();
    c_commands=init_hash();
    set_hash_nostring(commands, "end", (char*)end_command);
    set_hash_nostring(commands, "unlink", (char*)unlink_command);
#include "load_commands.h"
}


/**********************************************/
/* get all arguments - don't remove "s and \s */
/**********************************************/
char *get_arg_all(char *s, char *arg)
{
    /* int inside=FALSE; */
    int nest = 0;

    s = space_out(s);
    while (*s)
    {

        if (*s == '\\')
        {
            *arg++ = *s++;
            if (*s)
                *arg++ = *s++;
        }
        else if (*s == ';' && nest < 1)
        {
            break;
        }
        else if (*s == DEFAULT_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == DEFAULT_CLOSE)
        {
            nest--;
            *arg++ = *s++;
        }
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}

/****************************************************/
/* get an inline command - terminated with 0 or ')' */
/****************************************************/
char *get_inline(char *s, char *arg)
{
    /* int inside=FALSE; */
    int nest = 0;

    s = space_out(s);
    while (*s)
    {
        if (*s == '\\')
        {
            *arg++ = *s++;
            if (*s)
                *arg++ = *s++;
        }
        else if (*s == ')' && nest < 1)
            break;
        else if (*s == DEFAULT_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == DEFAULT_CLOSE)
        {
            nest--;
            *arg++ = *s++;
        }
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}

/**************************************/
/* get all arguments - remove "s etc. */
/* Example:                           */
/* In: "this is it" way way hmmm;     */
/* Out: this is it way way hmmm       */
/**************************************/
inline char *get_arg_with_spaces(char *s, char *arg)
{
    int nest = 0;

    /* int inside=FALSE; */

    s = space_out(s);
    while (*s)
    {

        if (*s == '\\')
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s == ';' && nest == 0)
            break;
        else if (*s == DEFAULT_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == DEFAULT_CLOSE)
        {
            *arg++ = *s++;
            nest--;
        }
        else
            *arg++ = *s++;
    }
    *arg = '\0';
    return s;
}

inline char *get_arg_in_braces(char *s,char *arg,int flag)
{
    int nest = 0;
    char *ptr;

    s = space_out(s);
    ptr = s;
    if (*s != DEFAULT_OPEN)
    {
        if (flag == 0)
            s = get_arg_stop_spaces(ptr, arg);
        else
            s = get_arg_with_spaces(ptr, arg);
        return s;
    }
    s++;
    while (*s != '\0' && !(*s == DEFAULT_CLOSE && nest == 0))
    {
        if (*s=='\\')
            ;
        else
            if (*s == DEFAULT_OPEN)
                nest++;
            else
                if (*s == DEFAULT_CLOSE)
                    nest--;
        *arg++ = *s++;
    }
    if (!*s)
        tintin_eprintf(0,"#Unmatched braces error! Bad argument is \"%s\".", ptr);
    else
        s++;
    *arg = '\0';
    return s;

}
/**********************************************/
/* get one arg, stop at spaces                */
/* remove quotes                              */
/**********************************************/
inline char *get_arg_stop_spaces(char *s, char *arg)
{
    int inside = FALSE;

    s = space_out(s);

    while (*s)
    {
        if (*s == '\\')
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s == '"')
        {
            s++;
            inside = !inside;
        }
        else if (*s == ';')
        {
            if (inside)
                *arg++ = *s++;
            else
                break;
        }
        else if (!inside && *s == ' ')
            break;
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}


char *get_arg(char *s,char *arg,int flag,struct session *ses)
{
    char tmp[BUFFER_SIZE],*cptr;
    
    cptr=get_arg_in_braces(s,arg,flag);
    if (*s)
    {
        substitute_vars(arg,tmp);
        substitute_myvars(tmp,arg,ses);
    }
    return cptr;
}

/**********************************************/
/* get the command, stop at spaces            */
/* remove quotes                              */
/**********************************************/
char *get_command(char *s, char *arg)
{
    int inside = FALSE;

    while (*s)
    {
        if (*s == '\\')
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s == '"')
        {
            s++;
            inside = !inside;
        }
        else if (*s == ';')
        {
            if (inside)
                *arg++ = *s++;
            else
                break;
        }
        else if (!inside && (*s==' ' || *s==9))
            break;
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}

/*********************************************/
/* spaceout - advance ptr to next none-space */
/* return: ptr to the first none-space       */
/*********************************************/
inline char *space_out(char *s)
{
    while (isspace(*s))
        s++;
    return s;
}

/************************************/
/* send command+argument to the mud */
/************************************/
void write_com_arg_mud(char *command, char *argument, int nsp, struct session *ses)
{
    char outtext[BUFFER_SIZE];
    int i;

    if (ses==nullsession)
    {
        tintin_eprintf(ses, "#NO SESSION ACTIVE. USE THE %cSESSION COMMAND TO START ONE.", tintin_char);
        prompt(NULL);
    }
    else
    {
        check_insert_path(command, ses, 0);
        strncpy(outtext, command, BUFFER_SIZE);
        if (*argument)
        {
            if (nsp<1)
                nsp=1;
            i=BUFFER_SIZE-1-strlen(outtext);
            while (nsp--)
                strncat(outtext, " ", i),i--;
            strncat(outtext, argument, i);
        }
        do_out_MUD_colors(outtext);
        write_line_mud(outtext, ses);
        outtext[i = strlen(outtext)] = '\n';
        if (ses->logfile)
            if (fwrite(outtext, i + 1, 1, ses->logfile)<1)
            {
                ses->logfile=0;
                tintin_eprintf(ses, "#WRITE ERROR -- LOGGING DISABLED.  Disk full?");
            };
    }
}


/***************************************************************/
/* show a prompt - mud prompt if we're connected/else just a > */
/***************************************************************/
void prompt(struct session *ses)
{
#if FORCE_PROMPT
    if (ses && !PSEUDO_PROMPT)
        write_line_mud("", ses);
    else tintin_printf(">",ses);
#endif
}
