/* $Id: session.c,v 2.3 1998/10/12 09:45:23 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: session.c.c - funtions related to sessions                  */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include "tintin.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void show_session();
struct session *new_session();

extern char *get_arg_in_braces();
extern char *space_out();
extern char *mystrdup();
extern struct listnode *copy_list();
extern struct listnode *init_list();
extern int run(char *command);

extern int sessionsstarted;
extern struct session *sessionlist, *activesession;
extern struct listnode *common_aliases, *common_actions, *common_subs;
extern struct listnode *common_myvars, *common_highs, *common_antisubs;
extern struct listnode *common_pathdirs;
extern char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */


/*
  common code for #session and #run for cases of:
    #session            - list all sessions
    #session {a}        - print info about session a
  (opposed to #session {a} {mud.address.here 666} - starting a new session)
*/
int list_sessions(char *arg,struct session *ses,char *left,char *right)
{
  struct session *sesptr;
  /* struct listnode *ln; */
  /* int i; */
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);

  if (!*left) {
    tintin_puts("#THESE SESSIONS HAS BEEN DEFINED:", ses);
    for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
      show_session(sesptr);
    prompt(ses);
  } else if (*left && !*right) {
    for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
      if (!strcmp(sesptr->name, left)) {
	show_session(sesptr);
	break;
      }
    if (sesptr == NULL) {
      tintin_puts("#THAT SESSION IS NOT DEFINED.", ses);
      prompt(NULL);
    }
  } else {
    for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
      if (strcmp(sesptr->name, left) == 0) {
	tintin_puts("#THERE'S A SESSION WITH THAT NAME ALREADY.", ses);
	prompt(NULL);
	return(1);
      };
    return(0);
  };
  return(1);
}

/************************/
/* the #session command */
/************************/
struct session *session_command(char *arg,struct session *ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE];
	int sock;
	char *host, *port;

	if (list_sessions(arg,ses,left,right))
		return(ses);	/* (!*left)||(!*right) */

  port = host = space_out(mystrdup(right));

  if (!*host) {
    tintin_puts("#HEY! SPECIFY AN ADDRESS WILL YOU?", ses);
    return ses;
  }
  while (*port && !isspace(*port))
    port++;
  *port++ = '\0';
  port = space_out(port);

  if (!*port) {
    tintin_puts("#HEY! SPECIFY A PORT NUMBER WILL YOU?", ses);
    return ses;
  }
  if (!(sock = connect_mud(host, port, ses)))
    return ses;

	return(new_session(left,right,sock,ses));
}


/********************/
/* the #run command */
/********************/
struct session *run_command(char *arg,struct session *ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE];
	int sock;

	if (list_sessions(arg,ses,left,right))
		return(ses);	/* (!*left)||(!*right) */

	if (!*right)
	{
		tintin_puts("#HEY! SPECIFY AN COMMAND, WILL YOU?",ses);
		return(ses);
	};

	if (!(sock=run(right)))
		return ses;

	return(new_session(left,right,sock,ses));
}


/******************/
/* show a session */
/******************/
void show_session(ses)
     struct session *ses;
{
  char temp[BUFFER_SIZE];

  sprintf(temp, "%-10s{%s}", ses->name, ses->address);

  if (ses == activesession)
    strcat(temp, " (active)");
  if (ses->snoopstatus)
    strcat(temp, " (snooped)");
  if (ses->logfile)
    strcat(temp, " (logging)");
  tintin_puts2(temp, (struct session *)NULL);
  prompt(NULL);
}

/**********************************/
/* find a new session to activate */
/**********************************/
struct session *newactive_session()
{

  if (sessionlist) {
    char buf[BUFFER_SIZE];

    activesession = sessionlist;
    sprintf(buf, "#SESSION '%s' ACTIVATED.", sessionlist->name);
    tintin_puts(buf, NULL);
  } else
    tintin_puts("#THERE'S NO ACTIVE SESSION NOW.", NULL);
  prompt(NULL);
  return sessionlist;
}


/**********************/
/* open a new session */
/**********************/
struct session *new_session(char *name,char *address,int sock,
                            struct session *ses)
{
  struct session *newsession;
  int i;
  
  newsession = (struct session *)malloc(sizeof(struct session));

  newsession->name = mystrdup(name);
  newsession->address = mystrdup(address);
  newsession->tickstatus = FALSE;
  newsession->tick_size = DEFAULT_TICK_SIZE;
  newsession->time0 = 0;
  newsession->snoopstatus = FALSE;
  newsession->logfile = NULL;
  newsession->ignore = DEFAULT_IGNORE;
  newsession->aliases = copy_list(common_aliases, ALPHA);
  newsession->actions = copy_list(common_actions, PRIORITY);
  newsession->subs = copy_list(common_subs, ALPHA);
  newsession->myvars = copy_list(common_myvars, ALPHA);
  newsession->highs = copy_list(common_highs, ALPHA);
  newsession->pathdirs = copy_list(common_pathdirs, ALPHA);
  newsession->socket = sock;
  newsession->antisubs = copy_list(common_antisubs, ALPHA);
  newsession->socketbit = 1 << sock;
  newsession->next = sessionlist;
  for (i = 0; i < HISTORY_SIZE; i++)
    newsession->history[i] = NULL;
  newsession->path = init_list(newsession->path);
  newsession->path_list_size = 0;
  newsession->path_length = 0;
  newsession->more_coming = 0;
  newsession->events = NULL;
  newsession->old_more_coming=0;
  sessionlist = newsession;
  activesession = newsession;
  sessionsstarted++;


  return (newsession);
}

/*****************************************************************************/
/* cleanup after session died. if session=activesession, try find new active */
/*****************************************************************************/
void cleanup_session(ses)
     struct session *ses;
{
  int i;
  char buf[BUFFER_SIZE];
  struct session *sesptr;

  sessionsstarted--;
  kill_all(ses, END);
  /* printf("DEBUG: Hist: %d \n\r",HISTORY_SIZE); */
  /* CHANGED to fix a possible memory leak
     for(i=0; i<HISTORY_SIZE; i++)
     ses->history[i]=NULL;
   */
  for (i = 0; i < HISTORY_SIZE; i++)
    if ((ses->history[i]))
      free(ses->history[i]);
  if (ses == sessionlist)
    sessionlist = ses->next;
  else {
    for (sesptr = sessionlist; sesptr->next != ses; sesptr = sesptr->next) ;
    sesptr->next = ses->next;
  }
  sprintf(buf, "#SESSION '%s' DIED.", ses->name);
  tintin_puts(buf, NULL);
/*  if(write(ses->socket, "ctld\n", 5)<5)
     syserr("write in cleanup"); *//* can't do this, cozof the peer stuff in net.c */
  if (close(ses->socket) == -1)
    syserr("close in cleanup");
  if (ses->logfile)
    fclose(ses->logfile);

  free(ses);
}
