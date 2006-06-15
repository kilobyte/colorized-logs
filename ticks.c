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
#ifdef HAVE_TIME_H
#include <time.h>
#endif

/* externs */
struct session *sessionlist;

/* extern functions */
extern void tintin_puts(char *, struct session *);
extern void execute_event(struct eventnode *ev, struct session *ses);
extern void tintin_puts1(char *cptr, struct session *ses);

/* local globals */
int sec_to_tick, time0, tick_size = 75;
int ticker_interrupted;

/*********************/
/* the #tick command */
/*********************/
void tick_command(struct session *ses)
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
void tickoff_command(struct session *ses)
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
void tickon_command(struct session *ses)
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
    if (ses)
    {
        if (*arg != '\0')
        {
            if (isdigit(*arg))
            {
                ses->tick_size = atoi(arg);
                ses->time0 = time(NULL);
                tintin_puts("#OK NEW TICKSIZE SET", ses);
            }
            else
                tintin_puts("#SPECIFY A NUMBER!!!!TRYING TO CRASH ME EH?", ses);
        }
        else
            tintin_puts("#SET THE TICK-SIZE TO WHAT?", ses);
    }
    else
        tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
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
        free(ev);
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
    else if (ses->tickstatus && tt-10==time && ses->tick_size>10 && time!=ses->time10)
    {
        tintin_puts1("#10 SECONDS TO TICK!!!",ses);
        ses->time10=time;
    }

    if(ses->tickstatus && ses->tick_size>10 && tt-time>10)
        tt-=10;

    return ((et<tt && et!=0) ? et : tt);
}
