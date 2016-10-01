/*********************************************************************/
/* file: parse.c - some utility-functions                            */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/action.h"
#include "protos/colors.h"
#include "protos/files.h"
#include "protos/globals.h"
#include "protos/hash.h"
#include "protos/hooks.h"
#include "protos/print.h"
#include "protos/prof.h"
#include "protos/net.h"
#include "protos/parse.h"
#include "protos/path.h"
#include "protos/utils.h"
#include "protos/variables.h"

#include "commands.h"
typedef void (*t_command)(const char*, struct session*);
typedef struct session *(*t_c_command)(const char*, struct session*);

static struct session *parse_tintin_command(const char *command, const char *arg, struct session *ses);
static void do_speedwalk(const char *cp, struct session *ses);
static bool do_goto(const char *txt, struct session *ses);
static inline const char *get_arg_with_spaces(const char *s, char *arg);
static const char* get_command(const char *s, char *arg);
static void write_com_arg_mud(const char *command, const char *argument, int nsp, struct session *ses);
extern void end_command(const char *arg, struct session *ses);
extern void unlink_command(const char *arg, struct session *ses);

static inline bool is_speedwalk_dirs(const char *cp);

static inline const char *get_arg_stop_spaces(const char *s, char *arg);
static const char *get_arg_all(const char *s, char *arg);
bool in_alias=false;
static struct hashtable *commands, *c_commands;
int recursion;

/**************************************************************************/
/* parse input, check for TINTIN commands and aliases and send to session */
/**************************************************************************/
struct session* parse_input(const char *input, bool override_verbatim, struct session *ses)
{
    char command[BUFFER_SIZE], arg[BUFFER_SIZE], result[BUFFER_SIZE], *al;
    int nspaces;
#ifdef PROFILING
    const char *oldprof=prof_area;
    PROF("parsing input");
# define PPOP prof_area=oldprof
#else
# define PPOP
#endif

    if (++recursion>=MAX_RECURSION)
    {
        in_alias=false;
        if (recursion==MAX_RECURSION)
            tintin_eprintf(ses, "#TOO DEEP RECURSION.");
        recursion=MAX_RECURSION*3;
        PPOP;
        return ses;
    }

    debuglog(ses, "%s", input);
    if (!ses->server_echo && activesession == ses)
        term_echoing = true;
    if (*input == '\0')
    {
        if (ses!=nullsession)
        {
            write_line_mud("", ses);
            debuglog(ses, "");
        }
        else if (!in_read)
            write_com_arg_mud("", "", 0, ses);
        PPOP;
        return ses;
    }
    if ((*input==tintin_char) && is_abrev(input + 1, "verbatim"))
    {
        verbatim_command("", ses);
        debuglog(ses, "%s", input);
        PPOP;
        return ses;
    }
    if (ses->verbatim && !override_verbatim && (ses!=nullsession))
    {
        write_line_mud(input, ses);
        debuglog(ses, "%s", input);
        PPOP;
        return ses;
    }
    if (*input==verbatim_char && (ses!=nullsession))
    {
        input++;
        write_line_mud(input, ses);
        debuglog(ses, "%s", input-1);
        PPOP;
        return ses;
    }

    while (*input)
    {
        PROFPUSH("expanding variables");
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
        PROFPOP;

        if (in_alias)
        {
            if (!*input && *(*pvars)[0])
            {
                if (strlen(arg)+1+strlen((*pvars)[0]) > BUFFER_SIZE-10)
                {
                    if (!aborting)
                    {
                        tintin_eprintf(ses, "#ERROR: arguments too long in %s %s %s", command, arg, (*pvars)[0]);
                        aborting=true;
                    }
                }
                if (*arg)
                    strcat(arg, " ");
                strcat(arg, (*pvars)[0]);
            }
            in_alias=false;
        }
        if (recursion>MAX_RECURSION)
        {
            PPOP;
            return ses;
        }
        if (aborting)
        {
            aborting=false;
            recursion--;
            PPOP;
            return ses;
        }
        debuglog(ses, "%s", command);

        if (*command == tintin_char)
            ses = parse_tintin_command(command + 1, arg, ses);
        else if ((al = get_hash(ses->aliases, command)))
        {
            const char *cpsource=arg;
            pvars_t vars, *lastpvars;

            strcpy(vars[0], arg);

            PROF("expanding aliases");
            for (int i = 1; i < 10; i++)
                cpsource=get_arg_in_braces(cpsource, vars[i], 0);
            in_alias=true;
            lastpvars=pvars;
            pvars=&vars;
            strcpy(arg, al); /* alias can #unalias itself */
            ses = parse_input(arg, true, ses);
            pvars=lastpvars;
        }
        else if (ses->speedwalk && !*arg && is_speedwalk_dirs(command))
            do_speedwalk(command, ses);
        else
#ifdef GOTO_CHAR
            if ((*arg)||!do_goto(command, ses))
#endif
            {
                PROF("expanding text being sent");
                get_arg_with_spaces(arg, arg);
                write_com_arg_mud(command, arg, nspaces, ses);
            }
    }
    aborting=false;
    recursion--;
    PPOP;
    return ses;
}

/************************************************************************/
/* return true if commands only consists of lowercase letters N,S,E ... */
/************************************************************************/
static inline bool is_speedwalk_dirs(const char *cp)
{
    bool res = false;

    while (*cp)
    {
        if (*cp != 'n' && *cp != 'e' && *cp != 's' && *cp != 'w' && *cp != 'u' && *cp != 'd' &&
                !isadigit(*cp))
            return false;
        if (!isadigit(*cp))
            res = true;
        cp++;
    }
    return res;
}

/**************************/
/* do the speedwalk thing */
/**************************/
static void do_speedwalk(const char *cp, struct session *ses)
{
    char sc[2];
    const char *loc;
    int loopcnt, i;

    strcpy(sc, "x");
    while (*cp)
    {
        loc = cp;
        bool multflag = false;
        while (isadigit(*cp))
        {
            cp++;
            multflag = true;
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
           bounds of the original pointer.  Corrects the bug with speedwalking
           where if you typed "u7" tintin would go apeshit. (JE)
         */
        if (*cp)
            cp++;
    }
}


static bool do_goto(const char *txt, struct session *ses)
{
    char *ch;

    if (!(ch=strchr(txt, GOTO_CHAR)))
        return false;
    if (ch+1>=strchr(txt, 0))
        return false;
    if (ch!=txt)
    {
        *ch=' ';
        goto_command(txt, ses);
    }
    else
    {
        char tmp[BUFFER_SIZE];

        if (!(ch=get_hash(ses->myvars, "loc"))||(!*ch))
        {
            tintin_eprintf(ses, "#Cannot goto from $loc, it is not set!");
            return true; // was syntaxically correct
        }
        sprintf(tmp, "{%s} {%s}", ch, txt+1);
        goto_command(tmp, ses);
    }
    return true;
}

/*************************************/
/* parse most of the tintin-commands */
/*************************************/
static struct session* parse_tintin_command(const char *command, const char *arg, struct session *ses)
{
    const char *func, *a;
    char *b, cmd[BUFFER_SIZE], right[BUFFER_SIZE];
#ifdef PROFILING
    const char *oldprof=prof_area;
    PROF("executing commands");
#endif

    for (struct session *sesptr = sessionlist; sesptr; sesptr = sesptr->next)
        if (strcmp(sesptr->name, command) == 0)
        {
            if (*arg)
            {
                get_arg_in_braces(arg, right, 1);
                parse_input(right, true, sesptr);     /* was: #sessioname commands */
                PPOP;
                return ses;
            }
            else
            {
                activesession = sesptr;
                tintin_printf(ses, "#SESSION '%s' ACTIVATED.", sesptr->name);
                do_hook(ses, HOOK_DEACTIVATE, 0, true);  /* FIXME: blockzap */
                PPOP;
                return do_hook(sesptr, HOOK_ACTIVATE, 0, false);
            }
        }

    a=command;
    b=cmd;
    while (*a)
        *b++=toalower(*a++);
    *b=0;

    if (isadigit(*command))
    {
        int i = atoi(command);

        if (i > 0)
        {
            get_arg_in_braces(arg, right, 1);
            while (i-- > 0)
                ses = parse_input(right, true, ses);
        }
        else
            tintin_eprintf(ses, "#Cannot repeat a command a non-positive number of times.");
        PPOP;
        return ses;
    }
    else if ((func=get_hash(c_commands, cmd)))
        ses=((t_c_command)func)(arg, ses);
    else if ((func=get_hash(commands, cmd)))
        ((t_command)func)(arg, ses);
    else
        tintin_eprintf(ses, "#UNKNOWN TINTIN-COMMAND: [%c%s]", tintin_char, command);
    PPOP;
    return ses;
}

static void add_command(struct hashtable *h, const char *command, t_command func)
{
    char cmd[BUFFER_SIZE];

    if (get_hash(c_commands, command) || get_hash(commands, command))
    {
        fprintf(stderr, "Cannot add command: {%s}.\n", command);
        exit(1);
    }
    strcpy(cmd, command);
    for (int n=strlen(cmd); n; n--)
    {
        cmd[n]=0;
        if (!get_hash(c_commands, cmd) && !get_hash(commands, cmd))
            set_hash_nostring(h, cmd, (char*)func);
    }
}

void init_parse(void)
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
static const char* get_arg_all(const char *s, char *arg)
{
    int nest = 0;

    s = space_out(s);
    while (*s)
    {

        if (*s==CHAR_VERBATIM)     /* \ */
        {
            *arg++ = *s++;
            if (*s)
                *arg++ = *s++;
        }
        else if (nest<1 && (*s==CHAR_NEWLINE)) /* ; */
        {
            break;
        }
        else if (*s == BRACE_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == BRACE_CLOSE)
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
const char* get_inline(const char *s, char *arg)
{
    int nest = 0;

    s = space_out(s);
    while (*s)
    {
        if (*s==CHAR_VERBATIM)     /* \ */
        {
            *arg++ = *s++;
            if (*s)
                *arg++ = *s++;
        }
        else if (*s == ')' && nest < 1)
            break;
        else if (*s == BRACE_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == BRACE_CLOSE)
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
static inline const char* get_arg_with_spaces(const char *s, char *arg)
{
    int nest = 0;

    s = space_out(s);
    while (*s)
    {

        if (*s==CHAR_VERBATIM)     /* \ */
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s == ';' && nest == 0)
            break;
        else if (*s == BRACE_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == BRACE_CLOSE)
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

const char* get_arg_in_braces(const char *s, char *arg, bool allow_spaces)
{
    int nest = 0;
    const char *ptr;

    s = space_out(s);
    ptr = s;
    if (*s != BRACE_OPEN)
    {
        if (allow_spaces)
            s = get_arg_with_spaces(ptr, arg);
        else
            s = get_arg_stop_spaces(ptr, arg);
        return s;
    }
    s++;
    while (*s != '\0' && !(*s == BRACE_CLOSE && nest == 0))
    {
        if (*s==CHAR_VERBATIM)     /* \ */
            ;
        else if (*s == BRACE_OPEN)
            nest++;
        else if (*s == BRACE_CLOSE)
            nest--;
        *arg++ = *s++;
    }
    if (!*s)
        tintin_eprintf(0, "#Unmatched braces error! Bad argument is \"%s\".", ptr);
    else
        s++;
    *arg = '\0';
    return s;

}
/**********************************************/
/* get one arg, stop at spaces                */
/* remove quotes                              */
/**********************************************/
static inline const char* get_arg_stop_spaces(const char *s, char *arg)
{
    bool inside = false;

    s = space_out(s);

    while (*s)
    {
        if (*s==CHAR_VERBATIM)     /* \ */
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s==CHAR_QUOTE)   /* " */
        {
            s++;
            inside = !inside;
        }
        else if (!inside && *s == ' ')
            break;
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}


const char* get_arg(const char *s, char *arg, bool allow_spaces, struct session *ses)
{
    const char *cptr=get_arg_in_braces(s, arg, allow_spaces);
    if (*arg)
    {
        char tmp[BUFFER_SIZE];
        substitute_vars(arg, tmp);
        substitute_myvars(tmp, arg, ses);
    }
    return cptr;
}

/**********************************************/
/* get the command, stop at spaces            */
/* remove quotes                              */
/**********************************************/
static const char* get_command(const char *s, char *arg)
{
    bool inside = false;

    while (*s)
    {
        if (*s==CHAR_VERBATIM)     /* \ */
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s==CHAR_QUOTE)   /* " */
        {
            s++;
            inside = !inside;
        }
        else if (*s==CHAR_NEWLINE) /* ; */
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
const char* space_out(const char *s)
{
    while (isaspace(*s))
        s++;
    return s;
}

/************************************/
/* send command+argument to the mud */
/************************************/
static void write_com_arg_mud(const char *command, const char *argument, int nsp, struct session *ses)
{
    char outtext[BUFFER_SIZE];
    int i;

    if (ses==nullsession)
    {
#if 0
        tintin_eprintf(ses, "#NO SESSION ACTIVE. USE THE %cSESSION COMMAND TO START ONE.  (was: {%s} {%s})", tintin_char, command, argument);
#else
        tintin_eprintf(ses, "#NO SESSION ACTIVE. USE THE %cSESSION COMMAND TO START ONE.", tintin_char);
#endif
    }
    else
    {
        check_insert_path(command, ses);
        strncpy(outtext, command, BUFFER_SIZE);
        if (*argument)
        {
            if (nsp<1)
                nsp=1;
            i=BUFFER_SIZE-1-strlen(outtext);
            while (nsp--)
                strncat(outtext, " ", i), i--;
            strncat(outtext, argument, i);
        }
        do_out_MUD_colors(outtext);
        write_line_mud(outtext, ses);
    }
}
