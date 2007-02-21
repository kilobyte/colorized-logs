/*********************************************************************/
/* file: main.c - main module - signal setup/shutdown etc            */
/*                             TINTIN++                              */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifndef HAVE_MEMCPY
# define memcpy(d, s, n) bcopy ((s), (d), (n))
# define memmove(d, s, n) bcopy ((s), (d), (n))
#endif
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
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include <stdarg.h>

#include <signal.h>
#include "tintin.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/resource.h>
#include <wchar.h>
#include "unicode.h"
#include "ui.h"

#ifndef BADSIG
#define BADSIG (void (*)())-1
#endif

typedef void (*sighandler_t)(int);

/*************** globals ******************/
int term_echoing = TRUE;
int keypad= DEFAULT_KEYPAD;
int retain= DEFAULT_RETAIN;
int puts_echoing = TRUE;
int alnum = 0;
int acnum = 0;
int subnum = 0;
int varnum = 0;
int hinum = 0;
int routnum = 0;
int pdnum = 0;
int antisubnum = 0;
int bindnum = 0;
int hooknum = 0;
int gotpassword=0;
int got_more_kludge=0;
int hist_num;
int need_resize=0;
extern int LINES,COLS;
extern int tty,xterm;
char *tintin_exec;
struct session *lastdraft;
int aborting=0;
int eofinput=0;
int any_closed=0;
extern int recursion;
char *_; /* incoming line being processed */
extern int o_lastcolor;
int real_quiet=0; /* if set, #verbose 0 will be really quiet */ 
char *history[HISTORY_SIZE];
struct session *sessionlist, *activesession, *nullsession;
char **pvars;	/* the %0, %1, %2,....%9 variables */
char tintin_char = DEFAULT_TINTIN_CHAR;
char verbatim_char = DEFAULT_VERBATIM_CHAR;
char prev_command[BUFFER_SIZE];
void tintin(void);
void read_mud(struct session *ses);
void do_one_line(char *line,int nl,struct session *ses);
void snoop(char *buffer, struct session *ses);
void tintin_puts(char *cptr, struct session *ses);
void tintin_puts1(char *cptr, struct session *ses);
void tintin_printf(struct session *ses, const char *format, ...);
void tintin_eprintf(struct session *ses, const char *format, ...);
char status[BUFFER_SIZE];

/************ externs *************/
extern char done_input[BUFFER_SIZE];

extern int ticker_interrupted, time0;
extern int tick_size, sec_to_tick;

static void myquitsig(int);
extern struct session *newactive_session(void);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern struct completenode *complete_head;
extern struct listnode *init_list(void);
extern void read_complete(void);
extern void syserr(char *msg, ...);
extern char* mystrdup(const char*);
extern int do_one_antisub(char *line, struct session *ses);
extern void prompt(struct session *ses);
extern int check_event(int time, struct session *ses);
extern void check_all_actions(char *line, struct session *ses);
extern void check_all_promptactions(char *line, struct session *ses);
extern void do_all_high(char *line,struct session *ses);
extern void do_all_sub(char *line, struct session *ses);
extern void do_in_MUD_colors(char *txt,int quotetype);
extern void init_bind(void);
extern int isnotblank(char *line,int flag);
extern int match(char *regex, char *string);
extern int iscompleteprompt(char *line);
void echo_input(char *txt);
extern struct hashtable* init_hash();
extern void init_parse();
extern struct session *do_read(FILE *myfile, char *filename, struct session *ses);
extern void do_history(char *buffer, struct session *ses);
extern int read_buffer_mud(char *buffer, struct session *ses);
extern void cleanup_session(struct session *ses);
void make_name(char *str, char *basis, int run);
extern void set_magic_hook(struct session *ses);
extern struct session* do_hook(struct session *ses, int t, char *data, int blockzap);
extern void flush_socket(struct session *ses);
extern void write_logf(struct session *ses, char *txt, char *prefix, char *suffix);
extern void write_log(struct session *ses, char *txt, int n);
extern void init_net();
extern void init_locale();
extern void end_command(char *arg, struct session *ses);
#ifdef PROFILING
extern char *prof_area;
extern time_t kbd_lag, mud_lag;
extern int kbd_cnt, mud_cnt;
extern void setup_prof();
#endif
extern int ui_sep_input;
extern struct listnode *init_list(void);
extern void addnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext);
extern void kill_list(struct listnode *nptr);
extern void user_setdriver(int dr);
extern void local_to_utf8(char *d, char *s, int maxb, mbstate_t *cs);
extern void nullify_conv(struct charset_conv *conv);
extern void convert(struct charset_conv *conv, char *outbuf, char *inbuf, int dir);

static void tstphandler(int sig)
{
    if (ui_own_output)
        user_pause();
    kill(getpid(), SIGSTOP);
}

static void sigchild(void)
{
    while (waitpid(-1,0,WNOHANG)>0);
}

static void sigcont(void)
{
    if (ui_own_output)
        user_resume();
}

static void sigsegv(void)
{
    if (ui_own_output)
        user_done();
    fflush(0);
/*  write(2,"Segmentation fault.\n",20);*/
    signal(SIGSEGV, SIG_DFL);
    raise(SIGSEGV);
/*  exit(SIGSEGV);*/
}

static void sigfpe(void)
{
    if (ui_own_output)
        user_done();
    fflush(0);
/*  write(2,"Floating point exception.\n",26);*/
    signal(SIGFPE, SIG_DFL);
    raise(SIGFPE);
/*  exit(SIGFPE);*/
}

static void sighup(void)
{
    fflush(0);
    signal(SIGHUP, SIG_DFL);
    raise(SIGHUP);
}

static void sigwinch(void)
{
    need_resize=1;
}


int new_news(void)
{
    struct stat KBtin, news;
    
    if (stat(tintin_exec, &KBtin))
        return 0;       /* can't stat the executable??? */
    if (stat(NEWS_FILE, &news))
#ifdef DATA_PATH
        if (stat(DATA_PATH "/" NEWS_FILE, &news))
#endif
            return 0;       /* either no NEWS file, or can't stat it */
    return (news.st_ctime>=news.st_atime)||(KBtin.st_ctime+10>news.st_atime);
}

/************************/
/* the #suspend command */
/************************/
void suspend_command(char *arg, struct session *ses)
{
    tstphandler(SIGTSTP);
}

void setup_signals(void)
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_RESTART;

    if (signal(SIGTERM, (sighandler_t)myquitsig) == BADSIG)
        syserr("signal SIGTERM");
    if (signal(SIGQUIT, (sighandler_t)myquitsig) == BADSIG)
        syserr("signal SIGQUIT");
    if (signal(SIGINT, (sighandler_t)myquitsig) == BADSIG)
        syserr("signal SIGINT");
    if (signal(SIGHUP, (sighandler_t)sighup) == BADSIG)
        syserr("signal SIGHUP");
    act.sa_handler=(sighandler_t)tstphandler;
    if (sigaction(SIGTSTP,&act,0))
        syserr("sigaction SIGTSTP");
    
    if (ui_own_output && ui_tty)
    {
        act.sa_handler=(sighandler_t)sigcont;
        if (sigaction(SIGCONT,&act,0))
            syserr("sigaction SIGCONT");
        act.sa_handler=(sighandler_t)sigwinch;
        if (sigaction(SIGWINCH,&act,0))
            syserr("sigaction SIGWINCH");
    }
        
    if (ui_own_output)
    {
        if (signal(SIGSEGV,(sighandler_t)sigsegv) == BADSIG)
            syserr("signal SIGSEGV");
        if (signal(SIGFPE,(sighandler_t)sigfpe) == BADSIG)
            syserr("signal SIGFPE");
    }
    
    act.sa_handler=(sighandler_t)sigchild;
    if (sigaction(SIGCHLD,&act,0))
        syserr("sigaction SIGCHLD");
    if (signal(SIGPIPE, SIG_IGN) == BADSIG)
        syserr("signal SIGPIPE");
}

/****************************************/
/* attempt to assure enough stack space */
/****************************************/
void setup_ulimit(void)
{
    struct rlimit rlim;

    if (getrlimit(RLIMIT_STACK,&rlim))
        return;
    if ((unsigned int)rlim.rlim_cur>=STACK_LIMIT)
        return;
    if (rlim.rlim_cur==rlim.rlim_max)
        return;
    if ((unsigned int)rlim.rlim_max<STACK_LIMIT)
        rlim.rlim_cur=rlim.rlim_max;
    else
        rlim.rlim_cur=STACK_LIMIT;
    setrlimit(RLIMIT_STACK,&rlim);
}

static void opterror(char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

static struct listnode *options;

void parse_options(int argc, char **argv, char **environ)
{
    int noargs=0;
    int arg;
    
    options=init_list();

    for (arg=1;arg<argc;arg++)
    {
        if (*argv[arg]=='-' && !noargs)
        {
            if (!strcmp(argv[arg],"--"))
                noargs=1;
            else if (!strcmp(argv[arg],"--version"))	/* make autotest happy */
            {
                printf("KBtin version "VERSION"\n");
                exit(0);
            }
            else if (!strcmp(argv[arg],"-v"))
                addnode_list(options, "#verbose 1", 0, 0);
            else if (!strcmp(argv[arg],"-q"))
                addnode_list(options, "#verbose 0", 0, 0);
            else if (!strcmp(argv[arg],"-p"))
                user_setdriver(0);
            else if (!strcmp(argv[arg],"-i"))
                user_setdriver(1);
            else if (!strcmp(argv[arg],"-c"))
            {
                if (++arg==argc)
                    opterror("Invalid option: bare -c");
                else
                    addnode_list(options, "c", argv[arg], 0);
            }
            else if (!strcmp(argv[arg],"-r"))
            {
                if (++arg==argc)
                    opterror("Invalid option: bare -r");
                else
                    addnode_list(options, "r", argv[arg], 0);
            }
            else if (!strcmp(argv[arg],"-s"))
            {
                if (++arg==argc)
                    opterror("Invalid option: bare -s");
                else if (++arg==argc)
                    opterror("Bad option: -s needs both an address and a port number!");
                else
                    addnode_list(options, "s", argv[arg-1], argv[arg]);
            }
            else
                opterror("Invalid option: {%s}",argv[arg]);
        }
        else
            addnode_list(options, " ", argv[arg], 0);
    }
    if (argc<=1)
        addnode_list(options, "-", 0, 0);
}

void apply_options()
{
    struct listnode *opt;
    char homepath[BUFFER_SIZE], temp[BUFFER_SIZE], sname[BUFFER_SIZE], *strptr;
    char ustr[BUFFER_SIZE];
    FILE *f;
#ifdef UTF8
# define DO_INPUT(str,iv) local_to_utf8(ustr,str,BUFFER_SIZE,0);\
                          activesession=parse_input(str,iv,activesession);
#else
# define DO_INPUT(str,iv) activesession=parse_input(str,iv,activesession);
#endif

    for(opt=options->next; opt; opt=opt->next)
    {
        switch(*opt->left)
        {
        case '#':
            *opt->left=tintin_char;
            activesession=parse_input(opt->left,1,activesession);
            break;
        case 'c':
            DO_INPUT(opt->right, 0);
            break;
        case 'r':
            set_magic_hook(activesession);
            make_name(sname, opt->right, 1);
            snprintf(temp, BUFFER_SIZE,
                "%crun %s {%s}", tintin_char, sname, opt->right);
            DO_INPUT(temp, 1);
            break;
        case 's':
            set_magic_hook(activesession);
            make_name(sname, opt->right, 1);
            snprintf(temp, BUFFER_SIZE,
                "%cses %s {%s %s}", tintin_char, sname, opt->right, opt->pr);
            DO_INPUT(temp, 1);
            break;
        case ' ':
#ifdef UTF8
            local_to_utf8(ustr, opt->right, BUFFER_SIZE, 0);
#else
# define ustr opt->right
#endif
            if ((f=fopen(opt->right,"r")))
            {
                if (activesession->verbose || !real_quiet)
                    tintin_printf(0, "#READING {%s}", ustr);
                activesession = do_read(f, ustr, activesession);
            }
            else
                tintin_eprintf(0, "#FILE NOT FOUND: {%s}", ustr);
#undef ustr
            break;
        case '-':
            *homepath = '\0';
            if (!strcmp(DEFAULT_FILE_DIR, "HOME"))
                if ((strptr = (char *)getenv("HOME")))
                    strcpy(homepath, strptr);
                else
                    *homepath = '\0';
            else
                strcpy(homepath, DEFAULT_FILE_DIR);
            
            strcpy(temp, homepath);
            strcat(temp, "/.tintinrc");
#ifdef UTF8
            local_to_utf8(ustr, temp, BUFFER_SIZE, 0);
#else
# define ustr temp
#endif
            if ((f=fopen(temp,"r")))
                activesession = do_read(f, ustr, activesession);
            else
            {
                if ((strptr = (char *)getenv("HOME")))
                {
                    strcpy(homepath, strptr);
                    strcpy(temp, homepath);
                    strcat(temp, "/.tintinrc");
#ifdef UTF8
                    local_to_utf8(ustr, temp, BUFFER_SIZE, 0);
#endif
                    if ((f=fopen(temp,"r")))
                        activesession = do_read(f, ustr, activesession);
#undef ustr
                }
            }
        }
    }
    
    kill_list(options);
}

/**************************************************************************/
/* main() - show title - setup signals - init lists - readcoms - tintin() */
/**************************************************************************/
int main(int argc, char **argv, char **environ)
{
    struct session *ses;

    tintin_exec=argv[0];
#ifdef UTF8
    init_locale();
#endif
    user_setdriver(isatty(0)?1:0);
    parse_options(argc, argv, environ);
    init_bind();
    hist_num=-1;
    init_parse();
    strcpy(status,EMPTY_LINE);
    user_init();
    /*  read_complete();		no tab-completion */
    ses = NULL;
    srand((getpid()*0x10001)^time(0));
    lastdraft=0;

    if (ui_own_output || tty)
    {
/*
    Legal crap does _not_ belong here.  Anyone interested in the license
can check the files accompanying KBtin, without any need of being spammed
every time.  It is not GNU bc or something similar, we don't want half
a screenful of all-uppercase (cAPS kEY IS STUCK AGAIN?) text that no one
ever wants to read -- that is what docs are for.
*/
    tintin_printf(0,"~2~##################################################");
    tintin_printf(0, "#~7~                ~12~K B ~3~t i n~7~     v %-15s ~2~#", VERSION);
    tintin_printf(0,"#~7~ current developer: ~9~Adam Borowski               ~2~#");
    tintin_printf(0,"#~7~                          (~9~kilobyte@angband.pl~7~) ~2~#");
    tintin_printf(0,"#~7~ based on ~12~tintin++~7~ v 2.1.9 by Peter Unold,      ~2~#");
    tintin_printf(0,"#~7~  Bill Reiss, David A. Wagner, Joann Ellsworth, ~2~#");
    tintin_printf(0,"#~7~     Jeremy C. Jack, Ulan@GrimneMUD and         ~2~#");
    tintin_printf(0,"#~7~  Jacek Narebski                                ~2~#");
    tintin_printf(0,"##################################################~7~");
    tintin_printf(0,"~15~#session <name> <host> <port> ~7~to connect to a remote server");
    tintin_printf(0,"                              ~8~#ses t2t towers.angband.com 9999");
    tintin_printf(0,"~15~#run <name> <command>         ~7~to run a local command");
    tintin_printf(0,"                              ~8~#run advent adventure");
    tintin_printf(0,"                              ~8~#run sql mysqlclient");
    tintin_printf(0,"~15~#help                         ~7~to get the help index");
    if (new_news())
        tintin_printf(ses,"Check #news now!");
    }

    init_net();
    setup_signals();
#ifdef PROFILING
    setup_prof();
#endif
    PROF("initializing");
    setup_ulimit();
    time0 = time(NULL);

    nullsession=(struct session *)malloc(sizeof(struct session));
    nullsession->name=mystrdup("main");
    nullsession->address=0;
    nullsession->tickstatus = FALSE;
    nullsession->tick_size = DEFAULT_TICK_SIZE;
    nullsession->pretick = DEFAULT_PRETICK;
    nullsession->time0 = 0;
    nullsession->snoopstatus = TRUE;
    nullsession->logfile = 0;
    nullsession->logname = 0;
    nullsession->logtype = DEFAULT_LOGTYPE;
    nullsession->blank = DEFAULT_DISPLAY_BLANK;
    nullsession->echo = ui_sep_input?DEFAULT_ECHO_SEPINPUT
                                    :DEFAULT_ECHO_NOSEPINPUT;
    nullsession->speedwalk = DEFAULT_SPEEDWALK;
    nullsession->togglesubs = DEFAULT_TOGGLESUBS;
    nullsession->presub = DEFAULT_PRESUB;
    nullsession->verbatim = 0;
    nullsession->ignore = DEFAULT_IGNORE;
    nullsession->aliases = init_hash();
    nullsession->actions = init_list();
    nullsession->prompts = init_list();
    nullsession->subs = init_list();
    nullsession->myvars = init_hash();
    nullsession->highs = init_list();
    nullsession->pathdirs = init_hash();
    nullsession->socket = 0;
    nullsession->issocket = 0;
    nullsession->naws = 0;
#ifdef HAVE_LIBZ
    nullsession->can_mccp = 0;
    nullsession->mccp = 0;
    nullsession->mccp_more = 0;
#endif
    nullsession->last_term_type=0;
    nullsession->server_echo = 0;
    nullsession->nagle = 0;
    nullsession->antisubs = init_list();
    nullsession->binds = init_hash();
    nullsession->next = 0;
    nullsession->sessionstart=nullsession->idle_since=time(0);
    nullsession->debuglogfile=0;
    nullsession->debuglogname=0;
    {
        int i;
        for (i=0;i<HISTORY_SIZE;i++)
            history[i]=0;
        for (i=0;i<MAX_LOCATIONS;i++)
        {
            nullsession->routes[i]=0;
            nullsession->locations[i]=0;
        }
        for(i=0;i<NHOOKS;i++)
            nullsession->hooks[i]=0;
    };
    nullsession->path = init_list();
    nullsession->no_return = 0;
    nullsession->path_length = 0;
    nullsession->last_line[0] = 0;
    nullsession->events = NULL;
    nullsession->verbose=0;
    nullsession->closing=0;
    sessionlist = nullsession;
    activesession = nullsession;
    pvars=0;

    nullsession->mesvar[0] = DEFAULT_ALIAS_MESS;
    nullsession->mesvar[1] = DEFAULT_ACTION_MESS;
    nullsession->mesvar[2] = DEFAULT_SUB_MESS;
    nullsession->mesvar[3] = DEFAULT_EVENT_MESS;
    nullsession->mesvar[4] = DEFAULT_HIGHLIGHT_MESS;
    nullsession->mesvar[5] = DEFAULT_VARIABLE_MESS;
    nullsession->mesvar[6] = DEFAULT_ROUTE_MESS;
    nullsession->mesvar[7] = DEFAULT_GOTO_MESS;
    nullsession->mesvar[8] = DEFAULT_BIND_MESS;
    nullsession->mesvar[9] = DEFAULT_SYSTEM_MESS;
    nullsession->mesvar[10]= DEFAULT_PATH_MESS;
    nullsession->mesvar[11]= DEFAULT_ERROR_MESS;
    nullsession->mesvar[12]= DEFAULT_HOOK_MESS;
#ifdef UTF8
    nullsession->charset=mystrdup(DEFAULT_CHARSET);
    nullsession->logcharset=logcs_is_special(DEFAULT_LOGCHARSET) ?
                              DEFAULT_LOGCHARSET : mystrdup(DEFAULT_LOGCHARSET);
    nullify_conv(&nullsession->c_io);
    nullify_conv(&nullsession->c_log);
#endif

    PROF("other");
    apply_options();
    tintin();
    return 0;
}

/******************************************************/
/* return seconds to next tick (global, all sessions) */
/* also display tick messages                         */
/******************************************************/
int check_events(void)
{
    struct session *sp;
    int tick_time = 0, curr_time, tt;

    curr_time = time(NULL);
    for (sp = sessionlist; sp; sp = sp->next)
    {
        tt = check_event(curr_time, sp);
        /* printf("#%s %d(%d)\n", sp->name, tt, curr_time); */
        if (tt > curr_time && (tick_time == 0 || tt < tick_time))
            tick_time = tt;
    }

    if (tick_time > curr_time)
        return (tick_time - curr_time);
    return (0);
}

/***************************/
/* the main loop of tintin */
/***************************/
void tintin(void)
{
    int i, result, maxfd;
    struct session *sesptr, *t;
    struct timeval tv;
    fd_set readfdmask;
#ifdef XTERM_TITLE
    struct session *lastsession=0;
#endif
    char kbdbuf[BUFFER_SIZE];
    WC ch;
    int inbuf=0;
#ifdef UTF8
    mbstate_t instate;
    
    memset(&instate, 0, sizeof(instate));
#endif

    while (TRUE)
    {
#ifdef XTERM_TITLE
        if (ui_own_output && activesession!=lastsession)
        {
            lastsession=activesession;
            if (activesession==nullsession)
                user_title(XTERM_TITLE, "(no session)");
            else
                user_title(XTERM_TITLE, activesession->name);
        }
#endif
        maxfd=0;
        FD_ZERO(&readfdmask);
        if (!eofinput)
            FD_SET(0, &readfdmask);
        else
            if (activesession==nullsession)
                end_command(0, activesession);
        for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
        {
            if (sesptr==nullsession)
                continue;
            if (sesptr->nagle)
                flush_socket(sesptr);
            FD_SET(sesptr->socket, &readfdmask);
            if (sesptr->socket>maxfd)
                maxfd=sesptr->socket;
        }

        tv.tv_sec = check_events();
        tv.tv_usec = 0;

        result = select(maxfd+1, &readfdmask, 0, 0, &tv);

        if (need_resize)
        {
            char buf[BUFFER_SIZE];
            
            user_resize();
            sprintf(buf, "#NEW SCREEN SIZE: %dx%d.", COLS, LINES);
            tintin_puts1(buf, activesession);
        }

        if (result == 0)
            continue;
        else if (result < 0 && errno == EINTR)
            continue;   /* Interrupted system call */
        else if (result < 0)
            syserr("select");

        if (FD_ISSET(0, &readfdmask))
        {
            PROFSTART;
            PROFPUSH("user interface");
            result=read(0, kbdbuf+inbuf, BUFFER_SIZE-inbuf);
            if (result==-1)
                myquitsig(0);
            if (result==0 && !isatty(0))
                eofinput=1;
            inbuf+=result;
            
            i=0;
            while(i<inbuf)
            {
#ifdef UTF8
                result=mbrtowc(&ch, kbdbuf+i, inbuf-i, &instate);
                if (result==-2)		/* incomplete but valid sequence */
                {
                    memmove(kbdbuf, kbdbuf+i, inbuf-i);
                    inbuf-=i;
                    goto partial;
                }
                else if (result==-1)	/* invalid sequence */
                {
                    ch=0xFFFD;
                    i++;
                    errno=0;
                    /* Shift by 1 byte.  We can use a more intelligent shift,
                     * but staying charset-agnostic makes the code simpler.
                     */
                }
                else if (result==0)	/* literal 0 */
                    i++; /* oops... bad ISO/ANSI, bad */
                else
                    i+=result;
#else
                ch=kbdbuf[i++];
#endif
                if (user_process_kbd(activesession, ch))
                {
                    hist_num=-1;
                    
                    if (term_echoing || (got_more_kludge && done_input[0]))
                        /* got_more_kludge: echo any non-empty line */
                    {
                        if (activesession && *done_input)
                            if (strcmp(done_input, prev_command))
                                do_history(done_input, activesession);
                        if (activesession->echo)
                            echo_input(done_input);
                        if (activesession->logfile)
                            write_logf(activesession, done_input, LOG_INPUT_PREFIX, LOG_INPUT_SUFFIX);
                    }
                    if (*done_input)
                        strcpy(prev_command, done_input);
                    aborting=0;
                    activesession = parse_input(done_input,0,activesession);
                    recursion=0;
                }
            }
            inbuf=0;
        partial:
            PROFEND(kbd_lag,kbd_cnt);
            PROFPOP;
        }
        for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
        {
            if (sesptr->socket && FD_ISSET(sesptr->socket,&readfdmask))
            {
                aborting=0;
                any_closed=0;
                do
                {
                    read_mud(sesptr);
                    if (any_closed)
                    {
                        any_closed=0;
                        goto after_read;
                        /* The remaining sessions will be done after select() */
                    }
#ifdef HAVE_LIBZ
                } while (sesptr->mccp_more);
#else
                } while (0);
#endif
            }
        }
    after_read:
        if (activesession->server_echo
            && (2-activesession->server_echo != gotpassword))
        {
            gotpassword= 2-activesession->server_echo;
            if (!gotpassword)
                got_more_kludge=0;
            user_passwd(gotpassword && !got_more_kludge);
            term_echoing=!gotpassword;
        }
    }
}


/*************************************************************/
/* read text from mud and test for actions/snoop/substitutes */
/*************************************************************/
void read_mud(struct session *ses)
{
    char buffer[BUFFER_SIZE], linebuffer[BUFFER_SIZE], *cpsource, *cpdest;
    char temp[BUFFER_SIZE];
    int didget, n, count;

    PROFSTART;
    if ((didget = read_buffer_mud(buffer, ses))==-1)
    {
        cleanup_session(ses);
        if (ses == activesession)
            activesession = newactive_session();
        return;
    }
    else
    {
        if (!didget)
            return; /* possible if only telnet protocol data was received */
    }
    if (ses->logfile)
    {
        if (ses->logtype)
        {
            count=0;
        
            if (ses->halfcr_log)
            {
                ses->halfcr_log=0;
                if (buffer[0]!='\n')
                    temp[count++]='\r';
            }
        
            for (n = 0; n < didget; n++)
                if (buffer[n] != '\r')
                    temp[count++] = buffer[n];
                else
                {
                    if (n+1==didget)
                        ses->halfcr_log=1;
                    else if (buffer[n+1]!='\n')
                        temp[count++]='\r';
                }
            temp[count]=0;	/* didget<BUFFER_SIZE, so no overflow */
            write_log(ses, temp, count);
        }
        else
        {
            buffer[didget]=0;
            write_log(ses, buffer, didget);
        }
    }

    cpsource = buffer;
    strcpy(linebuffer, ses->last_line);
    cpdest = strchr(linebuffer,'\0');

    if (ses->halfcr_in)
    {
        ses->halfcr_in=0;
        goto halfcr;
    }
    while (*cpsource)
    {		/*cut out each of the lines and process */
        if (*cpsource == '\n')
        {
            *cpdest = '\0';
            do_one_line(linebuffer,1,ses);

            cpsource++;
            cpdest=linebuffer;
        }
        else if (*cpsource=='\r')
        {
            cpsource++;
        halfcr:
            if (*cpsource=='\n')
                continue;
            if (!*cpsource)
            {
                ses->halfcr_in=1;
                break;
            }
            *cpdest=0;
            if (cpdest!=linebuffer)
                do_one_line(linebuffer,0,ses);
            cpdest=linebuffer;
        }
        else
            *cpdest++ = *cpsource++;
    }
    if (cpdest-linebuffer>INPUT_CHUNK) /* let's split too long lines */
    {
        *cpdest=0;
        do_one_line(linebuffer,1,ses);
        cpdest=linebuffer;
    }
    *cpdest = '\0';
    strcpy(ses->last_line,linebuffer);
    if (!ses->more_coming)
        if (cpdest!=linebuffer)
            do_one_line(linebuffer,0,ses);
    PROFEND(mud_lag, mud_cnt);
}
/**********************************************************/
/* do all of the functions to one line of buffer          */
/**********************************************************/
void do_one_line(char *line,int nl,struct session *ses)
{
    int isnb;
#ifdef UTF8
    char ubuf[BUFFER_SIZE];

    PROFPUSH("conv: remote->utf8");
    convert(&ses->c_io, ubuf, line, -1);
# define line ubuf
    PROF("looking for passwords");
#else
    PROFPUSH("looking for passwords");
#endif
    switch (ses->server_echo)
    {
    case 0:
        if ((match(PROMPT_FOR_PW_TEXT,line)
            || match(PROMPT_FOR_PW_TEXT2,line))
           && !gotpassword)
        {
            gotpassword=1;
            user_passwd(1);
            term_echoing=FALSE;
        };
        break;
    case 1:
        if (match(PROMPT_FOR_MORE_TEXT,line))
        {
            user_passwd(0);
            got_more_kludge=1;
        };
    };
    _=line;
    PROF("processing incoming colors");
    do_in_MUD_colors(line,0);
    isnb=isnotblank(line,0);
    PROF("promptactions");
    if (!ses->ignore && (nl||isnb))
        check_all_promptactions(line, ses);
    PROF("actions");
    if (nl && !ses->presub && !ses->ignore)
        check_all_actions(line, ses);
    PROF("substitutions");
    if (!ses->togglesubs && (nl||isnb) && !do_one_antisub(line, ses))
        do_all_sub(line, ses);
    PROF("actions");
    if (nl && ses->presub && !ses->ignore)
        check_all_actions(line, ses);
    PROF("highlights");
    if (isnb&&!ses->togglesubs)
        do_all_high(line, ses);
    PROF("display");
    if (isnotblank(line,ses->blank))
    {
        if (ses==activesession)
        {
            if (nl)
            {
                if (!activesession->server_echo)
                    gotpassword=0;
                sprintf(strchr(line,0),"\n");
                user_textout_draft(0, 0);
                user_textout(line);
                lastdraft=0;
            }
            else
            {
                if (ui_drafts)
                {
                    isnb = ses->gas ? ses->ga : iscompleteprompt(line);
#ifdef PARTIAL_LINE_MARKER
                    sprintf(strchr(line,0),PARTIAL_LINE_MARKER);
#endif
                    user_textout_draft(line, isnb);
                }
                lastdraft=ses;
            }
        }
        else
            if (ses->snoopstatus)
                snoop(line,ses);
    }
    PROFPOP;
    _=0;
#undef line
}

/**********************************************************/
/* snoop session ses - chop up lines and put'em in buffer */
/**********************************************************/
void snoop(char *buffer, struct session *ses)
{
    tintin_printf(0,"%s%% %s\n",ses->name,buffer);
}

/*****************************************************/
/* output to screen should go throught this function */
/* text gets checked for actions                     */
/*****************************************************/
void tintin_puts(char *cptr, struct session *ses)
{
    char line[BUFFER_SIZE];
    strcpy(line,cptr);
    if (ses)
    {
        _=line;
        check_all_actions(line, ses);
        _=0;
    }
    tintin_printf(ses,line);
}

/*****************************************************/
/* output to screen should go throught this function */
/* text gets checked for substitutes and actions     */
/*****************************************************/
void tintin_puts1(char *cptr, struct session *ses)
{
    char line[BUFFER_SIZE];

    strcpy(line,cptr);

    _=line;
    if (!ses->presub && !ses->ignore)
        check_all_actions(line, ses);
    if (!ses->togglesubs)
        if (!do_one_antisub(line, ses))
            do_all_sub(line, ses);
    if (ses->presub && !ses->ignore)
        check_all_actions(line, ses);
    if (!ses->togglesubs)
        do_all_high(line, ses);
    if (isnotblank(line,ses->blank))
        if (ses==activesession)
        {
            cptr=strchr(line,0);
            if (cptr-line>=BUFFER_SIZE-2)
                cptr=line+BUFFER_SIZE-2;
            cptr[0]='\n';
            cptr[1]=0;
            user_textout(line);
        }
    _=0;
}

void tintin_printf(struct session *ses, const char *format, ...)
{
    va_list ap;
#ifdef HAVE_VSNPRINTF
    char buf[BUFFER_SIZE];
#else
    char buf[BUFFER_SIZE*4]; /* let's hope this will never overflow... */
#endif

    if ((ses == activesession || ses == nullsession || !ses) && puts_echoing)
    {
        va_start(ap, format);
#ifdef HAVE_VSNPRINTF
        if (vsnprintf(buf, BUFFER_SIZE-1, format, ap)>BUFFER_SIZE-2)
            buf[BUFFER_SIZE-3]='>';
#else
        vsprintf(buf, format, ap);
#endif
        va_end(ap);
        strcat(buf, "\n");
        user_textout(buf);
    }
}

void tintin_eprintf(struct session *ses, const char *format, ...)
{
    va_list ap;
#ifdef HAVE_VSNPRINTF
    char buf[BUFFER_SIZE];
#else
    char buf[BUFFER_SIZE*4]; /* let's hope this will never overflow... */
#endif

    /* note: the behavior on !ses is wrong */
    if ((ses == activesession || ses == nullsession || !ses) && (puts_echoing||!ses||ses->mesvar[11]))
    {
        va_start(ap, format);
#ifdef HAVE_VSNPRINTF
        if (vsnprintf(buf, BUFFER_SIZE-1, format, ap)>BUFFER_SIZE-2)
            buf[BUFFER_SIZE-3]='>';
#else
        vsprintf(buf, format, ap);
#endif
        va_end(ap);
        strcat(buf, "\n");
        user_textout(buf);
    }
}

void echo_input(char *txt)
{
    char out[BUFFER_SIZE],*cptr,*optr;
    static int c=7;
    
    if (ui_own_output)
    {
        user_textout("");
        c=lastcolor;
    }
    
    optr=out;
    if (ui_own_output)
        optr+=sprintf(optr, ECHO_COLOR);
    cptr=txt;
    while (*cptr)
    {
        if ((*optr++=*cptr++)=='~')
            optr+=sprintf(optr, "~:~");
        if (optr-out > BUFFER_SIZE-10)
            break;
    }
    if (ui_own_output)
        optr+=sprintf(optr, "~%d~\n", c);
    else
    {
        *optr++='\n';
        *optr=0;
    }
    user_textout(out);
}

/**********************************************************/
/* Here's where we go when we wanna quit TINTIN FAAAAAAST */
/**********************************************************/
static void myquitsig(int sig)
{
    struct session *sesptr, *t;
    int err=errno;

    for (sesptr = sessionlist; sesptr; sesptr = t)
    {
        t = sesptr->next;
        if (sesptr!=nullsession && !sesptr->closing)
        {
            sesptr->closing=1;
            do_hook(sesptr, HOOK_ZAP, 0, 1);
            sesptr->closing=0;
            cleanup_session(sesptr);
        }
    }
    activesession = nullsession;
    do_hook(nullsession, HOOK_END, 0, 1);
    activesession = NULL;

    if (ui_own_output)
    {
        user_textout("~7~\n");
        switch(sig)
        {
        case SIGTERM:
            user_textout("Terminated\n");
            break;
        case SIGQUIT:
            user_textout("Quit\n");
            break;
        case SIGINT:
        default:
            break;
        case 0:
            user_textout(strerror(err));
        }
        user_done();
    }
    else
        if (tty)
            user_textout("~7~\n");
    exit(0);
}
