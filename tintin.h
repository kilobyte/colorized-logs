/******************************************************************/
/* file: tintin.h - the include file for KBtin                    */
/******************************************************************/
#undef TELNET_DEBUG     /* define to show TELNET negotiations */
#undef USER_DEBUG       /* debugging of the user interface */
#undef TERM_DEBUG       /* debugging pseudo-tty stuff */
#undef PROFILING        /* profiling */

#include <stdbool.h>

/**********************/
/* color ANSI numbers */
/**********************/
#define COLOR_BLACK     0
#define COLOR_BLUE      4
#define COLOR_GREEN     2
#define COLOR_CYAN      6
#define COLOR_RED       1
#define COLOR_MAGENTA   5
#define COLOR_YELLOW    3
#define COLOR_WHITE     7

#define LOGCS_LOCAL     ((char*)1)
#define LOGCS_REMOTE    ((char*)2)


/*************************/
/* telnet protocol stuff */
/*************************/
#define TERM                    "KBtin"   /* terminal type */

/************************************************************************/
/* Do you want to use help compression or not:  with it, space is saved */
/* but without it the help runs slightly faster.  If so, make sure the  */
/* compression stuff is defined in the default values.                  */
/************************************************************************/

#ifndef COMPRESSED_HELP
#define COMPRESSED_HELP 1
#endif

/***********************************************/
/* Some default values you might wanna change: */
/***********************************************/
#define CONSOLE_LENGTH 32768
#define STATUS_COLOR COLOR_BLACK
#define INPUT_COLOR  COLOR_BLUE
#define MARGIN_COLOR COLOR_RED
/* FIXME: neither INPUT_COLOR nor MARGIN_COLOR can be COLOR_WHITE */
/*#define IGNORE_INT*//* uncomment to disable INT (usually ^C) from keyboard */
/*#define BARE_ESC*/  /* uncomment to allow use of bare ESC key.  It will
                         prevent Alt-XXX from being recognized, though. */
#define XTERM_TITLE "KBtin - %s"
#undef  PTY_ECHO_HACK   /* not working yet */
#define ECHO_COLOR "~8~"
#define LOG_INPUT_PREFIX "" /* you can add ANSI codes, etc here */
#define LOG_INPUT_SUFFIX ""
#define RESET_RAW       /* reset pseudo-terminals to raw mode every write */
#define GOTO_CHAR '>'   /* be>mt -> #goto be mt */
                        /*Comment last line out to disable this behavior */
#define DEFAULT_LOGTYPE LOG_LF    /* LOG_RAW: cr/lf or what server sends, LOG_LF: lf, LOG_TTYREC */
#define BRACE_OPEN '{' /*character that starts an argument */
#define BRACE_CLOSE '}' /*character that ends an argument */
#define HISTORY_SIZE 128                  /* history size */
#define MAX_PATH_LENGTH 256               /* max path length (#route) */
#define MAX_LOCATIONS 2048
#define DEFAULT_TINTIN_CHAR '#'           /* tintin char */
#define DEFAULT_TICK_SIZE 60
#define DEFAULT_ROUTE_DISTANCE 10
#define DEFAULT_VERBATIM_CHAR '\\'        /* if an input starts with this
                                             char, it will be sent 'as is'
                                             to the MUD */
#define MAX_SESNAME_LENGTH 512 /* don't accept session names longer than this */
#ifdef __FreeBSD__
#define MAX_RECURSION 64
#else
#define MAX_RECURSION 128
#endif
#ifndef DEFAULT_FILE_DIR
#define DEFAULT_FILE_DIR "." /* Path to Tintin files, or HOME */
#endif
#if COMPRESSED_HELP
#define COMPRESSION_EXT ".gz"      /* for compress: ".Z" */
#define UNCOMPRESS_CMD "gzip -cd " /* for compress: "uncompress -c" */
#else
#define COMPRESSION_EXT ""
#endif
#define NEWS_FILE   "NEWS"
#define CONFIG_DIR ".tintin"
#define CERT_DIR   "ssl"

#define DEFAULT_DISPLAY_BLANK true        /* blank lines */
#define DEFAULT_ECHO_SEPINPUT true        /* echo when there is an input box */
#define DEFAULT_ECHO_NOSEPINPUT false     /* echo when input is not managed */
#define DEFAULT_IGNORE false              /* ignore */
#define DEFAULT_SPEEDWALK false           /* speedwalk */
        /* note: classic speedwalks are possible only on some primitive
           MUDs with only 4 basic directions (w,e,n,s)                   */
#define DEFAULT_PRESUB false              /* presub before actions */
#define DEFAULT_TOGGLESUBS false          /* turn subs on and off FALSE=ON*/
#define DEFAULT_KEYPAD false              /* start in standard keypad mode */
#define DEFAULT_RETAIN false              /* retain the last typed line */
#define DEFAULT_ALIAS_MESS true           /* messages for responses */
#define DEFAULT_ACTION_MESS true          /* when setting/deleting aliases, */
#define DEFAULT_SUB_MESS true             /* actions, etc. may be set to */
#define DEFAULT_HIGHLIGHT_MESS true       /* default either on or off */
#define DEFAULT_VARIABLE_MESS true        /* might want to turn off these */
#define DEFAULT_EVENT_MESS true
#define DEFAULT_ROUTE_MESS true
#define DEFAULT_GOTO_MESS true
#define DEFAULT_BIND_MESS true
#define DEFAULT_SYSTEM_MESS true
#define DEFAULT_PATH_MESS true
#define DEFAULT_ERROR_MESS true
#define DEFAULT_HOOK_MESS true
#define DEFAULT_LOG_MESS true
#define DEFAULT_PRETICK 10
#define DEFAULT_CHARSET "ISO-8859-1"      /* the MUD-side charset */
#define DEFAULT_LOGCHARSET LOGCS_LOCAL
#define DEFAULT_PARTIAL_LINE_MARKER 0
#define BAD_CHAR '?'                      /* marker for chars illegal for a charset */
#define CHAR_VERBATIM '\\'
#define CHAR_QUOTE '"'
#define CHAR_NEWLINE ';'
/*************************************************************************/
/* The text below is checked for. If it trickers then echo is turned off */
/* echo is turned back on the next time the user types a return          */
/*************************************************************************/
#define PROMPT_FOR_PW_TEXT "*assword:*"
#define PROMPT_FOR_PW_TEXT2 "*assphrase:*"
/*************************************************************************/
/* Whether the MUD tells us to echo off, let's check whether it's a      */
/* password input prompt, or a --More-- prompt.  Unfortunately, both     */
/* types can come in a variety of types ("*assword:", "Again:", national */
/* languages, etc), so it's better to assume it's a password if unsure.  */
/*************************************************************************/
#define PROMPT_FOR_MORE_TEXT "*line * of *"

#define REMOVE_ONEELEM_BRACES /* remove braces around one element list in
                                 #splitlist command i.e. {atom} -> atom
                                 similar to #getitemnr command behaviour */

#define EMPTY_LINE "-gag-"
#define STACK_LIMIT 8192*1024

/**************************************************************************/
/* The stuff below here shouldn't be modified unless you know what you're */
/* doing........                                                          */
/**************************************************************************/
typedef enum {PRIORITY, ALPHA, LENGTH} llist_mode_t;
#define K_ACTION_MAGIC "#X~4~~2~~12~[This action is being deleted!]~7~X"

#define BUFFER_SIZE 4096
#define INPUT_CHUNK 1536
enum
{
    MSG_ALIAS,
    MSG_ACTION,
    MSG_SUBSTITUTE,
    MSG_EVENT,
    MSG_HIGHLIGHT,
    MSG_VARIABLE,
    MSG_ROUTE,
    MSG_GOTO,
    MSG_BIND,
    MSG_SYSTEM,
    MSG_PATH,
    MSG_ERROR,
    MSG_HOOK,
    MSG_LOG,
    MAX_MESVAR
};
enum
{
    HOOK_OPEN,
    HOOK_CLOSE,
    HOOK_ZAP,
    HOOK_END,
    HOOK_SEND,
    HOOK_ACTIVATE,
    HOOK_DEACTIVATE,
    HOOK_TITLE,
    NHOOKS
};

/************************ includes *********************/
#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include "_stdint.h"
#include <iconv.h>
#include <ctype.h>
#include <wctype.h>
#include "malloc.h"
#include "unicode.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <wchar.h>
#include <signal.h>
#include <errno.h>
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
#ifdef HAVE_ZLIB
# include <zlib.h>
#endif
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
#if GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif
#ifdef HAVE_GNUTLS
# include <gnutls/gnutls.h>
# include <gnutls/x509.h>
#endif
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t n);
#endif

/************************ structures *********************/
struct listnode
{
    struct listnode *next;
    char *left, *right, *pr;
};

struct hashentry
{
    char *left;
    char *right;
};

struct hashtable
{
    int size;               /* allocated size */
    int nent;               /* current number of entries */
    int nval;               /* current number of values (entries-deleted) */
    struct hashentry *tab;  /* entries table */
};

struct completenode
{
    struct completenode *next;
    char *strng;
};

struct eventnode
{
    struct eventnode *next;
    char *event;
    time_t time;
};

struct routenode
{
    struct routenode *next;
    int dest;
    char *path;
    int distance;
    char *cond;
};

typedef enum { CM_ISO8859_1, CM_UTF8, CM_ICONV, CM_ASCII } conv_mode_t;

struct charset_conv
{
    const char *name;
    conv_mode_t mode;
    int dir; /* -1=only in, 0=both, 1=only out */
    iconv_t i_in, i_out;
};

typedef enum { LOG_RAW, LOG_LF, LOG_TTYREC } logtype_t;

struct session
{
    struct session *next;
    char *name;
    char *address;
    bool tickstatus;
    time_t time0;      /* time of last tick (adjusted every tick) */
    time_t time10;
    int tick_size, pretick;
    bool snoopstatus;
    FILE *logfile, *debuglogfile;
    char *logname, *debuglogname;
    char *loginputprefix, *loginputsuffix;
    logtype_t logtype;
    bool ignore;
    struct listnode *actions, *prompts, *subs, *highs, *antisubs;
    struct hashtable *aliases, *myvars, *pathdirs, *binds;
    struct listnode *path;
    struct routenode *routes[MAX_LOCATIONS];
    char *locations[MAX_LOCATIONS];
    struct eventnode *events;
    int path_length, no_return;
    int socket, last_term_type;
    bool issocket, naws, ga, gas;
    int server_echo; /* 0=not negotiated, 1=we shouldn't echo, 2=we can echo */
    bool more_coming;
    char last_line[BUFFER_SIZE], telnet_buf[BUFFER_SIZE];
    int telnet_buflen;
    bool verbose, blank, echo, speedwalk, togglesubs, presub, verbatim;
    char *partial_line_marker;
    bool mesvar[MAX_MESVAR+1];
    time_t idle_since, server_idle_since;
    time_t sessionstart;
    char *hooks[NHOOKS];
    int closing;
    int nagle;
    bool halfcr_in, halfcr_log; /* \r at the end of a packet */
    int lastintitle;
    char *charset, *logcharset;
    struct charset_conv c_io, c_log;
#ifdef HAVE_ZLIB
    bool can_mccp, mccp_more;
    z_stream *mccp;
    char mccp_buf[INPUT_CHUNK];
#endif
#ifdef HAVE_GNUTLS
    gnutls_session_t ssl;
#endif
    struct timeval line_time;
};

typedef char pvars_t[10][BUFFER_SIZE];

#ifdef PROFILING
# define PROF(x) prof_area=(x)
# define PROFPUSH(x) {const char *prev_prof=prof_area; prof_area=(x)
# define PROFPOP prof_area=prev_prof;}
# define PROFSTART struct timeval tvstart, tvend;gettimeofday(&tvstart,0);
# define PROFEND(x,y) gettimeofday(&tvend,0);(x)+=(tvend.tv_sec-tvstart.tv_sec) \
    *1000000+tvend.tv_usec-tvstart.tv_usec;(y)++;
#else
# define PROF(x)
# define PROFPUSH(x)
# define PROFPOP
# define PROFSTART
# define PROFEND(x,y)
#endif

#ifdef WORDS_BIGENDIAN
# define to_little_endian(x) ((uint32_t) ( \
    ((uint32_t)(x) &(uint32_t)0x000000ffU) << 24 | \
    ((uint32_t)(x) &(uint32_t)0x0000ff00U) <<  8 | \
    ((uint32_t)(x) &(uint32_t)0x00ff0000U) >>  8 | \
    ((uint32_t)(x) &(uint32_t)0xff000000U) >> 24))
#else
# define to_little_endian(x) ((uint32_t)(x))
#endif
#define from_little_endian(x) to_little_endian(x)

struct ttyrec_header
{
    uint32_t sec;
    uint32_t usec;
    uint32_t len;
};

#define logcs_is_special(x) ((x)==LOGCS_LOCAL || (x)==LOGCS_REMOTE)
#define logcs_name(x) (((x)==LOGCS_LOCAL)?"local":              \
                       ((x)==LOGCS_REMOTE)?"remote":            \
                        (x))
#define logcs_charset(x) (((x)==LOGCS_LOCAL)?user_charset_name: \
                          ((x)==LOGCS_REMOTE)?ses->charset:     \
                           (x))

/* Chinese rod numerals are _not_ digits for our purposes. */
static inline bool isadigit(WC x) { return x>='0' && x<='9'; }
/* Solaris is buggy for high-bit chars in UTF-8. */
static inline bool isaspace(WC x) { return x==' ' || x=='\t' || x=='\n' || x==12 || x=='\v'; }
#define iswadigit(x) isadigit(x)
static inline bool isw2width(WC x)
{
    return x>=0x1100  && (x<=0x11ff ||
           x==0x2329 || x==0x232a   ||
           x>=0x2e80) && x!=0x303f
                      && (x<=0xa4cf ||
           x>=0xac00) && (x<=0xd7ff ||
           x>=0xf900) && (x<=0xfaff ||
           x>=0xfe30) && (x<=0xfe6f ||
           x>=0xff00) && (x<=0xff60 ||
           x>=0xffe0) && (x<=0xffe6 ||
           x>=0x20000) && x<=0x3ffff;
}

static inline bool is7alpha(WC x) { return (x>='A'&&x<='Z') || (x>='a'&&x<='z'); }
static inline bool is7alnum(WC x) { return (x>='0'&&x<='9') || is7alpha(x); }
static inline char toalower(char x) { return (x>='A' && x<='Z') ? x+32 : x; }
#define EMPTY_CHAR 0xffff
#define VALID_TIN_CHARS "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define is7punct(x) strchr(VALID_TIN_CHARS, (x))

#define write_stdout(x, len) do if (write(1, (x), (len))!=(len)) \
                                  syserr("write to stdout failed"); while (0)
