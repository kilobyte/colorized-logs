/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: main.c - main module - signal setup/shutdown etc            */
/*                             TINTIN++                              */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include <stdlib.h>

#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#else
#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy ((s), (d), (n))
#define memmove(d, s, n) bcopy ((s), (d), (n))
#endif
#endif

#include <signal.h>
#include "tintin.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef BADSIG
#define BADSIG (void (*)())-1
#endif

/*************** globals ******************/
int blank = DEFAULT_DISPLAY_BLANK;
int term_echoing = TRUE;
int Echo = DEFAULT_ECHO;
int speedwalk = DEFAULT_SPEEDWALK;
int togglesubs = DEFAULT_TOGGLESUBS;
int presub = DEFAULT_PRESUB;
int mudcolors = DEFAULT_MUDCOLORS;
int sessionsstarted;
int puts_echoing = TRUE;
int verbose = FALSE;
int alnum = 0;
int acnum = 0;
int subnum = 0;
int varnum = 0;
int hinum = 0;
int pdnum = 0;
int antisubnum = 0;
int verbatim = 0;
char homepath[1025];
char E = 27;
extern int hist_num;
extern int need_resize;

struct session *sessionlist, *activesession;
struct listnode *common_aliases, *common_actions, *common_subs, *common_myvars;
struct listnode *common_highs, *common_antisubs, *common_pathdirs;
char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */
char tintin_char = DEFAULT_TINTIN_CHAR;
char verbatim_char = DEFAULT_VERBATIM_CHAR;
char system_com[80] = SYSTEM_COMMAND_DEFAULT;
int mesvar[7];
int display_row, display_col, input_row, input_col;
int split_line, term_columns;
char prev_command[BUFFER_SIZE];
int text_came;
void tintin();
void read_mud();
void do_one_line();
void snoop();
void tintin_puts();
void tintin_puts2();
char status[BUFFER_SIZE];

/************ externs *************/
extern char done_input[BUFFER_SIZE];
extern void user_init();
extern int process_kbd();
extern void textout(char *txt);

extern int ticker_interrupted, time0;
extern int tick_size, sec_to_tick;

static void myquitsig();
extern struct session *newactive_session();
extern struct session *parse_input();
extern struct session *read_command();
extern struct completenode *complete_head;
extern struct listnode *init_list();
extern void read_complete();
extern void syserr();
extern void sigwinch();
extern void user_resize();

extern int do_one_antisub();
extern void do_one_sub();
extern void do_one_high();
extern void prompt();
extern int check_event();
extern void check_all_actions();

#ifdef HAVE_TIME_H
#include <time.h>
#endif
int read();
extern void do_history();
extern int read_buffer_mud();
extern void cleanup_session();
int write();

int last_line_length;

#if defined(HAVE_SYS_TERMIO_H) && !defined(BSD_ECHO)
#include <sys/termio.h>
#ifdef HAVE_TCFLAG_T
tcflag_t c_lflag;
cc_t c_cc[NCCS];

#else
unsigned char c_cc[NCC];
unsigned short c_lflag;

#endif
#endif

/* CHANGED to get rid of double-echoing bug when tintin++ gets suspended */
void tstphandler(sig)
     int sig;
{
  echo();
  kill(getpid(), SIGSTOP);
  noecho();
}


void sigchild()
{
    waitpid(0,0,WNOHANG);
}

/**************************************************************************/
/* main() - show title - setup signals - init lists - readcoms - tintin() */
/**************************************************************************/

void main(argc, argv, environ)
     int argc;
     char **argv;
     char **environ;
{
  struct session *ses;
  char *strptr, temp[BUFFER_SIZE];
  int arg_num;
  int fd;

  strcpy(status,".");
  user_init();  
  text_came = FALSE;
  read_complete();
  hist_num=-1;
  ses = NULL;
  srand(getpid());

  tintin_puts2("~2~##################################################", ses);
  sprintf(temp, "#              ~11~K B - t i n~7~     v %-15s ~2~#", VERSION_NUM);
  tintin_puts2(temp, ses);
  tintin_puts2("#~7~ based on ~12~tintin++~7~ v 2.1.9 by Peter Unold,      ~2~#", ses);
  tintin_puts2("#~7~  Bill Reiss, David A. Wagner, Joann Ellsworth, ~2~#", ses);
  tintin_puts2("#~7~    Jeremy C. Jack and Ulan@GrimneMUD           ~2~#", ses);
  tintin_puts2("##################################################~7~", ses);
  tintin_puts2("Check #news now!", ses);

  if (signal(SIGTERM, myquitsig) == BADSIG)
    syserr("signal SIGTERM");
  if (signal(SIGINT, myquitsig) == BADSIG)
    syserr("signal SIGINT");
  /* CHANGED to get rid of double-echoing bug when tintin++ gets suspended */
  if (signal(SIGTSTP, tstphandler) == BADSIG)
    syserr("signal SIGTSTP");
  if (signal(SIGWINCH,sigwinch) == BADSIG)
  	syserr("signal SIGWINCH");
  if (signal(SIGCHLD,sigchild) == BADSIG)
  	syserr("signal SIGCHLD");
  time0 = time(NULL);

  common_aliases = init_list();
  common_actions = init_list();
  common_subs = init_list();
  common_myvars = init_list();
  common_highs = init_list();
  common_antisubs = init_list();
  common_pathdirs = init_list();
  mesvar[0] = DEFAULT_ALIAS_MESS;
  mesvar[1] = DEFAULT_ACTION_MESS;
  mesvar[2] = DEFAULT_SUB_MESS;
  mesvar[3] = DEFAULT_ANTISUB_MESS;
  mesvar[4] = DEFAULT_HIGHLIGHT_MESS;
  mesvar[5] = DEFAULT_VARIABLE_MESS;
  mesvar[6] = DEFAULT_PATHDIR_MESS;
  *homepath = '\0';
  if (!strcmp(DEFAULT_FILE_DIR, "HOME"))
    if ((strptr = (char *)getenv("HOME")))
      strcpy(homepath, strptr);
    else
      *homepath = '\0';
  else
    strcpy(homepath, DEFAULT_FILE_DIR);
  arg_num = 1;
  if (argc > 1 && argv[1]) {
    if (*argv[1] == '-' && *(argv[1] + 1) == 'v') {
      arg_num = 2;
      verbose = TRUE;
    }
  }
  if (argc > arg_num && argv[arg_num]) {
    activesession = read_command(argv[arg_num], NULL);
  } else {
    strcpy(temp, homepath);
    strcat(temp, "/.tintinrc");
    if ((fd = open(temp, O_RDONLY)) > 0) {	/* Check if it exists */
      close(fd);
      activesession = read_command(temp, NULL);
    } else {
      if ((strptr = (char *)getenv("HOME"))) {
	strcpy(homepath, strptr);
	strcpy(temp, homepath);
	strcat(temp, "/.tintinrc");
	if ((fd = open(temp, O_RDONLY)) > 0) {	/* Check if it exists */
	  close(fd);
	  activesession = read_command(temp, NULL);
	}
      }
    }
  }
  tintin();
}

/******************************************************/
/* return seconds to next tick (global, all sessions) */
/* also display tick messages                         */
/******************************************************/
int check_events()
{
  struct session *sp;
  int tick_time = 0, curr_time, tt;

  curr_time = time(NULL);
  for (sp = sessionlist; sp; sp = sp->next) {
      tt = check_event(curr_time, sp);
      /* printf("#%s %d(%d)\n", sp->name, tt, curr_time); */
      if (tt > curr_time && (tick_time == 0 || tt < tick_time)) {
	tick_time = tt;
      }
  }

  if (tick_time > curr_time)
    return (tick_time - curr_time);
  return (50);	/* we don't return 0 to kluge around a bug in #select */
}

/***************************/
/* the main loop of tintin */
/***************************/
void tintin()
{
  char buffer[BUFFER_SIZE], strng[80];
  int didget, done, readfdmask, result;
  struct session *sesptr, *t;
  struct timeval tv;

  while (TRUE) {
    readfdmask = 1;
    for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
      readfdmask |= sesptr->socketbit;
    /* ticker_interrupted=FALSE; */

    tv.tv_sec = check_events();
    tv.tv_usec = 0;

    result = select(32, (fd_set *) & readfdmask, 0, 0, (struct timeval *)&tv);

    if (result == 0)
      continue;
    else if (result < 0 && errno == EINTR)
/*      tintin_puts("#Interrupted system call - ignoring... :)\n", \
		  (struct session *)NULL); */;
        	     /* getting a signal is not a reason to spam the user */
    else if (result < 0)
      syserr("select");
      
   if (need_resize)
   	user_resize();


    if (readfdmask & 1) {
      done = process_kbd(activesession);
      if (done) {
	hist_num=-1;
	if (term_echoing) {
	  if (activesession && *done_input)
	    if (strcmp(done_input, prev_command))
	      do_history(done_input, activesession);
	  if (Echo)
	  {
	      textout(done_input);
	      textout("\n");
	  };
	}
	if (*done_input)
	  strcpy(prev_command, done_input);
	activesession = parse_input(done_input, activesession);
      }
    }
    for (sesptr = sessionlist; sesptr; sesptr = t) {
      t = sesptr->next;
      if (sesptr->socketbit & readfdmask) {
	read_mud(sesptr);
      }
    }
  }
}


/*************************************************************/
/* read text from mud and test for actions/snoop/substitutes */
/*************************************************************/
void read_mud(ses)
     struct session *ses;
{
	char buffer[BUFFER_SIZE], linebuffer[BUFFER_SIZE], *cpsource, *cpdest;
	char temp[BUFFER_SIZE];
	int didget, n, count;

	if (!(didget = read_buffer_mud(buffer, ses)))
	{
  		cleanup_session(ses);
		if (ses == activesession)
			activesession = newactive_session();
		return;
	};
	if (ses->logfile)
		if (!OLD_LOG)
		{
			count = 0;
			for (n = 0; n < didget; n++)
				if (buffer[n] != '\r')
					temp[count++] = buffer[n];
			fwrite(temp, count, 1, ses->logfile);
		}
		else
			fwrite(buffer, didget, 1, ses->logfile);

	cpsource = buffer;
	cpdest = linebuffer;
	if (ses->old_more_coming == 1)
	{
		strcpy(linebuffer, ses->last_line);
		cpdest += strlen(linebuffer);
	};
	while (*cpsource)
	{		/*cut out each of the lines and process */
	    	if (*cpsource == '\n' || *cpsource == '\r')
    		{
			*cpdest = '\0';
			do_one_line(linebuffer, ses);

			if (!(*linebuffer == '.' && !*(linebuffer + 1)))
			{
				char tmp[2];
				tmp[0]=*cpsource++;
				tmp[1]=0;
				textout(tmp);
			}
			else
				cpsource++;
			cpdest = linebuffer;
		}
		else
			*cpdest++ = *cpsource++;
	}
	*cpdest = '\0';
	if (ses->more_coming)
		strcpy(ses->last_line, linebuffer);
	else
		do_one_line(linebuffer, ses);
}
/**********************************************************/
/* do all of the functions to one line of buffer          */
/**********************************************************/
void do_one_line(line, ses)
     char *line;
     struct session *ses;
{
  if (mudcolors)
      do_in_MUD_colors(line);
  if (!presub && !ses->ignore)
    check_all_actions(line, ses);
  if (!togglesubs)
    if (!do_one_antisub(line, ses))
      do_one_sub(line, ses);
  if (presub && !ses->ignore)
    check_all_actions(line, ses);
  do_one_high(line, ses);
  if (strcmp(line,"."))
      if (ses==activesession)
	textout(line);
      else
	if (ses->snoopstatus)
  		snoop(line,ses);
}

/**********************************************************/
/* snoop session ses - chop up lines and put'em in buffer */
/**********************************************************/
void snoop(buffer, ses)
     char *buffer;
     struct session *ses;
{
  /* int n; */
  char *cpsource, *cpdest, line[BUFFER_SIZE], linebuffer[BUFFER_SIZE],
    header[BUFFER_SIZE];

  *linebuffer = '\0';
  if (display_col != 1) {
    sprintf(line, "\n");
    strcat(linebuffer, line);
  }
  cpsource = buffer;
  sprintf(header, "%s%% ", ses->name);
  strcpy(line, header);
  cpdest = line + strlen(line);
  while (*cpsource) {
    if (*cpsource == '\n' || *cpsource == '\r') {
      *cpdest++ = *cpsource++;
      if (*cpsource == '\n' || *cpsource == '\r')
	*cpdest++ = *cpsource++;
      *cpdest = '\0';
      strcat(linebuffer, line);
      cpdest = line + strlen(header);
    } else
      *cpdest++ = *cpsource++;
  }
  if (cpdest != line + strlen(header)) {
    *cpdest++ = '\n';
    *cpdest = '\0';
    strcat(linebuffer, line);
  }
  textout(linebuffer);
  display_col = 1;
}

/*****************************************************/
/* output to screen should go throught this function */
/* text gets checked for actions                     */
/*****************************************************/
void tintin_puts(cptr, ses)
     char *cptr;
     struct session *ses;
{
  tintin_puts2(cptr, ses);
  if (ses)
    check_all_actions(cptr, ses);

}
/*****************************************************/
/* output to screen should go throught this function */
/* not checked for actions                           */
/*****************************************************/
void tintin_puts2(cptr, ses)
     char *cptr;
     struct session *ses;
{
  char strng[1024];

  if ((ses == activesession || ses == NULL) && puts_echoing) {
    sprintf(strng, "%s\n", cptr);
    textout(strng);
    display_col = 1;
    text_came = TRUE;
  }
}
/*****************************************************/
/* output to screen should go throught this function */
/* not checked for actions                           */
/*****************************************************/
void tintin_puts3(cptr, ses)
     char *cptr;
     struct session *ses;
{
  char strng[1024];

  if ((ses == activesession || ses == NULL) && puts_echoing) {
    cptr++;
    sprintf(strng, "%s\n", cptr);
    textout(strng);
    display_col = 1;
  }
  text_came = TRUE;
}

/**********************************************************/
/* Here's where we go when we wanna quit TINTIN FAAAAAAST */
/**********************************************************/
static void myquitsig()
{
  struct session *sesptr, *t;

  for (sesptr = sessionlist; sesptr; sesptr = t) {
    t = sesptr->next;
    cleanup_session(sesptr);
  }
  sesptr = NULL;

   textout("\nYour fireball hits TINTIN with full force, causing an immediate death.\n");
   textout("TINTIN is dead! R.I.P.\n");
   textout("Your blood freezes as you hear TINTIN's death cry.\n");
  user_done();
  exit(0);
}