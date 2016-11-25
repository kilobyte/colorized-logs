#include "tintin.h"
#include "protos/action.h"
#include "protos/glob.h"
#include "protos/globals.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/utils.h"
#include "protos/variables.h"


void execute_event(struct eventnode *ev, struct session *ses)
{
    if (activesession==ses && ses->mesvar[MSG_EVENT])
        tintin_printf(ses, "[EVENT: %s]", ev->event);
    parse_input(ev->event, true, ses);
    recursion=0;
}

/* list active events matching regexp arg */
static void list_events(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    time_t ct; /* current time */
    bool flag;
    struct eventnode *ev;

    if (!ses)
    {
        tintin_eprintf(ses, "#NO SESSION ACTIVE => NO EVENTS!");
        return;
    }

    ct = time(NULL);
    ev = ses->events;
    arg = get_arg_in_braces(arg, left, 1);

    if (!*left)
    {
        tintin_printf(ses, "#Defined events:");
        while (ev != NULL)
        {
            tintin_printf(ses, "(%d)\t {%s}", ev->time-ct, ev->event);
            ev = ev->next;
        }
    }
    else
    {
        flag = false;
        while (ev != NULL)
        {
            if (match(left, ev->event))
            {
                tintin_printf(ses, "(%d)\t {%s}", ev->time-ct, ev->event);
                flag = true;
            }
            ev = ev->next;
        }
        if (!flag)
            tintin_printf(ses, "#THAT EVENT (%s) IS NOT DEFINED.", left);
    }
}

/* add new event to the list */
void delay_command(const char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE], *cptr;
    time_t delay;
    struct eventnode *ev, *ptr, *ptrlast;

    if (!ses)
    {
        tintin_eprintf(ses, "#NO SESSION ACTIVE => NO EVENTS!");
        return;
    }

    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);
    substitute_vars(left, temp);
    substitute_myvars(temp, left, ses);
    substitute_vars(right, temp);
    substitute_myvars(temp, right, ses);
    if (!*right)
    {
        list_events(left, ses);
        return;
    }

    if (!*left || (delay=strtol(left, &cptr, 10))<0 || *cptr)
    {
        tintin_eprintf(ses, "#EVENT IGNORED (DELAY={%s}), NEGATIVE DELAY", left);
        return;
    }

    ev = TALLOC(struct eventnode);
    ev->time = time(NULL) + delay;
    ev->next = NULL;
    ev->event = mystrdup(right);

    if (ses->events == NULL)
        ses->events = ev;
    else if (ses->events->time > ev->time)
    {
        ev->next = ses->events;
        ses->events = ev;
    }
    else
    {
        ptr = ses->events;
        while ((ptrlast = ptr) && (ptr = ptr->next))
        {
            if (ptr->time > ev->time)
            {
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

void event_command(const char *arg, struct session *ses)
{
    delay_command(arg, ses);
}

/* remove ev->next from list */
static void remove_event(struct eventnode **ev)
{
    struct eventnode *tmp;
    if (*ev)
    {
        tmp = (*ev)->next;
        SFREE((*ev)->event);
        TFREE(*ev, struct eventnode);
        *ev=tmp;
    }
}

/* remove events matching regexp arg from list */
void undelay_command(const char *arg, struct session *ses)
{
    char temp[BUFFER_SIZE], left[BUFFER_SIZE];
    bool flag;
    struct eventnode **ev;

    if (!ses)
    {
        tintin_eprintf(ses, "#NO SESSION ACTIVE => NO EVENTS!");
        return;
    }

    arg = get_arg_in_braces(arg, left, 1);
    substitute_vars(left, temp);
    substitute_myvars(temp, left, ses);

    if (!*left)
    {
        tintin_eprintf(ses, "#ERROR: valid syntax is: #undelay {event pattern}");
        return;
    }

    flag = false;
    ev = &(ses->events);
    while (*ev)
        if (match(left, (*ev)->event))
        {
            flag=true;
            if (ses==activesession && ses->mesvar[MSG_EVENT])
                tintin_printf(ses, "#Ok. Event {%s} at %ld won't be executed.",
                    (*ev)->event, (*ev)->time-time(0));
            remove_event(ev);
        }
        else
            ev=&((*ev)->next);

    if (!flag)
        tintin_printf(ses, "#THAT EVENT IS NOT DEFINED.");
}

void removeevent_command(const char *arg, struct session *ses)
{
    undelay_command(arg, ses);
}

void unevent_command(const char *arg, struct session *ses)
{
    undelay_command(arg, ses);
}
