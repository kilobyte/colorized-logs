/*********************************************************************/
/* file: tintin.h - the include file for tintin++                    */
/*                             TINTIN ++                             */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                    modified by Bill Reiss 1993                    */
/*                     coded by peter unold 1992                     */
/*********************************************************************/

#include <stdio.h>
#include <curses.h>
/************************/
/* The meaning of life: */
/************************/
#undef TRUE
#undef FALSE
#define TRUE 1
#define FALSE 0

/************************************************************************/
/* Do you want to use help compression or not:  with it, space is saved */
/* but without it the help runs slightly faster.  If so, make sure the  */
/* compression stuff is defined in the default values.                  */
/************************************************************************/

#ifndef COMPRESSED_HELP
#define COMPRESSED_HELP TRUE
#endif

#ifdef HAVE_BCOPY
#define memcpy(s1, s2, n) bcopy(s2, s1, n)
#endif

/***********************************************/
/* Some default values you might wanna change: */
/***********************************************/
#define SCREEN_WIDTH 80
#define ALPHA 1
#define PRIORITY 0
#define CLEAN 0
#define END 1
#define OLD_LOG 0 /* set to one to use old-style logging */
#define DEFAULT_OPEN '{' /*character that starts an argument */
#define DEFAULT_CLOSE '}' /*character that ends an argument */
#define SYSTEM_COMMAND_DEFAULT "system"   /* name of the system command */
#define HISTORY_SIZE 30                   /* history size */
#define MAX_PATH_LENGTH 200               /* max path lenght */
#define DEFAULT_TINTIN_CHAR '#'           /* tintin char */
#define DEFAULT_TICK_SIZE 75              /* typical for diku */
#define DEFAULT_VERBATIM_CHAR '\\'        /* if an input starts with this
                                             char, it will be sent 'as is'
                                             to the MUD */
#ifndef DEFAULT_FILE_DIR
#define DEFAULT_FILE_DIR "/usr/local/lib/tintin" /* Path to Tintin files, or HOME */
#endif
#if COMPRESSED_HELP
#define DEFAULT_COMPRESSION_EXT ".Z"     /* for compress: ".Z" */
#define DEFAULT_EXPANSION_STR "uncompress -c "/* for compress: "uncompress -c" */
#endif

#define DEFAULT_DISPLAY_BLANK TRUE        /* blank lines */
#define DEFAULT_ECHO TRUE                 /* echo */         
#define DEFAULT_IGNORE FALSE              /* ignore */
#define DEFAULT_SPEEDWALK FALSE           /* speedwalk */
#define DEFAULT_PRESUB FALSE              /* presub before actions */
#define DEFAULT_TOGGLESUBS FALSE          /* turn subs on and off FALSE=ON*/
#define DEFAULT_MUDCOLORS TRUE            /* convert ~n~ to color codes */

#define DEFAULT_ALIAS_MESS TRUE           /* messages for responses */
#define DEFAULT_ACTION_MESS TRUE          /* when setting/deleting aliases, */
#define DEFAULT_SUB_MESS TRUE             /* actions, etc. may be set to */
#define DEFAULT_ANTISUB_MESS TRUE         /* default either on or off */
#define DEFAULT_HIGHLIGHT_MESS TRUE       /* TRUE=ON FALSE=OFF */
#define DEFAULT_VARIABLE_MESS TRUE        /* might want to turn off these */
#define DEFAULT_PATHDIR_MESS TRUE
/**************************************************************************/
/* Whenever TINTIN has written something to the screen, the program sends */
/* a CR/LF to the diku to force a new prompt to appear. You can have      */
/* TINTIN print it's own pseudo prompt instead.                           */
/**************************************************************************/
#define PSEUDO_PROMPT TRUE
/**************************************************************************/
/* the codes below are used for highlighting text, and is set for the     */
/* codes for VT-100 terminal emulation. If you are using a different      */
/* teminal type, replace the codes below with the correct codes and       */
/* change the codes set up in highlight.c                                 */
/**************************************************************************/
#define DEFAULT_BEGIN_COLOR "["
#define DEFAULT_END_COLOR "[m"
/*************************************************************************/
/* The text below is checked for. If it trickers then echo is turned off */
/* echo is turned back on the next time the user types a return          */
/*************************************************************************/
#define PROMPT_FOR_PW_TEXT "assword:"

/**************************************************************************/ 
/* The stuff below here shouldn't be modified unless you know what you're */
/* doing........                                                          */
/**************************************************************************/ 
#define BUFFER_SIZE 2048
#define VERSION_NUM "0.2.1"
/************************ structures *********************/
struct listnode {
  struct listnode *next;
  char *left, *right, *pr;
};

struct completenode {
  struct completenode *next;
  char *strng;
};

struct eventnode {
  struct eventnode *next;
  char *event;
  int time; /* time_t */
};

struct session {
  struct session *next;
  char *name;
  char *address;
  int tickstatus;
  int time0;      /* time of last tick (adjusted every tick) */
  int tick_size;
  int snoopstatus;
  FILE *logfile;
  int ignore;
  struct listnode *aliases, *actions, *subs, *myvars, *highs, *antisubs;  
  char *history[HISTORY_SIZE];
  struct listnode *path, *pathdirs;
  struct eventnode *events;
  int path_length, path_list_size;
  int socket, socketbit;
  int old_more_coming,more_coming;
  char last_line[BUFFER_SIZE];
};
