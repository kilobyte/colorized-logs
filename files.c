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
#include "tintin.h"

struct completenode *complete_head;
void prepare_for_write(char *command, char *left, char *right, char *pr, char *result);

extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern struct session *nullsession;
extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern void user_done(void);
extern struct listnode *searchnode_list(struct listnode *listhead,char *cptr);
extern void tintin_puts(char *cptr, struct session *ses);
extern void prompt(struct session *ses);
extern void char_command(char *arg,struct session *ses);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);
extern void tintin_printf(struct session *ses, char *format, ...);
extern void user_condump(FILE *f);
extern char *space_out(char *s);

extern int puts_echoing;
extern int alnum, acnum, subnum, hinum, varnum, antisubnum, routnum, bindnum;
extern int verbose;
extern char tintin_char;

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

/**********************************/
/* load a completion file         */
/**********************************/
void read_complete(void)
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
        tintin_puts("no tab.txt file, no completion list\n", (struct session *)NULL);
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
    tintin_puts("tab.txt file loaded.\n", (struct session *)NULL);
    prompt(NULL);
    tintin_puts("\n", (struct session *)NULL);

}

/*******************************/
/* remove file from filesystem */
/*******************************/
void unlink_file(char *arg, struct session *ses)
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
        tintin_puts("#unlink <filename> to remove it", ses);

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
            tintin_puts("#COULDN'T OPEN FILE.", ses);
    }
    else
        tintin_puts("#Syntax: #deathlog <file> <text>", ses);
}

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
            fh = popen(strcat(strcpy(temp,"gzip -9 >"),fname), "w");
        if (fh)
        {
            user_condump(fh);
            fclose(fh);
        }
        else
            tintin_puts("#COULDN'T OPEN FILE.", ses);
    }
    else
        tintin_puts("#Syntax: #condump <file>", ses);
    prompt(NULL);
}

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
                tintin_puts("#OK. LOGGING TURNED OFF.", ses);
            }
            get_arg_in_braces(arg, temp, 1);
            substitute_vars(temp, fname);
            substitute_myvars(fname, temp, ses);
            expand_filename(temp, fname);
            if ((strlen(fname)<4)||(strcmp(fname+strlen(fname)-3,".gz")))
                if ((ses->logfile = fopen(fname, "w")))
                    tintin_puts("#OK. LOGGING.....", ses);
                else
                    tintin_puts("#COULDN'T OPEN FILE.", ses);
            else
                if ((ses->logfile = popen(strcat(strcpy(temp,"gzip -9 >"),fname), "w")))
                    tintin_puts("#OK. LOGGING.....", ses);
                else
                    tintin_puts("#COULDN'T OPEN PIPE.", ses);
        }
        else if (ses->logfile)
        {
            fclose(ses->logfile);
            ses->logfile = NULL;
            tintin_puts("#OK. LOGGING TURNED OFF.", ses);
        }
        else
            tintin_puts("#LOGGING ALREADY OFF.", ses);
    }
    else
        tintin_puts("#THERE'S NO SESSION TO LOG.", ses);
    prompt(NULL);
}

/*********************/
/* the #read command */
/*********************/
struct session *read_command(char *filename, struct session *ses)
{
    FILE *myfile;
    char line[BUFFER_SIZE], buffer[BUFFER_SIZE], *cptr, *eptr;
    int flag,nl,ignore_lines;

    flag = !in_read;
    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, filename);
    substitute_myvars(filename, buffer, ses);
    expand_filename(buffer, filename);
    if ((myfile = fopen(filename, "r")) == NULL)
    {
        tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
        prompt(NULL);
        return ses;
    }
    if (!verbose)
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
            char_command(line, ses);
            if (!verbose)
                puts_echoing = FALSE;
            flag = FALSE;
        }
        for (cptr = line; *cptr && *cptr != '\n'; cptr++) ;
        *cptr = '\0';
        
        if (isspace(*line) && *buffer && (*buffer==tintin_char))
        {
            cptr=space_out(line);
            if (ignore_lines || (strlen(cptr)+strlen(buffer) >= BUFFER_SIZE/2))
            {
                puts_echoing=1;
                tintin_printf(ses,"#ERROR! LINE %d TOO LONG IN %s, TRUNCATING", nl, filename);
                *line=0;
                ignore_lines=1;
                puts_echoing=verbose;
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
        ignore_lines=0;
        strcpy(buffer, line);
    }
    if (*buffer)
        ses=parse_input(buffer,1,ses);
    in_read--;
    if (!verbose && !in_read)
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
    }
    fclose(myfile);
    prompt(NULL);
    return ses;
}

/**********************/
/* the #write command */
/**********************/
void write_command(char *filename, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE];
    struct listnode *nodeptr;
    struct routenode *rptr;
    int nr;

    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, filename);
    substitute_myvars(filename, buffer, ses);
    expand_filename(buffer, filename);
    if (*filename == '\0')
    {
        tintin_puts("#write <filename>", ses);
        prompt(NULL);
        return;
    }
    if ((myfile = fopen(filename, "w")) == NULL)
    {
        tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
        prompt(NULL);
        return;
    }
    nodeptr = ses->aliases;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("alias", nodeptr->left, nodeptr->right, "\0", buffer);
        fputs(buffer, myfile);
    }

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
        prepare_for_write("antisub", nodeptr->left, 0, "\0", buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->subs;
    while ((nodeptr = nodeptr->next))
    {
        if (strcmp( nodeptr->right, EMPTY_LINE))
            prepare_for_write("sub", nodeptr->left, nodeptr->right, "\0", buffer);
        else
            prepare_for_write("gag", nodeptr->left, 0, "\0", buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->myvars;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("var", nodeptr->left, nodeptr->right, "\0", buffer);
        fputs(buffer, myfile);
    }
    nodeptr = ses->highs;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
        fputs(buffer, myfile);
    }
    nodeptr = ses->pathdirs;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("pathdir", nodeptr->right, nodeptr->left, "\0", buffer);
        fputs(buffer, myfile);
    }
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
    nodeptr = ses->binds;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("bind", nodeptr->right, nodeptr->left, "\0", buffer);
        fputs(buffer, myfile);
    }
    fclose(myfile);
    tintin_puts("#COMMANDS-FILE WRITTEN.", ses);
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
    char buffer[BUFFER_SIZE];
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
        tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
        prompt(NULL);
        return;
    }
    if ((myfile = fopen(filename, "w")) == NULL)
    {
        tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
        prompt(NULL);
        return;
    }
    nodeptr = ses->aliases;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->aliases, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        prepare_for_write("alias", nodeptr->left, nodeptr->right, "\0", buffer);
        fputs(buffer, myfile);
    }

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
        prepare_for_write("antisubstitute", nodeptr->left, 0, "\0", buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->subs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->subs, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        if (strcmp( nodeptr->right, EMPTY_LINE))
            prepare_for_write("sub", nodeptr->left, nodeptr->right, "\0", buffer);
        else
            prepare_for_write("gag", nodeptr->left, 0, "\0", buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->myvars;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->myvars, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        prepare_for_write("var", nodeptr->left, nodeptr->right, "\0", buffer);
        fputs(buffer, myfile);
    }

    nodeptr = ses->highs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->highs, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
        fputs(buffer, myfile);
    }
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

    nodeptr = ses->binds;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->binds, nodeptr->left)))
            if (!strcmp(onptr->right,nodeptr->right))
                continue;
        prepare_for_write("bind", nodeptr->left, nodeptr->right, "\0", buffer);
        fputs(buffer, myfile);
    }

    fclose(myfile);
    tintin_puts("#COMMANDS-FILE WRITTEN.", ses);
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
    if (strlen(pr) != 0)
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
