/* $Id: events.c,v 2.3 1998/11/25 17:14:00 jku Exp $ */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tintin.h"

extern struct session *sessionlist;
extern struct session *activesession;
extern int Echo;

extern char *get_arg_in_braces();

void execute_event(struct eventnode *ev, struct session *ses)
{
  char tmp[BUFFER_SIZE];

  if(Echo && activesession == ses) {
    sprintf(tmp, "[EVENT: %s]", ev->event);
    tintin_puts2(tmp, ses);
  }
  parse_input(ev->event, ses);
}

/* list active events matching regexp arg */
void list_events(arg, ses)
     char *arg;
     struct session *ses;
{
  char temp[BUFFER_SIZE], left[BUFFER_SIZE];
  int ct; /* current time */
  int flag;
  struct eventnode *ev;

  if (!ses) {
    tintin_puts("#NO SESSION ACTIVE => NO EVENTS!", ses);
    return;
  }

  ct = time(NULL);
  ev = ses->events;
  arg = get_arg_in_braces(arg, left, 1);

  if (!*left) {
    tintin_puts("#Defined events:", ses);
    while(ev != NULL) {
      sprintf(temp, "(%d)\t {%s}", ev->time-ct, ev->event);
      tintin_puts(temp, ses);
      ev = ev->next;
    }
  }
  else {
    flag = 0;
    while(ev != NULL) {
      if (match(left, ev->event)) {
        sprintf(temp, "(%d)\t {%s}", ev->time-ct, ev->event);
        tintin_puts(temp, ses);
	flag = 1;
      }
      ev = ev->next;
    }
    if (flag == 0)
      tintin_puts("#THAT EVENT IS NOT DEFINED.", ses);
  }
}

/* add new event to the list */
void delay_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE];
  int delay;
  struct eventnode *ev, *ptr, *ptrlast;

  if (!ses) {
    tintin_puts("#NO SESSION ACTIVE => NO EVENTS!", ses);
    return;
  }

  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if(!*right) {
    list_events(left, ses);
    return;
  }

  if ((delay = atoi(left)) == 0) {
    tintin_puts("#EVENT IGNORED => DELAY=0", ses);
    return;
  }

  ev = (struct eventnode *)malloc(sizeof(struct eventnode));
  ev->time = time(NULL) + delay;
  ev->next = NULL;
  ev->event = (char *)malloc(strlen(right)+1);
  strcpy(ev->event, right);

  if(ses->events == NULL) {
    ses->events = ev;
  }
  else if(ses->events->time > ev->time) {
    ev->next = ses->events;
    ses->events = ev;
  }
  else {
    ptr = ses->events;
    while((ptrlast = ptr) && (ptr = ptr->next)) {
      if(ptr->time > ev->time) {
	ev->next = ptr;
	ptrlast->next = ev;
	return;
      }
    }
    ptrlast->next = ev;
    ev->next = NULL;
    return;
  }
}

/* remove ev->next from list */
struct eventnode *remove_event(struct eventnode *ev)
{
  struct eventnode *tmp;
  if(ev) {
    tmp = ev->next;
    free(ev->event);
    free(ev);
    return(tmp);
  }
  return(NULL);
}

/* remove events matching regexp arg from list */
void remove_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char temp[BUFFER_SIZE], left[BUFFER_SIZE];
  int flag;
  struct eventnode *ev;
  
  fprintf(stderr, "#REMOVE: %s\n", arg);
  if (!ses) {
    tintin_puts("#NO SESSION ACTIVE => NO EVENTS!", ses);
    return;
  }
tintin_puts("#NIE CHCIALO MI SIE PISAC TEJ PROCEDURY :))", ses);
return;

  ev = ses->events;
  arg = get_arg_in_braces(arg, left, 1);

  flag = 0;
  if (*left && ses->events) {
    if(match(left, ses->events->event)) {
      
    }
    else {
      while(ev != NULL) {
        if (match(left, ev->event)) {
          tintin_puts(temp, ses);
          flag = 1;
        }
        ev = ev->next;
      }
    }
  }
  if (flag == 0)
    tintin_puts("#THAT EVENT IS NOT DEFINED.", ses);
}
