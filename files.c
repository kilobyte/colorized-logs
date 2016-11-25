/*********************************************************************/
/* file: files.c - funtions for logfile and reading/writing comfiles */
/*                             TINTIN + +                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*                    New code by Bill Reiss 1993                    */
/*********************************************************************/
#include "tintin.h"
#include "protos/action.h"
#include "protos/globals.h"
#include "protos/hash.h"
#include "protos/hooks.h"
#include "protos/llist.h"
#include "protos/print.h"
#include "protos/net.h"
#include "protos/parse.h"
#include "protos/run.h"
#include "protos/unicode.h"
#include "protos/user.h"
#include "protos/utils.h"
#include "protos/variables.h"
#include <pwd.h>
#include <fcntl.h>

static void prepare_for_write(const char *command, const char *left, const char *right, const char *pr, char *result);
extern void char_command(const char *arg, struct session *ses);

/*******************************/
/* expand tildes in a filename */
/********************************************/
/* result must be at least BUFFER_SIZE long */
/********************************************/
static void expand_filename(const char *arg, char *result, char *lstr)
{
    char *r0=result;

    if (*arg=='~')
    {
        if (*(arg+1)=='/')
            result+=snprintf(result, BUFFER_SIZE, "%s", getenv("HOME")), arg++;
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
                    result+=snprintf(result, BUFFER_SIZE, "%s", pwd->pw_dir), arg=p;
            }
        }
    }
    strlcpy(result, arg, r0-result+BUFFER_SIZE);
    utf8_to_local(lstr, r0);
}

/****************************************/
/* convert charsets and write to a file */
/****************************************/
static void cfputs(const char *s, FILE *f)
{
    char lstr[BUFFER_SIZE*8];

    utf8_to_local(lstr, s);
    fputs(lstr, f);
}

static void cfprintf(FILE *f, const char *fmt, ...)
{
    char lstr[BUFFER_SIZE*8], buf[BUFFER_SIZE*4];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, BUFFER_SIZE*4, fmt, ap);
    va_end(ap);

    utf8_to_local(lstr, buf);
    fputs(lstr, f);
}

#if 0
/**********************************/
/* load a completion file         */
/**********************************/
void read_complete(const char *arg, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE];
    bool flag = true;
    struct completenode *tcomplete, *tcomp2;

    if ((complete_head = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL)
    {
        fprintf(stderr, "couldn't alloc completehead\n");
        user_done();
        exit(1);
    }
    tcomplete = complete_head;

    if ((myfile = fopen("tab.txt", "r")) == NULL)
    {
        if (const char *cptr = getenv("HOME"))
        {
            snprintf(buffer, BUFFER_SIZE, "%s/.tab.txt", cptr);
            myfile = fopen(buffer, "r");
        }
    }
    if (myfile == NULL)
    {
        tintin_eprintf(0, "no tab.txt file, no completion list");
        return;
    }
    while (fgets(buffer, sizeof(buffer), myfile))
    {
        char *cptr;
        for (cptr = buffer; *cptr && *cptr != '\n'; cptr++) ;
        *cptr = '\0';
        if ((tcomp2 = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL)
        {
            fprintf(stderr, "couldn't alloc completehead\n");
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
    tintin_printf(0, "tab.txt file loaded.");
    tintin_printf(0, "");

}
#endif

/*******************************/
/* remove file from filesystem */
/*******************************/
void unlink_command(const char *arg, struct session *ses)
{
    char file[BUFFER_SIZE], temp[BUFFER_SIZE], lstr[BUFFER_SIZE];

    if (*arg)
    {
        arg = get_arg_in_braces(arg, temp, 1);
        substitute_vars(temp, file);
        substitute_myvars(file, temp, ses);
        expand_filename(temp, file, lstr);
        unlink(lstr);
    }
    else
        tintin_eprintf(ses, "#ERROR: valid syntax is: #unlink <filename>");

}

/*************************/
/* the #deathlog command */
/*************************/
void deathlog_command(const char *arg, struct session *ses)
{
    FILE *fh;
    char fname[BUFFER_SIZE], text[BUFFER_SIZE], temp[BUFFER_SIZE], lfname[BUFFER_SIZE];

    if (*arg)
    {
        arg = get_arg_in_braces(arg, temp, 0);
        arg = get_arg_in_braces(arg, text, 1);
        substitute_vars(temp, fname);
        substitute_myvars(fname, temp, ses);
        expand_filename(temp, fname, lfname);
        substitute_vars(text, temp);
        substitute_myvars(temp, text, ses);
        if ((fh = fopen(lfname, "a")))
        {
            cfprintf(fh, "%s\n", text);
            fclose(fh);
        }
        else
            tintin_eprintf(ses, "#ERROR: COULDN'T OPEN FILE {%s}.", fname);
    }
    else
        tintin_eprintf(ses, "#ERROR: valid syntax is: #deathlog <file> <text>");
}

void log_off(struct session *ses)
{
    fclose(ses->logfile);
    ses->logfile = NULL;
    SFREE(ses->logname);
    ses->logname = NULL;
    cleanup_conv(&ses->c_log);
}

static inline void ttyrec_timestamp(struct ttyrec_header *th)
{
    struct timeval t;

    gettimeofday(&t, 0);
    th->sec=to_little_endian(t.tv_sec);
    th->usec=to_little_endian(t.tv_usec);
}

/* charset is always UTF-8 */
void write_logf(struct session *ses, const char *txt, const char *prefix, const char *suffix)
{
    char buf[BUFFER_SIZE*2], lbuf[BUFFER_SIZE*2];
    int len;

    sprintf(buf, "%s%s%s%s\n", prefix, txt, suffix, ses->logtype?"":"\r");
    if (ses->logtype==LOG_TTYREC)
    {
        ttyrec_timestamp((struct ttyrec_header *)lbuf);
        len=sizeof(struct ttyrec_header);
    }
    else
        len=0;
    convert(&ses->c_log, lbuf+len, buf, 1);
    len+=strlen(lbuf+len);
    if (ses->logtype==LOG_TTYREC)
    {
        uint32_t blen=len-sizeof(struct ttyrec_header);
        lbuf[ 8]=blen;
        lbuf[ 9]=blen>>8;
        lbuf[10]=blen>>16;
        lbuf[11]=blen>>24;
    }

    if ((int)fwrite(lbuf, 1, len, ses->logfile)<len)
    {
        log_off(ses);
        tintin_eprintf(ses, "#WRITE ERROR -- LOGGING DISABLED.  Disk full?");
    }
}

/* charset is always {remote} */
void write_log(struct session *ses, const char *txt, int n)
{
    struct ttyrec_header th;
    char ubuf[BUFFER_SIZE*2], lbuf[BUFFER_SIZE*2];

    if (ses->logcharset!=LOGCS_REMOTE && strcasecmp(user_charset_name, ses->charset))
    {
        convert(&ses->c_io, ubuf, txt, -1);
        convert(&ses->c_log, lbuf, ubuf, 1);
        n=strlen(lbuf);
        txt=lbuf;
    }
    if (ses->logtype==LOG_TTYREC)
    {
        ttyrec_timestamp(&th);
        th.len=to_little_endian(n);
        if (fwrite(&th, 1, sizeof(struct ttyrec_header), ses->logfile)<
            sizeof(struct ttyrec_header))
        {
            log_off(ses);
            tintin_eprintf(ses, "#WRITE ERROR -- LOGGING DISABLED.  Disk full?");
            return;
        }
    }

    if ((int)fwrite(txt, 1, n, ses->logfile)<n)
    {
        log_off(ses);
        tintin_eprintf(ses, "#WRITE ERROR -- LOGGING DISABLED.  Disk full?");
    }
}

/***************************/
/* the #logcomment command */
/***************************/
void logcomment_command(const char *arg, struct session *ses)
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
    write_logf(ses, text, "", "");
}

/*******************************/
/* the #loginputformat command */
/*******************************/
void loginputformat_command(const char *arg, struct session *ses)
{
    char text[BUFFER_SIZE];

    arg=get_arg(arg, text, 0, ses);
    SFREE(ses->loginputprefix);
    ses->loginputprefix = mystrdup(text);
    arg=get_arg(arg, text, 1, ses);
    SFREE(ses->loginputsuffix);
    ses->loginputsuffix = mystrdup(text);
    if (ses->mesvar[MSG_LOG])
        tintin_printf(ses, "#OK. INPUT LOG FORMAT NOW: %s%%0%s",
            ses->loginputprefix, ses->loginputsuffix);
}

static FILE* open_logfile(struct session *ses, const char *name, const char *filemsg, const char *appendmsg, const char *pipemsg)
{
    char fname[BUFFER_SIZE], lfname[BUFFER_SIZE];
    FILE *f;
    int len;

    if (*name=='|')
    {
        if (!*++name)
        {
            tintin_eprintf(ses, "#ERROR: {|} IS NOT A VALID PIPE.");
            return 0;
        }
        if ((f=mypopen(name, true, -1)))
        {
            if (ses->mesvar[MSG_LOG])
                tintin_printf(ses, pipemsg, name);
        }
        else
            tintin_eprintf(ses, "#ERROR: COULDN'T OPEN PIPE: {%s}.", name);
        return f;
    }
    if (*name=='>')
    {
        if (*++name=='>')
        {
            expand_filename(++name, fname, lfname);
            if ((f=fopen(lfname, "a")))
            {
                if (ses->mesvar[MSG_LOG])
                    tintin_printf(ses, appendmsg, fname);
            }
            else
                tintin_eprintf(ses, "ERROR: COULDN'T APPEND TO FILE: {%s}.", fname);
            return f;
        }
        expand_filename(name, fname, lfname);
        if ((f=fopen(lfname, "w")))
        {
            if (ses->mesvar[MSG_LOG])
                tintin_printf(ses, filemsg, fname);
        }
        else
            tintin_eprintf(ses, "ERROR: COULDN'T OPEN FILE: {%s}.", fname);
        return f;
    }
    expand_filename(name, fname, lfname);
    len=strlen(fname);
    const char *zip=0;
    if (len>=4 && !strcmp(fname+len-3, ".gz"))
        zip="gzip -9";
    else if (len>=5 && !strcmp(fname+len-4, ".bz2"))
        zip="bzip2";
    else if (len>=4 && !strcmp(fname+len-3, ".xz"))
        zip="xz";
    if (zip)
    {
        int fd=open(lfname, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, 0666);
        if (fd==-1)
        {
            tintin_eprintf(ses, "#ERROR: COULDN'T CREATE FILE {%s}: %s",
                           fname, strerror(errno));
            return 0;
        }

        if ((f = mypopen(zip, true, fd)))
        {
            if (ses->mesvar[MSG_LOG])
                tintin_printf(ses, filemsg, fname);
        }
        else
            tintin_eprintf(ses, "#ERROR: COULDN'T OPEN PIPE {%s >%s}.", zip, fname);
    }
    else if ((f = fopen(lfname, "w")))
        {
            if (ses->mesvar[MSG_LOG])
                tintin_printf(ses, filemsg, fname);
        }
        else
            tintin_eprintf(ses, "#ERROR: COULDN'T OPEN FILE {%s}.", fname);
    return f;
}

/************************/
/* the #condump command */
/************************/
void condump_command(const char *arg, struct session *ses)
{
    FILE *fh;
    char fname[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (!ui_con_buffer)
    {
        tintin_eprintf(ses, "#UI: No console buffer => can't dump it.");
        return;
    }
    if (*arg)
    {
        arg = get_arg_in_braces(arg, temp, 0);
        substitute_vars(temp, fname);
        substitute_myvars(fname, temp, ses);
        fh=open_logfile(ses, fname,
            "#DUMPING CONSOLE TO {%s}",
            "#APPENDING CONSOLE DUMP TO {%s}",
            "#PIPING CONSOLE DUMP TO {%s}");
        if (fh)
        {
            user_condump(fh);
            fclose(fh);
        }
    }
    else
        tintin_eprintf(ses, "#Syntax: #condump <file>");
}

/********************/
/* the #log command */
/********************/
void log_command(const char *arg, struct session *ses)
{
    char fname[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (ses!=nullsession)
    {
        if (*arg)
        {
            if (ses->logfile)
            {
                log_off(ses);
                if (ses->mesvar[MSG_LOG])
                    tintin_printf(ses, "#OK. LOGGING TURNED OFF.");
            }
            get_arg_in_braces(arg, temp, 1);
            substitute_vars(temp, fname);
            substitute_myvars(fname, temp, ses);
            ses->logfile=open_logfile(ses, temp,
                "#OK. LOGGING TO {%s} .....",
                "#OK. APPENDING LOG TO {%s} .....",
                "#OK. PIPING LOG TO {%s} .....");
            if (ses->logfile)
                ses->logname=mystrdup(temp);
            if (!new_conv(&ses->c_log, logcs_charset(ses->logcharset), 1))
                tintin_eprintf(ses, "#Warning: can't open charset: %s",
                                    logcs_charset(ses->logcharset));

        }
        else if (ses->logfile)
        {
            log_off(ses);
            if (ses->mesvar[MSG_LOG])
                tintin_printf(ses, "#OK. LOGGING TURNED OFF.");
        }
        else if (ses->mesvar[MSG_LOG])
            tintin_printf(ses, "#LOGGING ALREADY OFF.");
    }
    else
        tintin_eprintf(ses, "#THERE'S NO SESSION TO LOG.");
}

/*************************/
/* the #debuglog command */
/*************************/
void debuglog_command(const char *arg, struct session *ses)
{
    char fname[BUFFER_SIZE], temp[BUFFER_SIZE];

    if (*arg)
    {
        if (ses->debuglogfile)
        {
            fclose(ses->debuglogfile);
            if (ses->mesvar[MSG_LOG])
                tintin_printf(ses, "#OK. DEBUGLOG TURNED OFF.");
            ses->debuglogfile = NULL;
            SFREE(ses->debuglogname);
            ses->debuglogname = NULL;
        }
        get_arg_in_braces(arg, temp, 1);
        substitute_vars(temp, fname);
        substitute_myvars(fname, temp, ses);
            ses->debuglogfile=open_logfile(ses, temp,
                "#OK. DEBUGLOG SET TO {%s} .....",
                "#OK. DEBUGLOG APPENDING TO {%s} .....",
                "#OK. DEBUGLOG PIPED TO {%s} .....");
        if (ses->debuglogfile)
            ses->debuglogname=mystrdup(temp);
    }
    else if (ses->debuglogfile)
    {
        fclose(ses->debuglogfile);
        ses->debuglogfile = NULL;
        SFREE(ses->debuglogname);
        ses->debuglogname = NULL;
        if (ses->mesvar[MSG_LOG])
            tintin_printf(ses, "#OK. DEBUGLOG TURNED OFF.");
    }
    else
        tintin_printf(ses, "#DEBUGLOG ALREADY OFF.");
}

void debuglog(struct session *ses, const char *format, ...)
{
    va_list ap;
    char buf[BUFFER_SIZE];
    struct timeval tv;

    if (!ses->debuglogfile)
        return;

    gettimeofday(&tv, 0);
    va_start(ap, format);
    if (vsnprintf(buf, BUFFER_SIZE-1, format, ap)>BUFFER_SIZE-2)
        buf[BUFFER_SIZE-3]='>';
    va_end(ap);
    cfprintf(ses->debuglogfile, "%4ld.%06d: %s\n",
        (long int)tv.tv_sec-ses->sessionstart, (int)tv.tv_usec, buf);
}


struct session* do_read(FILE *myfile, const char *filename, struct session *ses)
{
    char line[BUFFER_SIZE], buffer[BUFFER_SIZE], lstr[BUFFER_SIZE], *cptr, *eptr;
    bool flag, ignore_lines;
    int nl;
    mbstate_t cs;

    memset(&cs, 0, sizeof(cs));

    flag = !in_read;
    if (!ses->verbose)
        puts_echoing = false;
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
    ignore_lines=false;
    nl=0;
    while (fgets(lstr, BUFFER_SIZE, myfile))
    {
        local_to_utf8(line, lstr, BUFFER_SIZE, &cs);
        if (!nl++)
            if (line[0]=='#' && line[1]=='!') /* Unix hashbang script */
                continue;
        if (flag)
        {
            puts_echoing = ses->verbose || !real_quiet;
            if (is7punct(*line))
                char_command(line, ses);
            if (!ses->verbose)
                puts_echoing = false;
            flag = false;
        }
        for (cptr = line; *cptr && *cptr != '\n' && *cptr!='\r'; cptr++) ;
        *cptr = '\0';

        if (isaspace(*line) && *buffer && (*buffer==tintin_char))
        {
            cptr=(char*)space_out(line);
            if (ignore_lines || (strlen(cptr)+strlen(buffer) >= BUFFER_SIZE/2))
            {
                puts_echoing=true;
                tintin_eprintf(ses, "#ERROR! LINE %d TOO LONG IN %s, TRUNCATING", nl, filename);
                *line=0;
                ignore_lines=true;
                puts_echoing=ses->verbose;
            }
            else
            {
                if (*cptr &&
                    !isspace(*(eptr=strchr(buffer, 0)-1)) &&
                    (*eptr!=';') && (*cptr!=BRACE_CLOSE) &&
                    (*cptr!=tintin_char || *eptr!=BRACE_OPEN))
                    strcat(buffer, " ");
                strcat(buffer, cptr);
                continue;
            }
        }
        ses = parse_input(buffer, true, ses);
        recursion=0;
        ignore_lines=false;
        strcpy(buffer, line);
    }
    if (*buffer)
    {
        ses=parse_input(buffer, true, ses);
        recursion=0;
    }
    in_read--;
    if (!in_read)
        puts_echoing = true;
    if (!ses->verbose && !in_read && !real_quiet)
    {
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
    return ses;
}


/*********************/
/* the #read command */
/*********************/
struct session* read_command(const char *filename, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE], fname[BUFFER_SIZE], lfname[BUFFER_SIZE];

    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, fname);
    substitute_myvars(fname, buffer, ses);
    expand_filename(buffer, fname, lfname);
    if (!*filename)
    {
        tintin_eprintf(ses, "#Syntax: #read filename");
        return ses;
    }
    if ((myfile = fopen(lfname, "r")) == NULL)
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", fname);
        return ses;
    }

    return do_read(myfile, fname, ses);
}


#define WFLAG(name, var, org)   if (var!=(org))                                 \
                                {                                               \
                                    sprintf(num, "%d", var);                    \
                                    prepare_for_write(name, num, 0, 0, buffer); \
                                    cfputs(buffer, myfile);                     \
                                }
#define SFLAG(name, var, org)   WFLAG(name, ses->var, org)
/**********************/
/* the #write command */
/**********************/
void write_command(const char *filename, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE*4], num[32], fname[BUFFER_SIZE], lfname[BUFFER_SIZE];
    struct listnode *nodeptr, *templist;
    struct routenode *rptr;

    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, fname);
    substitute_myvars(fname, buffer, ses);
    expand_filename(buffer, fname, lfname);
    if (*filename == '\0')
    {
        tintin_eprintf(ses, "#ERROR: syntax is: #write <filename>");
        return;
    }
    if ((myfile = fopen(lfname, "w")) == NULL)
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", fname);
        return;
    }

    WFLAG("keypad", keypad, DEFAULT_KEYPAD);
    WFLAG("retain", retain, DEFAULT_RETAIN);
    SFLAG("echo", echo, ui_sep_input?DEFAULT_ECHO_SEPINPUT
                                    :DEFAULT_ECHO_NOSEPINPUT);
    SFLAG("ignore", ignore, DEFAULT_IGNORE);
    SFLAG("speedwalk", speedwalk, DEFAULT_SPEEDWALK);
    SFLAG("presub", presub, DEFAULT_PRESUB);
    SFLAG("togglesubs", togglesubs, DEFAULT_TOGGLESUBS);
    SFLAG("verbose", verbose, false);
    SFLAG("blank", blank, DEFAULT_DISPLAY_BLANK);
    SFLAG("messages aliases", mesvar[MSG_ALIAS], DEFAULT_ALIAS_MESS);
    SFLAG("messages actions", mesvar[MSG_ACTION], DEFAULT_ACTION_MESS);
    SFLAG("messages substitutes", mesvar[MSG_SUBSTITUTE], DEFAULT_SUB_MESS);
    SFLAG("messages events", mesvar[MSG_EVENT], DEFAULT_EVENT_MESS);
    SFLAG("messages highlights", mesvar[MSG_HIGHLIGHT], DEFAULT_HIGHLIGHT_MESS);
    SFLAG("messages variables", mesvar[MSG_VARIABLE], DEFAULT_VARIABLE_MESS);
    SFLAG("messages routes", mesvar[MSG_ROUTE], DEFAULT_ROUTE_MESS);
    SFLAG("messages gotos", mesvar[MSG_GOTO], DEFAULT_GOTO_MESS);
    SFLAG("messages binds", mesvar[MSG_BIND], DEFAULT_BIND_MESS);
    SFLAG("messages #system", mesvar[MSG_SYSTEM], DEFAULT_SYSTEM_MESS);
    SFLAG("messages paths", mesvar[MSG_PATH], DEFAULT_PATH_MESS);
    SFLAG("messages errors", mesvar[MSG_ERROR], DEFAULT_ERROR_MESS);
    SFLAG("messages hooks", mesvar[MSG_HOOK], DEFAULT_HOOK_MESS);
    SFLAG("verbatim", verbatim, false);
    SFLAG("ticksize", tick_size, DEFAULT_TICK_SIZE);
    SFLAG("pretick", pretick, DEFAULT_PRETICK);
    if (strcmp(DEFAULT_CHARSET, ses->charset))
        cfprintf(myfile, "%ccharset {%s}\n", tintin_char, ses->charset);
    if (strcmp(logcs_name(DEFAULT_LOGCHARSET), logcs_name(ses->logcharset)))
        cfprintf(myfile, "%clogcharset {%s}\n", tintin_char, logcs_name(ses->logcharset));
    nodeptr = templist = hash2list(ses->aliases, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("alias", nodeptr->left, nodeptr->right, 0, buffer);
        cfputs(buffer, myfile);
    }
    zap_list(templist);

    nodeptr = ses->actions;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr,
                          buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = ses->antisubs;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("antisub", nodeptr->left, 0, 0, buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = ses->subs;
    while ((nodeptr = nodeptr->next))
    {
        if (strcmp( nodeptr->right, EMPTY_LINE))
            prepare_for_write("sub", nodeptr->left, nodeptr->right, 0, buffer);
        else
            prepare_for_write("gag", nodeptr->left, 0, 0, buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = templist = hash2list(ses->myvars, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("var", nodeptr->left, nodeptr->right, "\0", buffer);
        cfputs(buffer, myfile);
    }
    zap_list(templist);

    nodeptr = ses->highs;
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = templist = hash2list(ses->pathdirs, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("pathdir", nodeptr->left, nodeptr->right, 0, buffer);
        cfputs(buffer, myfile);
    }
    zap_list(templist);

    for (int nr=0;nr<MAX_LOCATIONS;nr++)
        if ((rptr=ses->routes[nr]))
            do
            {
                cfprintf(myfile, (*(rptr->cond))
                        ?"%croute {%s} {%s} {%s} %d {%s}\n"
                        :"%croute {%s} {%s} {%s} %d\n",
                        tintin_char,
                        ses->locations[nr],
                        ses->locations[rptr->dest],
                        rptr->path,
                        rptr->distance,
                        rptr->cond);
            } while ((rptr=rptr->next));

    nodeptr = templist = hash2list(ses->binds, "*");
    while ((nodeptr = nodeptr->next))
    {
        prepare_for_write("bind", nodeptr->left, nodeptr->right, 0, buffer);
        cfputs(buffer, myfile);
    }
    zap_list(templist);

    for (int nr=0;nr<NHOOKS;nr++)
        if (ses->hooks[nr])
        {
            prepare_for_write("hook", hook_names[nr], ses->hooks[nr], 0, buffer);
            cfputs(buffer, myfile);
        }

    fclose(myfile);
    tintin_printf(ses, "#COMMANDS-FILE WRITTEN.");
    return;
}


static bool route_exists(const char *A, const char *B, const char *path, int dist, const char *cond, struct session *ses)
{
    int a, b;

    for (a=0;a<MAX_LOCATIONS;a++)
        if (ses->locations[a]&&!strcmp(ses->locations[a], A))
            break;
    if (a==MAX_LOCATIONS)
        return false;
    for (b=0;b<MAX_LOCATIONS;b++)
        if (ses->locations[b]&&!strcmp(ses->locations[b], B))
            break;
    if (b==MAX_LOCATIONS)
        return false;
    for (struct routenode *rptr=ses->routes[a];rptr;rptr=rptr->next)
        if ((rptr->dest==b)&&
                (!strcmp(rptr->path, path))&&
                (rptr->distance==dist)&&
                (!strcmp(rptr->cond, cond)))
            return true;
    return false;
}

/*****************************/
/* the #writesession command */
/*****************************/
void writesession_command(const char *filename, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE*4], *val, num[32], fname[BUFFER_SIZE], lfname[BUFFER_SIZE];
    struct listnode *nodeptr, *onptr;
    struct routenode *rptr;

    if (ses==nullsession)
    {
        write_command(filename, ses);
        return;
    }

    get_arg_in_braces(filename, buffer, 1);
    substitute_vars(buffer, fname);
    substitute_myvars(fname, buffer, ses);
    expand_filename(buffer, fname, lfname);
    if (*filename == '\0')
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", fname);
        return;
    }
    if ((myfile = fopen(lfname, "w")) == NULL)
    {
        tintin_eprintf(ses, "#ERROR - COULDN'T OPEN FILE {%s}.", fname);
        return;
    }

#undef SFLAG
#define SFLAG(name, var, dummy) WFLAG(name, ses->var, nullsession->var);
    SFLAG("echo", echo, DEFAULT_ECHO);
    SFLAG("ignore", ignore, DEFAULT_IGNORE);
    SFLAG("speedwalk", speedwalk, DEFAULT_SPEEDWALK);
    SFLAG("presub", presub, DEFAULT_PRESUB);
    SFLAG("togglesubs", togglesubs, DEFAULT_TOGGLESUBS);
    SFLAG("verbose", verbose, false);
    SFLAG("blank", blank, DEFAULT_DISPLAY_BLANK);
    SFLAG("messages aliases", mesvar[MSG_ALIAS], DEFAULT_ALIAS_MESS);
    SFLAG("messages actions", mesvar[MSG_ACTION], DEFAULT_ACTION_MESS);
    SFLAG("messages substitutes", mesvar[MSG_SUBSTITUTE], DEFAULT_SUB_MESS);
    SFLAG("messages events", mesvar[MSG_EVENT], DEFAULT_EVENT_MESS);
    SFLAG("messages highlights", mesvar[MSG_HIGHLIGHT], DEFAULT_HIGHLIGHT_MESS);
    SFLAG("messages variables", mesvar[MSG_VARIABLE], DEFAULT_VARIABLE_MESS);
    SFLAG("messages routes", mesvar[MSG_ROUTE], DEFAULT_ROUTE_MESS);
    SFLAG("messages gotos", mesvar[MSG_GOTO], DEFAULT_GOTO_MESS);
    SFLAG("messages binds", mesvar[MSG_BIND], DEFAULT_BIND_MESS);
    SFLAG("messages #system", mesvar[MSG_SYSTEM], DEFAULT_SYSTEM_MESS);
    SFLAG("messages paths", mesvar[MSG_PATH], DEFAULT_PATH_MESS);
    SFLAG("messages errors", mesvar[MSG_ERROR], DEFAULT_ERROR_MESS);
    SFLAG("messages hooks", mesvar[MSG_HOOK], DEFAULT_HOOK_MESS);
    SFLAG("verbatim", verbatim, false);
    SFLAG("ticksize", tick_size, DEFAULT_TICK_SIZE);
    SFLAG("pretick", pretick, DEFAULT_PRETICK);
    if (strcmp(nullsession->charset, ses->charset))
        cfprintf(myfile, "%ccharset {%s}\n", tintin_char, ses->charset);
    if (strcmp(logcs_name(nullsession->logcharset), logcs_name(ses->logcharset)))
        cfprintf(myfile, "%clogcharset {%s}\n", tintin_char, logcs_name(ses->logcharset));

    nodeptr = onptr = hash2list(ses->aliases, "*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->aliases, nodeptr->left)))
            if (!strcmp(val, nodeptr->right))
                continue;
        prepare_for_write("alias", nodeptr->left, nodeptr->right, 0, buffer);
        cfputs(buffer, myfile);
    }
    zap_list(onptr);

    nodeptr = ses->actions;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->actions, nodeptr->left)))
            if (!strcmp(onptr->right, nodeptr->right)||
                    !strcmp(onptr->right, nodeptr->right))
                continue;
        prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr,
                          buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = ses->antisubs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->antisubs, nodeptr->left)))
            if (!strcmp(onptr->right, nodeptr->right))
                continue;
        prepare_for_write("antisubstitute", nodeptr->left, 0, 0, buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = ses->subs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->subs, nodeptr->left)))
            if (!strcmp(onptr->right, nodeptr->right))
                continue;
        if (strcmp( nodeptr->right, EMPTY_LINE))
            prepare_for_write("sub", nodeptr->left, nodeptr->right, 0, buffer);
        else
            prepare_for_write("gag", nodeptr->left, 0, 0, buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = onptr = hash2list(ses->myvars, "*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->myvars, nodeptr->left)))
            if (!strcmp(val, nodeptr->right))
                continue;
        prepare_for_write("var", nodeptr->left, nodeptr->right, 0, buffer);
        cfputs(buffer, myfile);
    }
    zap_list(onptr);

    nodeptr = ses->highs;
    while ((nodeptr = nodeptr->next))
    {
        if ((onptr=searchnode_list(nullsession->highs, nodeptr->left)))
            if (!strcmp(onptr->right, nodeptr->right))
                continue;
        prepare_for_write("highlight", nodeptr->right, nodeptr->left, 0, buffer);
        cfputs(buffer, myfile);
    }

    nodeptr = onptr = hash2list(ses->pathdirs, "*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->pathdirs, nodeptr->left)))
            if (!strcmp(val, nodeptr->right))
                continue;
        prepare_for_write("pathdirs", nodeptr->left, nodeptr->right, 0, buffer);
        cfputs(buffer, myfile);
    }
    zap_list(onptr);

    for (int nr=0;nr<MAX_LOCATIONS;nr++)
        if ((rptr=ses->routes[nr]))
            do
            {
                if (!route_exists(ses->locations[nr],
                                  ses->locations[rptr->dest],
                                  rptr->path,
                                  rptr->distance,
                                  rptr->cond,
                                  nullsession))
                    cfprintf(myfile, (*(rptr->cond))
                            ?"%croute {%s} {%s} {%s} %d {%s}\n"
                            :"%croute {%s} {%s} {%s} %d\n",
                            tintin_char,
                            ses->locations[nr],
                            ses->locations[rptr->dest],
                            rptr->path,
                            rptr->distance,
                            rptr->cond);
            } while ((rptr=rptr->next));

    nodeptr = onptr = hash2list(ses->binds, "*");
    while ((nodeptr = nodeptr->next))
    {
        if ((val=get_hash(nullsession->binds, nodeptr->left)))
            if (!strcmp(val, nodeptr->right))
                continue;
        prepare_for_write("bind", nodeptr->left, nodeptr->right, 0, buffer);
        cfputs(buffer, myfile);
    }
    zap_list(onptr);

    for (int nr=0;nr<NHOOKS;nr++)
        if (ses->hooks[nr])
            if (!nullsession->hooks[nr] ||
                strcmp(ses->hooks[nr], nullsession->hooks[nr]))
                {
                    prepare_for_write("hook", hook_names[nr], ses->hooks[nr], 0, buffer);
                    cfputs(buffer, myfile);
                }

    fclose(myfile);
    tintin_printf(ses, "#COMMANDS-FILE WRITTEN.");
    return;
}


static void prepare_for_write(const char *command, const char *left, const char *right, const char *pr, char *result)
{
    /* "result" is four times as big as the regular buffer.  This is */
    /* pointless as read line are capped at 1/2 buffer anyway.       */
    result+=sprintf(result, "%c%s {%s}", tintin_char, command, left);
    if (right)
        result+=sprintf(result, " {%s}", right);
    if (pr && strlen(pr))
        result+=sprintf(result, " {%s}", pr);
    sprintf(result, "\n");
}


/**********************************/
/* load a file for input to mud.  */
/**********************************/
void textin_command(const char *arg, struct session *ses)
{
    FILE *myfile;
    char buffer[BUFFER_SIZE], filename[BUFFER_SIZE], *cptr, lfname[BUFFER_SIZE];
    mbstate_t cs;

    memset(&cs, 0, sizeof(cs));

    get_arg_in_braces(arg, buffer, 1);
    substitute_vars(buffer, filename);
    substitute_myvars(filename, buffer, ses);
    expand_filename(buffer, filename, lfname);
    if (ses == nullsession)
    {
        tintin_eprintf(ses, "#You can't read any text in without a session being active.");
        return;
    }
    if ((myfile = fopen(lfname, "r")) == NULL)
    {
        tintin_eprintf(ses, "ERROR: File {%s} doesn't exist.", filename);
        return;
    }
    while (fgets(buffer, sizeof(buffer), myfile))
    {
        for (cptr = buffer; *cptr && *cptr != '\n'; cptr++) ;
        *cptr = '\0';
        local_to_utf8(lfname, buffer, BUFFER_SIZE, &cs);
        write_line_mud(lfname, ses);
    }
    fclose(myfile);
    tintin_printf(ses, "#File read - Success.");
}

const char *logtypes[]=
{
    "raw",
    "lf",
    "ttyrec",
};

/************************/
/* the #logtype command */
/************************/
void logtype_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];

    arg=get_arg(arg, left, 1, ses);
    if (!*left)
    {
        tintin_printf(ses, "#The log type is: %s", logtypes[ses->logtype]);
        return;
    }
    for (unsigned t=0;t<sizeof(logtypes)/sizeof(char*);t++)
        if (is_abrev(left, logtypes[t]))
        {
            ses->logtype=t;
            if (ses->mesvar[MSG_LOG])
                tintin_printf(ses, "#Ok, log type is now %s.", logtypes[t]);
            return;
        }
    tintin_eprintf(ses, "#No such logtype: {%s}\n", left);
}

/***************************/
/* the #logcharset command */
/***************************/
void logcharset_command(const char *arg, struct session *ses)
{
    char what[BUFFER_SIZE], *cset;
    struct charset_conv nc;

    get_arg(arg, what, 1, ses);
    cset=what;

    if (!*cset)
    {
        tintin_printf(ses, "#Log charset: %s", logcs_name(ses->logcharset));
        return;
    }
    if (!strcasecmp(cset, "local"))
        cset=LOGCS_LOCAL;
    else if (!strcasecmp(cset, "remote"))
        cset=LOGCS_REMOTE;
    if (!new_conv(&nc, logcs_charset(cset), 1))
    {
        tintin_eprintf(ses, "#No such charset: {%s}", logcs_charset(cset));
        return;
    }
    if (!logcs_is_special(ses->logcharset))
        SFREE(ses->logcharset);
    ses->logcharset=logcs_is_special(cset) ? cset : mystrdup(cset);
    if (ses!=nullsession && ses->logfile)
    {
        cleanup_conv(&ses->c_log);
        memcpy(&ses->c_log, &nc, sizeof(nc));
    }
    else
        cleanup_conv(&nc);
    if (ses->mesvar[MSG_LOG])
        tintin_printf(ses, "#Log charset set to %s", logcs_name(cset));
}
