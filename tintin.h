/******************************************************************/
/* file: tintin.h - the include file for KBtin                    */
/******************************************************************/
#define NDEBUG
#define UTF8                /* UTF8 support */

#ifndef NDEBUG
#undef TELNET_DEBUG        /* define to show TELNET negotiations */
#undef USER_DEBUG          /* debugging of the user interface */
#undef TERM_DEBUG          /* debugging pseudo-tty stuff */
#define PROFILING          /* profiling */
#endif

/************************/
/* The meaning of life: */
/************************/
#define TRUE 1
#define FALSE 0

/**********************/
/* color ANSI numbers */
/**********************/
#define COLOR_BLACK 	0
#define COLOR_BLUE  	4
#define COLOR_GREEN	2
#define COLOR_CYAN	6
#define COLOR_RED	1
#define COLOR_MAGENTA	5
#define COLOR_YELLOW	3
#define COLOR_WHITE	7

#define LOGCS_LOCAL	((char*)1)
#define LOGCS_REMOTE	((char*)2)


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
#define COMPRESSED_HELP TRUE
#endif

/***********************************************/
/* Some default values you might wanna change: */
/***********************************************/
#define CONSOLE_LENGTH 32768
#define STATUS_COLOR COLOR_BLACK
#define INPUT_COLOR  COLOR_BLUE
#define MARGIN_COLOR COLOR_RED
/* FIXME: neither INPUT_COLOR nor MARGIN_COLOR can be COLOR_WHITE */
/*#define GRAY2 */    /* if you have problems with the dark gray (~8~) color */
/*#define IGNORE_INT*//* uncomment to disable INT (usually ^C) from keyboard */
/*#define BARE_ESC*/  /* uncomment to allow use of bare ESC key.  It will
                         prevent Alt-XXX from being recognized, though. */
#define XTERM_TITLE "KBtin - %s"
#undef  PTY_ECHO_HACK   /* not working yet */
#define ASSUME_UTF8 0      /* if character width cannot be autodetected */
#define ECHO_COLOR "~8~"
#define LOG_INPUT_PREFIX "" /* you can add ANSI codes, etc here */
#define LOG_INPUT_SUFFIX ""
#define RESET_RAW       /* reset pseudo-terminals to raw mode every write */
#define GOTO_CHAR '>'	/* be>mt -> #goto be mt */
		/*Comment last line out to disable this behavior */
#define OLD_LOG 0 /* set to one to use old-style logging */
#define DEFAULT_LOGTYPE (1-OLD_LOG)       /* 0: cr/lf, 1: lf, 2: ttyrec */
#define DEFAULT_OPEN '{' /*character that starts an argument */
#define DEFAULT_CLOSE '}' /*character that ends an argument */
#define HISTORY_SIZE 128                  /* history size */
#define MAX_PATH_LENGTH 256               /* max path lenght */
#define MAX_LOCATIONS 512
#define DEFAULT_TINTIN_CHAR '#'           /* tintin char */
#define DEFAULT_TICK_SIZE 60
#define DEFAULT_ROUTE_DISTANCE 10
#define DEFAULT_VERBATIM_CHAR '\\'        /* if an input starts with this
                                             char, it will be sent 'as is'
                                             to the MUD */
#define MAX_RECURSION 256
#ifndef DEFAULT_FILE_DIR
#define DEFAULT_FILE_DIR "." /* Path to Tintin files, or HOME */
#endif
#if COMPRESSED_HELP
#define DEFAULT_COMPRESSION_EXT ".gz"     /* for compress: ".Z" */
#define DEFAULT_EXPANSION_STR "gzip -cd " /* for compress: "uncompress -c" */
#else
#define DEFAULT_COMPRESSION_EXT ""
#endif
#define NEWS_FILE   "NEWS"

#define DEFAULT_DISPLAY_BLANK TRUE        /* blank lines */
#define DEFAULT_ECHO_SEPINPUT TRUE        /* echo when there is an input box */
#define DEFAULT_ECHO_NOSEPINPUT FALSE     /* echo when input is not managed */
#define DEFAULT_IGNORE FALSE              /* ignore */
#define DEFAULT_SPEEDWALK FALSE           /* speedwalk */
	/* note: classic speedwalks are possible only on some primitive
	   MUDs with only 4 basic directions (w,e,n,s)                   */
#define DEFAULT_PRESUB FALSE              /* presub before actions */
#define DEFAULT_TOGGLESUBS FALSE          /* turn subs on and off FALSE=ON*/
#define DEFAULT_KEYPAD FALSE              /* start in standard keypad mode */
#define DEFAULT_RETAIN FALSE              /* retain the last typed line */
#define DEFAULT_ALIAS_MESS TRUE           /* messages for responses */
#define DEFAULT_ACTION_MESS TRUE          /* when setting/deleting aliases, */
#define DEFAULT_SUB_MESS TRUE             /* actions, etc. may be set to */
#define DEFAULT_HIGHLIGHT_MESS TRUE       /* default either on or off */
#define DEFAULT_VARIABLE_MESS TRUE        /* might want to turn off these */
#define DEFAULT_EVENT_MESS TRUE
#define DEFAULT_ROUTE_MESS TRUE
#define DEFAULT_GOTO_MESS TRUE
#define DEFAULT_BIND_MESS TRUE
#define DEFAULT_SYSTEM_MESS TRUE
#define DEFAULT_PATH_MESS TRUE
#define DEFAULT_ERROR_MESS TRUE
#define DEFAULT_HOOK_MESS TRUE
#define DEFAULT_PRETICK 10
#define DEFAULT_CHARSET "ISO-8859-1"	/* the MUD-side charset */
#define DEFAULT_LOGCHARSET LOGCS_LOCAL
/*#define PARTIAL_LINE_MARKER "\376"*/       /* comment out to disable */
#define BAD_CHAR '?'	        /* marker for chars illegal for a charset */
#define CHAR_VERBATIM '\\'
#define CHAR_QUOTE '"'
#define CHAR_NEWLINE ';'
/**************************************************************************/
/* Should a prompt appear whenever TINTIN has written something to the    */
/* screen?                                                                */
/**************************************************************************/
#define FORCE_PROMPT FALSE
/**************************************************************************/
/* Whenever TINTIN has written something to the screen, the program sends */
/* a CR/LF to the MUD to force a new prompt to appear. You can have       */
/* TINTIN print it's own pseudo prompt instead.                           */
/**************************************************************************/
#define PSEUDO_PROMPT FALSE
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
#define STOP_AT_SPACES 0
#define WITH_SPACES 1
#define ALPHA 1
#define PRIORITY 0
#define LENGTH 2
#define CLEAN 0
#define END 1
#define K_ACTION_MAGIC "#X~4~~2~~12~[This action is being deleted!]~7~X"

#define BUFFER_SIZE 2048
#define INPUT_CHUNK 512
#define MSG_ALIAS       0
#define MSG_ACTION      1
#define MSG_SUBSTITUTE  2
#define MSG_EVENT       3
#define MSG_HIGHLIGHT   4
#define MSG_VARIABLE    5
#define MSG_ROUTE       6
#define MSG_GOTO        7
#define MSG_BIND        8
#define MSG_SYSTEM      9
#define MSG_PATH        10
#define MSG_ERROR       11
#define MSG_HOOK        12
#define MAX_MESVAR      13
#define HOOK_OPEN       0
#define HOOK_CLOSE      1
#define HOOK_ZAP        2
#define HOOK_END        3
#define HOOK_SEND       4
#define HOOK_ACTIVATE	5
#define HOOK_DEACTIVATE	6
#define NHOOKS          7

/************************ structures *********************/
#include <stdio.h>
#include "_stdint.h"
#ifdef HAVE_ICONV_H
# include <iconv.h>
#else
# ifdef HAVE_SYS_ICONV_H
#  include <sys/iconv.h>
# else
#  error No iconv -- no fun.  Grab it and install!
# endif
#endif
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

struct listnode {
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
  int time; /* time_t */
};

struct routenode
{
	struct routenode *next;
	int dest;
	char *path;
	int distance;
	char *cond;
};

struct charset_conv
{
    char *name;
    int mode; /* 0=8bit, 1=UTF-8, 2=iconv */
    int dir; /* -1=only in, 0=both, 1=only out */
    iconv_t i_in, i_out;
};

struct session
{
  struct session *next;
  char *name;
  char *address;
  int tickstatus;
  int time0;      /* time of last tick (adjusted every tick) */
  int time10;
  int tick_size,pretick;
  int snoopstatus;
  FILE *logfile,*debuglogfile;
  char *logname,*debuglogname;
  int logtype;
  int ignore;
  struct listnode *actions, *prompts, *subs, *highs, *antisubs;
  struct hashtable *aliases, *myvars, *pathdirs, *binds;
  struct listnode *path;
  struct routenode *routes[MAX_LOCATIONS];
  char *locations[MAX_LOCATIONS];
  struct eventnode *events;
  int path_length, no_return;
  int socket, issocket, naws, ga, gas, last_term_type;
  int server_echo; /* 0=not negotiated, 1=we shouldn't echo, 2=we can echo */
  int more_coming;
  char last_line[BUFFER_SIZE],telnet_buf[BUFFER_SIZE];
  int telnet_buflen;
  int verbose,blank,echo,speedwalk,togglesubs,presub,verbatim;
  int mesvar[MAX_MESVAR+1];
  int idle_since;
  int sessionstart;
  char *hooks[NHOOKS];
  int closing;
  int nagle;
  int halfcr_in, halfcr_log; /* \r at the end of a packet */
#ifdef UTF8
  char *charset, *logcharset;
  struct charset_conv c_io,c_log;
#endif
#ifdef HAVE_LIBZ
  int can_mccp;
  z_stream *mccp;
  int mccp_more;
  char mccp_buf[INPUT_CHUNK];
#endif
};

typedef char pvars_t[10][BUFFER_SIZE];

#ifdef PROFILING
# define PROF(x) prof_area=(x)
# define PROFPUSH(x) {char *prev_prof=prof_area; prof_area=(x)
# define PROFPOP prof_area=prev_prof;}
# define PROFSTART struct timeval tvstart,tvend;gettimeofday(&tvstart,0);
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
    (((uint32_t)(x) &((uint32_t)0x000000ffU) << 24)) | \
    (((uint32_t)(x) &((uint32_t)0x0000ff00U) <<  8)) | \
    (((uint32_t)(x) &((uint32_t)0x00ff0000U) >>  8)) | \
    (((uint32_t)(x) &((uint32_t)0xff000000U) >> 24))))
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
#define logcs_name(x) (((x)==LOGCS_LOCAL)?"local":	\
                       ((x)==LOGCS_REMOTE)?"remote":	\
                        (x))
#define logcs_charset(x) (((x)==LOGCS_LOCAL)?user_charset_name:	\
                          ((x)==LOGCS_REMOTE)?ses->charset:	\
                           (x))

/* Chinese rod numerals are _not_ digits for our purposes. */
#define isadigit(x) ((x)>='0' && (x)<='9')
