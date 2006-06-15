/* $Id: files.c,v 1.8 1998/11/25 17:14:00 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: files.c - funtions for logfile and reading/writing comfiles */
/*                             TINTIN + +                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*                    New code by Bill Reiss 1993                    */
/*********************************************************************/
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <stdarg.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "tintin.h"

void prepare_for_write(char *command, char *left, char *right, char *pr, char *result);

extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern struct session *nullsession;
extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern void user_done(void);
extern struct listnode *searchnode_list(struct listnode *listhead,char *cptr);
extern void prompt(struct session *ses);
extern void char_command(char *arg,struct session *ses);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);
extern void tintin_printf(struct session *ses, char *format, ...);
extern void tintin_eprintf(struct session *ses, char *format, ...);
extern void user_condump(FILE *f);
extern char *space_out(char *s);
extern struct listnode* hash2list(struct hashtable *h, char *pat);
extern void zap_list(struct listnode *nptr);
extern char* get_hash(struct hashtable *h, char *key);
extern void write_line_mud(char *line, struct session *ses);
extern FILE *mypopen(const char *command, int wr);
extern char *mystrdup(char *s);
extern char *get_arg(char *s,char *arg,int flag,struct session *ses);

extern int puts_echoing;
extern int alnum, acnum, subnum, hinum, varnum, antisubnum, routnum, bindnum, pdnum, hooknum;
extern char tintin_char;
extern int recursion;
extern char *hook_names[];
extern int keypad, retain;

int in_read=0;

/*******************************/
/* expand tildes in a filename */
/*******************************/
void expand_filename(char *arg,char *result)
{
    if (*arg=='~')
    {
        if (*(arg+1)=='/')
            result+=sprintf(result,"%s",getenv("HOME")), arg++;
        else
        {
            char *p;
            char name[BUFFER_SIZE];
            struct passwd *pwd;
            
            p=strchr(arg+1, '/');
            if (p)
            {
                strncpy(name, arg+1, p-arg-1);
                pwd=getpwnam(name);
                if (pwd)
                    result+=sprintf(result,"%s",pwd->pw_dir), arg=p;
            }
        };
    };
    strcpy(result, arg);
}

#if 0
/**********************************/
/* load a completion file         */
/**********************************/
void read_complete(char *arg, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE], *cptr;
    int flag;
    struct completenode *tcomplete, *tcomp2;

    flag = TRUE;
    if ((complete_head = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL)
    {
        fprintf(stderr, "couldn't alloc completehead\n");
        user_done();
        exit(1);
    }
    tcomplete = complete_head;

    if ((myfile = fopen("tab.txt", "r")) == NULL)
    {
        if ((cptr = (char *)getenv("HOME")))
        {
            strcpy(buffer, cptr);
            strcat(buffer, "/.tab.txt");
            myfile = fopen(buffer, "r");
        }
    }
    if (myfile == NULL)
    {
        tintin_eprintf(0,"no tab.txt file, no completion list");
        return;
    }
    while (fgets(buffer, sizeof(buffer), myfile))
    {
        for (cptr = buffer; *cptr && *cptr != '\n'; cptr++) ;
        *cptr = '\0';
        if ((tcomp2 = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL)
        {
            fprintf(stderr, "couldn't alloc comletehead\n");
            user_done();
            exit(1);
        }
        if ((cptr = (char *)(malloc(strlen(buffer) + 1))) == NULL)
        {
            fprintf(stderr, "couldn't alloc memory for string in complete\n");
            user_done();
            exit(1);
        }
        strcpy(cptr, buffer);
        tcomp2->strng = cptr;
        tcomplete->next = tcomp2;
        tcomplete = tcomp2;
    }
    tcomplete->next = NULL;
    fclose(myfile);
    tintin_printf(0,"tab.txt file loaded.");
    prompt(NULL);
    tintin_printf(0,"");

}
#endif

/*******************************/
/* remove file from filesystem */
/*******************************/
void unlink_command(char *arg, struct session *ses)
{
    char file[BUFFER_SIZE], temp[BUFFER_SIZE];

    if(*arg)
    {
        arg = get_arg_in_braces(arg, temp, 1);
        substitute_vars(temp, file);
        substitute_myvars(file, temp, ses);
        expand_filename(temp, file);
        unlink(file);
    }
    else
        tintin_eprintf(ses, "#ERROR: valid syntax is: #unlink <filename>");

}

/*************************/
/* the #deathlog command */
/*************************/
void deathlog_command(char *arg, struct session *ses)
{
    FILE *fh;
    char fname[BUFFER_SIZE], text[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (*arg)
    {
        arg = get_arg_in_braces(arg, temp, 0);
        arg = get_arg_in_braces(arg, text, 1);
        substitute_vars(temp, fname);
        substitute_myvars(fname, temp, ses);
        expand_filename(temp, fname);
        substitute_vars(text, temp);
        substitute_myvars(temp, text, ses);
        if ((fh = fopen(fname, "a")))
        {
            if (text)
            {
                /*	fwrite(text, strlen(text), 1, fh);
                	fputc('\n', fh); */
                fprintf(fh, "%s\n", text);
            }
            fclose(fh);
        }
        else
            tintin_eprintf(ses, "#ERROR: COULDN'T OPEN FILE {%s}.", fname);
    }
    else
        tintin_eprintf(ses, "#ERROR: valid syntax is: #deathlog <file> <text>");
}

/***************************/
/* the #logcomment command */
/***************************/
void logcomment_command(char *arg, struct session *ses)
{
    char text[BUFFER_SIZE];

    if (!arg)
    {
    	tintin_eprintf(ses, "#Logcomment what?");
    	return;
    }
    if (!ses->logfile)
    {
        tintin_eprintf(ses, "#You're not logging.");
        return;
    }
    arg=get_arg(arg, text, 1, ses);
    if (fprintf(ses->logfile, "%s\n", text)<1)
    {
	    ses->logfile=0;
        tintin_eprintf(ses, "#WRITE ERROR -- LOGGING DISABLED.  Disk full?");
    }
}

#ifdef UI_FULLSCREEN
/************************/
/* the #condump command */
/************************/
void condump_command(char *arg, struct session *ses)
{
    FILE *fh;
    char fname[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (*arg)
    {
        arg = get_arg_in_braces(arg, temp, 0);
        substitute_vars(temp, fname);
        substitute_myvars(fname, temp, ses);
        expand_filename(temp, fname);
        if ((strlen(fname)<4)||(strcmp(fname+strlen(fname)-3,".gz")))
            fh = fopen(fname, "w");
        else
            fh = mypopen(strcat(strcpy(temp,"gzip -9 >"),fname), 1);
        if (fh)
        {
            user_condump(fh);
            fclose(fh);
        }
        else
            tintin_eprintf(ses,"#ERROR: COULDN'T OPEN FILE {%s}.", fname);
    }
    else
        tintin_eprintf(ses, "#Syntax: #condump <file>");
    prompt(NULL);
}
#endif

/********************/
/* the #log command */
/********************/
void log_command(char *arg, struct session *ses)
{
    char fname[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (ses!=nullsession)
    {
        if (*arg)
        {
            if (ses->logfile)
            {
                fclose(ses->logfile);
                tintin_printf(ses, "#OK. LOGGING TURNED OFF.");
                ses->logfile = NULL;
                free(ses->logname);
                ses->logname = NULL;
            }
            get_arg_in_braces(arg, temp, 1);
            substitute_vars(temp, fname);
            substitute_myvars(fname, temp, ses);
            expand_filename(temp, fname);
            if ((strlen(fname)<4)||(strcmp(fname+strlen(fname)-3,".gz")))
                if ((ses->logfile = fopen(fname, "w")))
                    tintin_printf(ses, "#OK. LOGGING TO {%s} .....", fname);
                else
                    tintin_eprintf(ses, "#ERROR: COULDN'T OPEN FILE {%s}.", fname);
            else
                if ((ses->logfile = mypopen(strcat(strcpy(temp,"gzip -9 >"),fname), 1)))
                    tintin_printf(ses, "#OK. LOGGING TO {%s} .....", fname);
                else
                    tintin_eprintf(ses, "#ERROR: COULDN'T OPEN PIPE: {gzip -9 >%s}.", fname);
            if (ses->logfile)
                ses->logname=mystrdup(fname);
        }
        else if (ses->logfile)
        {
            fclose(ses->logfile);
            ses->logfile = NULL;
            free(ses->logname);
            ses->logname = NULL;
            tintin_printf(ses, "#OK. LOGGING TURNED OFF.");
        }
        else
            tintin_printf(ses, "#LOGGING ALREADY OFF.");
    }
    else
        tintin_eprintf(ses, "#THERE'S NO SESSION TO LOG.");
    prompt(NULL);
}

/*************************/
/* the #debuglog command */
/*************************/
void debuglog_command(char *arg, struct session *ses)
{
    char fname[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (*arg)
    {
        if (ses->debuglogfile)
        {
            fclose(ses->debuglogfile);
            tintin_printf(ses, "#OK. DEBUGLOG TURNED OFF.");
            ses->debuglogfile = NULL;
            free(ses->debuglogname);
            ses->debuglogname = NULL;
        }
        get_arg_in_braces(arg, temp, 1);
        substitute_vars(temp, fname);
        substitute_myvars(fname, temp, ses);
        expand_filename(temp, fname);
        if ((strlen(fname)<4)||(strcmp(fname+strlen(fname)-3,".gz")))
            if ((ses->debuglogfile = fopen(fname, "w")))
                tintin_printf(ses, "#OK. DEBUGLOG SET TO {%s} .....", fname);
            else
                tintin_eprintf(ses, "#ERROR: COULDN'T OPEN FILE {%s}.", fname);
        else
            if ((ses->debuglogfile = mypopen(strcat(strcpy(temp,"gzip -9 >"),fname), 1)))
                tintin_printf(ses, "#OK. DEBUGLOG SET TO {%s} .....", fname);
            else
                tintin_eprintf(ses, "#ERROR: COULDN'T OPEN PIPE: {gzip -9 >%s}.", fname);
        if (ses->debuglogfile)
            ses->debuglogname=mystrdup(fname);
    }
    else if (ses->debuglogfile)
    {
        fclose(ses->debuglogfile);
        ses->debuglogfile = NULL;
        free(ses->debuglogname);
        ses->debuglogname = NULL;
        tintin_printf(ses, "#OK. DEBUGLOG TURNED OFF.");
    }
    else
        tintin_printf(ses, "#DEBUGLOG ALREADY OFF.");
    prompt(NULL);
}

void debuglog(struct session *ses, const char *format, ...)
{
    va_list ap;
#ifdef HAVE_VSNPRINTF
    char buf[BUFFER_SIZE];
#else
    char buf[BUFFER_SIZE*4]; /* let's hope this will never overflow... */
#endif
    struct timeval tv;

    if (ses->debuglogfile)
    {
        gettimeofday(&tv, 0);
        va_start(ap, format);
#ifdef HAVE_VSNPRINTF
        if (vsnprintf(buf, BUFFER_SIZE-1, format, ap)>BUFFER_SIZE-2)
            buf[BUFFER_SIZE-3]='>';
#else
        vsprintf(buf, format, ap);
#endif
        va_end(ap);
        fprintf(ses->debuglogfile, "%4d.%06d: %s\n",
            (int)tv.tv_sec-ses->sessionstart, (int)tv.tv_usec, buf);
    }
}


struct session *do_read(FILE *myfile, char *filename, struct session *ses)
{
    char line[BUFFER_SIZE], buffer[BUFFER_SIZE], *cptr, *eptr;
    int flag,nl,ignore_lines;

    flag = !in_read;
    if (!ses->verbose)
        puts_echoing = FALSE;
    if (!in_read)
    {
        alnum = 0;
        acnum = 0;
        subnum = 0;
        varnum = 0;
        hinum = 0;
        antisubnum = 0;
        routnum=0;
        bindnum=0;
        pdnum=0;
        hooknum=0;
    }
    in_read++;
    *buffer=0;
    ignore_lines=0;
    nl=0;
    while (fgets(line, BUFFER_SIZE, myfile))
    {
        nl++;
        if (flag)
        {
            puts_echoing = TRUE;
            if (ispunct(*line) || ((unsigned char)(*line)>127))
                char_command(line, ses);
            if (!ses->verbose)
                puts_echoing = FALSE;
            flag = FALSE;
        }
        for (cptr = line; *cptr && *cptr != '\n' && *cptr!='\r'; cptr++) ;
        *cptr = '\0';
        
        if (isspace(*line) && *buffer && (*buffer==tintin_char))
        {
            cptr=space_out(line);
            if (ignore_lines || (strlen(cptr)+strlen(buffer) >= BUFFER_SIZE/2))
            {
                puts_echoing=1;
                tintin_eprintf(ses,"#ERROR! LINE %d TOO LONG IN %s, TRUNCATING", nl, filename);
                *line=0;
                ignore_lines=1;
                puts_echoing=ses->verbose;
            }
            else
            {
                if (*cptr &&
                    !isspace(*(eptr=strchr(buffer,0)-1)) &&
                    (*eptr!=';') && (*cptr!=DEFAULT_CLOSE) &&
                    (*cptr!=tintin_char || *eptr!=DEFAULT_OPEN))
                    strcat(buffer," ");
                strcat(buffer,cptr);
                continue;
            }
        }
        ses = parse_input(buffer,1, ses);
        recursion=0;
        ignore_lines=0;
        strcpy(buffer, line);
    }
    if (*buffer)
    {
        ses=parse_input(buffer,1,ses);
        recursion=0;
    }
    in_read--;
    if (!ses->verbose && !in_read)
    {
        puts_echoing = TRUE;
        if (alnum > 0)
            tintin_printf(ses, "#OK. %d ALIASES LOADED.", alnum);
        if (acnum > 0)
            tintin_printf(ses, "#OK. %d ACTIONS LOADED.", acnum);
        if (antisubnum > 0)
            tintin_printf(ses, "#OK. %d ANTISUBS LOADED.", antisubnum);
        if (subnum > 0)
            tintin_printf(ses, "#OK. %d SUBSTITUTES LOADED.", subnum);
        if (varnum > 0)
            tintin_printf(ses, "#OK. %d VARIABLES LOADED.", varnum);
        if (hinum > 0)
            tintin_printf(ses, "#OK. %d HIGHLIGHTS LOADED.", hinum);
        if (routnum > 0)
            tintin_printf(ses, "#OK. %d ROUTES LOADED.", routnum);
        if (bindnum > 0)
            tintin_printf(ses, "#OK. %d BINDS LOADED.", bindnum);
        if (pdnum > 0)
            tintin_printf(ses, "#OK. %d PATHDIRS LOADED.", pdnum);
        if (hooknum > 0)
            tintin_printf(ses, "#OK. %d HOOKS LOADED.", hooknum);
    }
    fclose(myfile);
    prompt(NULL);
    return ses;
}


/*********************/
/* the #read command */
/*********************/
struct session *read_command(char *filename, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE];

    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, filename);
    substitute_myvars(filename, buffer, ses);
    expand_filename(buffer, filename);
    if (!*filename)
    {
        tintin_eprintf(ses, "#Syntax: #read filename");
        prompt(NULL);
        return ses;
    }
    if ((myfile = fopen(filename, "r")) == NULL)
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", filename);
        prompt(NULL);
        return ses;
    }
    
    return do_read(myfile, filename,ses);
}


#define WFLAG(name,var,org)		if(var!=org)                                \
                                    {                                       \
                                        sprintf(num, "%d", var);            \
                                        prepare_for_write(name, num, 0, 0, buffer); \
                                        fputs(buffer, myfile);              \
                                    }
#define SFLAG(name,var,org)     WFLAG(name,ses->var,org)
/**********************/
/* the #write command */
/**********************/
void write_command(char *filename, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE], num[32];
    struct listnode *nodeptr, *templist;
    struct routenode *rptr;
    int nr;

    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, filename);
    substitute_myvars(filename, buffer, ses);
    expand_filename(buffer, filename);
    if (*filename == '\0')
    {
        tintin_eprintf(ses, "#ERROR: syntax is: #write <filename>");
        prompt(NULL);
        return;
    }
    if ((myfile = fopen(filename, "w")) == NULL)
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", filename);
        prompt(NULL);
        return;
    }

    WFLAG("keypad", keypad, DEFAULT_KEYPAD);
    WFLAG("retain", retain, DEFAULT_RETAIN);
    SFLAG("echo", echo, DEFAULT_ECHO);
    SFLAG("ignore", ignore, DEFAULT_IGNORE);
    SFLAG("speedwalk", speedwalk, DEFAULT_SPEEDWALK);
    SFLAG("presub", presub, DEFAULT_PRESUB);
    SFLAG("togglesubs", togglesubs, DEFAULT_TOGGLESUBS);
    SFLAG("verbose", verbose, 0);
    SFLAG("blank", blank, DEFAULT_DISPLAY_BLANK);
    SFLAG("messages aliases", mesvar[0], DEFAULT_ALIAS_MESS);
    SFLAG("messages actions", mesvar[1], DEFAULT_ACTION_MESS);
    SFLAG("messages substitutes", mesvar[2], DEFAULT_SUB_MESS);
    SFLAG("messages events", mesvar[3], DEFAULT_EVENT_MESS);
    SFLAG("messages highlights", mesvar[4], DEFAULT_HIGHLIGHT_MESS);
    SFLAG("messages variables", mesvar[5], DEFAULT_VARIABLE_MESS);
    SFLAG("messages routes", mesvar[6], DEFAULT_ROUTE_MESS);
    SFLAG("messages gotos", mesvar[7], DEFAULT_GOTO_MESS);
    SFLAG("messages binds", mesvar[8], DEFAULT_BIND_MESS);
    SFLAG("messages #system", mesvar[9], DEFAULT_SYSTEM_MESS);
    SFLAG("messages paths", mesvar[10], DEFAULT_PATH_MESS);
    SFLAG("messages errors", mesvar[11], DEFAULT_ERROR_MESS);
    SFLAG("messages hooks", mesvar[12], DEFAULT_HOOK_MESS);
    SFLAG("verbatim", verbatim, 0);
    SFLAG("ticksize", tick_size, DEFAULT_TICK_SIZE);
    SFLAG("pretick", pretick, DEFAULT_PRETICK);
    nodeptr = templist = hash2list(ses->aliases, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("alias", nodeptr->left, nodeptr->right, 0, buffer);
        fputs(buffer, myfile);
    }
    zap_list(templist);

    nodeptr = ses->actions;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr,
                          buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->antisubs;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("antisub", nodeptr->left, 0, 0, buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->subs;
    while ((nodeptr = nodeptr->next))
    {
        if (strcmp( nodeptr->right, EMPTY_LINE))
            prepare_for_write("sub", nodeptr->left, nodeptr->right, 0, buffer);
        else
            prepare_for_write("gag", nodeptr->left, 0, 0, buffer);
        fputs(buffer, myfile);
    }

    nodeptr = templist = hash2list(ses->myvars, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("var", nodeptr->left, nodeptr->right, "\0", buffer);
        fputs(buffer, myfile);
    }
    zap_list(templist);
    
    nodeptr = ses->highs;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
        fputs(buffer, myfile);
    }
    
    nodeptr = templist = hash2list(ses->pathdirs, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("pathdir", nodeptr->left, nodeptr->right, 0, buffer);
        fputs(buffer, myfile);
    }
    zap_list(templist);

    for (nr=0;nr<MAX_LOCATIONS;nr++)
        if ((rptr=ses->routes[nr]))
            do
            {
                fprintf(myfile,(*(rptr->cond))
                        ?"#route {%s} {%s} {%s} %d {%s}\n"
                        :"#route {%s} {%s} {%s} %d\n",
                        ses->locations[nr],
                        ses->locations[rptr->dest],
                        rptr->path,
                        rptr->distance,
                        rptr->cond);
            } while((rptr=rptr->next));
            
    nodeptr = templist = hash2list(ses->binds, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("bind", nodeptr->left, nodeptr->right, 0, buffer);
        fputs(buffer, myfile);
    }
    zap_list(templist);
    
    for(nr=0;nr<NHOOKS;nr++)
        if (ses->hooks[nr])
        {
            prepare_for_write("hook", hook_names[nr], ses->hooks[nr], 0, buffer);
            fputs(buffer, myfile);
        }

    fclose(myfile);
    tintin_printf(ses, "#COMMANDS-FILE WRITTEN.");
    return;
}


int route_exists(char *A,char *B,char *path,int dist,char *cond, struct session *ses)
{
    int a,b;
    struct routenode *rptr;

    for (a=0;a<MAX_LOCATIONS;a++)
        if (ses->locations[a]&&!strcmp(ses->locations[a],A))
            break;
    if (a==MAX_LOCATIONS)
        return 0;
    for (b=0;b<MAX_LOCATIONS;b++)
        if (ses->locations[b]&&!strcmp(ses->locations[b],B))
            break;
    if (b==MAX_LOCATIONS)
        return 0;
    for (rptr=ses->routes[a];rptr;rptr=rptr->next)
        if ((rptr->dest==b)&&
                (!strcmp(rptr->path,path))&&
                (rptr->distance==dist)&&
                (!strcmp(rptr->cond,cond)))
            return 1;
    return 0;
}

/*****************************/
/* the #writesession command */
/*****************************/
void writesession_command(char *filename, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE], *val, num[32];
    struct listnode *nodeptr,*onptr;
    struct routenode *rptr;
    int nr;

    if (ses==nullsession)
    {
        write_command(filename,ses);
        return;
    }

    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, filename);
    substitute_myvars(filename, buffer, ses);
    expand_filename(buffer, filename);
    if (*filename == '\0')
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", filename);
        prompt(NULL);
        return;
    }
    if ((myfile = fopen(filename, "w")) == NULL)
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", filename);
        prompt(NULL);
        return;
    }

#undef SFLAG
#define SFLAG(name,var,dummy) WFLAG(name,ses->var,nullsession->var);
    SFLAG("echo", echo, DEFAULT_ECHO);
    SFLAG("ignore", ignore, DEFAULT_IGNORE);
    SFLAG("speedwalk", speedwalk, DEFAULT_SPEEDWALK);
    SFLAG("presub", presub, DEFAULT_PRESUB);
    SFLAG("togglesubs", togglesubs, DEFAULT_TOGGLESUBS);
    SFLAG("verbose", verbose, 0);
    SFLAG("blank", blank, DEFAULT_DISPLAY_BLANK);
    SFLAG("messages aliases", mesvar[0], DEFAULT_ALIAS_MESS);
    SFLAG("messages actions", mesvar[1], DEFAULT_ACTION_MESS);
    SFLAG("messages substitutes", mesvar[2], DEFAULT_SUB_MESS);
    SFLAG("messages events", mesvar[3], DEFAULT_EVENT_MESS);
    SFLAG("messages highlights", mesvar[4], DEFAULT_HIGHLIGHT_MESS);
    SFLAG("messages variables", mesvar[5], DEFAULT_VARIABLE_MESS);
    SFLAG("messages routes", mesvar[6], DEFAULT_ROUTE_MESS);
    SFLAG("messages gotos", mesvar[7], DEFAULT_GOTO_MESS);
    SFLAG("messages binds", mesvar[8], DEFAULT_BIND_MESS);
    SFLAG("messages #system", mesvar[9], DEFAULT_SYSTEM_MESS);
    SFLAG("messages paths", mesvar[10], DEFAULT_PATH_MESS);
    SFLAG("messages errors", mesvar[11], DEFAULT_ERROR_MESS);
    SFLAG("messages hooks", mesvar[12], DEFAULT_HOOK_MESS);
    SFLAG("verbatim", verbatim, 0);
    SFLAG("ticksize", tick_size, DEFAULT_TICK_SIZE);
    SFLAG("pretick", pretick, DEFAULT_PRETICK);
    
    nodeptr = onptr = hash2list(ses->aliases,"*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->aliases, nodeptr->left)))
            if (!strcmp(val,nodeptr->right))
                continue;
        prepare_for_write("alias", nodeptr->left, nodeptr->right, 0, buffer);
        fputs(buffer, myfile);
    }
    zap_list(onptr);

    nodeptr = ses->actions;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->actions, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right)||
                    !strcmp(onptr->right,nodeptr->right))
                continue;
        prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr,
                          buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->antisubs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->antisubs, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        prepare_for_write("antisubstitute", nodeptr->left, 0, 0, buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->subs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->subs, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        if (strcmp( nodeptr->right, EMPTY_LINE))
            prepare_for_write("sub", nodeptr->left, nodeptr->right, 0, buffer);
        else
            prepare_for_write("gag", nodeptr->left, 0, 0, buffer);
        fputs(buffer, myfile);
    }

    nodeptr = onptr = hash2list(ses->myvars,"*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->myvars, nodeptr->left)))
            if (!strcmp(val,nodeptr->right))
                continue;
        prepare_for_write("var", nodeptr->left, nodeptr->right, 0, buffer);
        fputs(buffer, myfile);
    }
    zap_list(onptr);

    nodeptr = ses->highs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->highs, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        prepare_for_write("highlight", nodeptr->right, nodeptr->left, 0, buffer);
        fputs(buffer, myfile);
    }
    
    nodeptr = onptr = hash2list(ses->pathdirs,"*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->pathdirs, nodeptr->left)))
            if (!strcmp(val,nodeptr->right))
                continue;
        prepare_for_write("pathdirs", nodeptr->left, nodeptr->right, 0, buffer);
        fputs(buffer, myfile);
    }
    zap_list(onptr);
    
    for (nr=0;nr<MAX_LOCATIONS;nr++)
        if ((rptr=ses->routes[nr]))
            do
            {
                if (!route_exists(ses->locations[nr],
                                  ses->locations[rptr->dest],
                                  rptr->path,
                                  rptr->distance,
                                  rptr->cond,
                                  nullsession))
                    fprintf(myfile,(*(rptr->cond))
                            ?"#route {%s} {%s} {%s} %d {%s}\n"
                            :"#route {%s} {%s} {%s} %d\n",
                            ses->locations[nr],
                            ses->locations[rptr->dest],
                            rptr->path,
                            rptr->distance,
                            rptr->cond);
            } while((rptr=rptr->next));
    
    nodeptr = onptr = hash2list(ses->binds,"*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->binds, nodeptr->left)))
            if (!strcmp(val,nodeptr->right))
                continue;
        prepare_for_write("bind", nodeptr->left, nodeptr->right, 0, buffer);
        fputs(buffer, myfile);
    }
    zap_list(onptr);
    
    for(nr=0;nr<NHOOKS;nr++)
        if (ses->hooks[nr])
            if (!nullsession->hooks[nr] ||
                strcmp(ses->hooks[nr],nullsession->hooks[nr]))
                {
                    prepare_for_write("hook", hook_names[nr], ses->hooks[nr], 0, buffer);
                    fputs(buffer, myfile);
                }

    fclose(myfile);
    tintin_printf(ses, "#COMMANDS-FILE WRITTEN.");
    return;
}


void prepare_for_write(char *command, char *left, char *right, char *pr, char *result)
{
    *result = tintin_char;
    *(result + 1) = '\0';
    strcat(result, command);
    strcat(result, " {");
    strcat(result, left);
    strcat(result, "}");
    if (right)
    {
        strcat(result, " {");
        strcat(result, right);
        strcat(result, "}");
    };
    if (pr && strlen(pr))
    {
        strcat(result, " {");
        strcat(result, pr);
        strcat(result, "}");
    }
    strcat(result, "\n");
}

void prepare_quotes(char *string)
{
    char s[BUFFER_SIZE], *cpsource, *cpdest;
    int nest = FALSE;

    strcpy(s, string);

    cpsource = s;
    cpdest = string;

    while (*cpsource)
    {
        if (*cpsource == '\\')
        {
            *cpdest++ = *cpsource++;
            if (*cpsource)
                *cpdest++ = *cpsource++;
        }
        else if (*cpsource == '\"' && nest == FALSE)
        {
            *cpdest++ = '\\';
            *cpdest++ = *cpsource++;
        }
        else if (*cpsource == '{')
        {
            nest = TRUE;
            *cpdest++ = *cpsource++;
        }
        else if (*cpsource == '}')
        {
            nest = FALSE;
            *cpdest++ = *cpsource++;
        }
        else
            *cpdest++ = *cpsource++;
    }
    *cpdest = '\0';
}

/**********************************/
/* load a file for input to mud.  */
/**********************************/
void textin_command(char *arg, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE], *cptr;

    get_arg_in_braces(arg, arg, 1);
    if (ses == nullsession)
    {
        tintin_eprintf(ses, "#You can't read any text in without a session being active.");
        prompt(NULL);
        return;
    }
    if ((myfile = fopen(arg, "r")) == NULL)
    {
        tintin_eprintf(ses, "ERROR: File {%s} doesn't exist.", arg);
        prompt(ses);
        return;
    }
    while (fgets(buffer, sizeof(buffer), myfile))
    {
        for (cptr = buffer; *cptr && *cptr != '\n'; cptr++) ;
        *cptr = '\0';
        write_line_mud(buffer, ses);
    }
    fclose(myfile);
    tintin_printf(ses,"#File read - Success.");
    prompt(ses);

}
