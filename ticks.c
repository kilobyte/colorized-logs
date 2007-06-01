/* $Id: ticks.c,v 2.4 1998/11/25 17:14:01 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: ticks.c - functions for the ticker stuff                    */
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
#include <signal.h>
#include <assert.h>
#include "tintin.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
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

/* externs */
struct session *sessionlist;

/* extern functions */
extern void tintin_puts(char *, struct session *);
extern void execute_event(struct eventnode *ev, struct session *ses);
extern void tintin_puts1(char *cptr, struct session *ses);
extern void tintin_printf(struct session *ses, char *format, ...);
extern void tintin_eprintf(struct session *ses, char *format, ...);
extern char *get_arg(char *s,char *arg,int flag,struct session *ses);

/* local globals */
int sec_to_tick, time0, tick_size = 75;
int ticker_interrupted;

/*********************/
/* the #tick command */
/*********************/
void tick_command(char *arg,struct session *ses)
{
    if (ses)
    {
        char buf[100];
        int to_tick;

        to_tick = ses->tick_size - (time(NULL) - ses->time0) % ses->tick_size;
        sprintf(buf, "THERE'S NOW %d SECONDS TO NEXT TICK.", to_tick);
        tintin_puts(buf, ses);
    }
    else
        tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
}

/************************/
/* the #tickoff command */
/************************/
void tickoff_command(char *arg,struct session *ses)
{
    if (ses)
    {
        ses->tickstatus = FALSE;
        tintin_puts("#TICKER IS NOW OFF.", ses);
    }
    else
        tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
}

/***********************/
/* the #tickon command */
/***********************/
void tickon_command(char *arg,struct session *ses)
{
    if (ses)
    {
        ses->tickstatus = TRUE;
        if(ses->time0 == 0)
            ses->time0 = time(NULL);
        tintin_puts("#TICKER IS NOW ON.", ses);
    }
    else
        tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
}


/*************************/
/* the #ticksize command */
/*************************/
void ticksize_command(char *arg,struct session *ses)
{
    int x;
    char left[BUFFER_SIZE], *err;

    get_arg(arg,left,1,ses);
    if (!ses)
    {
        tintin_printf(ses, "#NO SESSION ACTIVE => NO TICKER!");
        return;
    }
    if (!*left || !isdigit(*left))
    {
        tintin_eprintf(ses, "#SYNTAX: #ticksize <number>");
        return;
    }
    x=strtol(left,&err,10);
    if (*err || x<1 || x>=0x7fffffff)
    {
        tintin_eprintf(ses, "#TICKSIZE OUT OF RANGE (1..%d)", 0x7fffffff);
        return;
    }
    ses->tick_size = x;
    ses->time0 = time(NULL);
    tintin_printf(ses, "#OK. NEW TICKSIZE SET");
}


/************************/
/* the #pretick command */
/************************/
void pretick_command(char *arg,struct session *ses)
{
    int x;
    char left[BUFFER_SIZE], *err;

    get_arg(arg,left,1,ses);
    if (!ses)
    {
        tintin_printf(ses, "#NO SESSION ACTIVE => NO TICKER!");
        return;
    }
    if (!*left)
        x=ses->pretick? 0 : 10;
    else
    {
        x=strtol(left,&err,10);
        if (*err || x<0 || x>=0x7fffffff)
        {
            tintin_eprintf(ses, "#PRETICK VALUE OUT OF RANGE (0..%d)", 0x7fffffff);
            return;
        }
    }
    if (x>=ses->tick_size)
    {
        tintin_eprintf(ses, "#PRETICK (%d) has to be smaller than #TICKSIZE (%d)",
            x, ses->tick_size);
        return;
    }
    ses->pretick = x;
    ses->time10 = time(NULL);
    if (x)
        tintin_printf(ses, "#OK. PRETICK SET TO %d", x);
    else
        tintin_printf(ses, "#OK. PRETICK TURNED OFF");
}


void show_pretick_command(char *arg,struct session *ses)
{
    pretick_command(arg, ses);
}


int timetilltick(struct session *ses)
{
    return ses->tick_size - (time(NULL) - ses->time0) % ses->tick_size;
}

/* returns the time (since 1970) of next event (tick) */
int check_event(int time, struct session *ses)
{
    int tt; /* tick time */
    int et; /* event time */
    struct eventnode *ev;

    assert(ses != NULL);

    /* events check  - that should be done in #delay */
    while((ev=ses->events) && (ev->time<=time))
    {
        ses->events=ev->next;
        execute_event(ev, ses);
        TFREE(ev, struct eventnode);
    }
    et = (ses->events) ? ses->events->time : 0;

    /* ticks check */
    tt = ses->time0 + ses->tick_size; /* expected time of tick */

    if (tt <= time)
    {
        if (ses->tickstatus)
            tintin_puts1("#TICK!!!",ses);
        ses->time0 = time - (time - ses->time0) % ses->tick_size;
        tt = ses->time0 + ses->tick_size;
    }
    else if (ses->tickstatus && tt-ses->pretick==time
            && ses->tick_size>ses->pretick && time!=ses->time10)
    {
        tintin_puts1("#10 SECONDS TO TICK!!!",ses);
        ses->time10=time;
    }

    if(ses->tickstatus && ses->tick_size>ses->pretick && tt-time>ses->pretick)
        tt-=ses->pretick;

    return ((et<tt && et!=0) ? et : tt);
}
