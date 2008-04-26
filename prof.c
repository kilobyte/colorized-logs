#include "config.h"
#include "tintin.h"

#include <signal.h>

#ifdef PROFILING

typedef void (*sighandler_t)(int);

extern struct hashtable* init_hash();
extern char* get_hash(struct hashtable *h, char *key);
extern void set_hash_nostring(struct hashtable *h, char *key, char *value);
extern void tintin_printf(struct session *ses,char *format,...);
extern struct listnode* hash2list(struct hashtable *h, char *pat);
extern void syserr(char *msg, ...);
extern void zap_list(struct listnode *nptr);

char *prof_area;
time_t kbd_lag, mud_lag;
int kbd_cnt, mud_cnt;
struct hashtable *prof_count;

static void sigprof(void)
{
    int c;

    if (!prof_area)
        return;
    c=(int)get_hash(prof_count, prof_area);
    set_hash_nostring(prof_count, prof_area, (char *)(c+1));
}

void setup_prof()
{
    struct sigaction act;
    struct itimerval it;
    
    prof_count=init_hash();
#define PS(x) set_hash_nostring(prof_count, x, 0);
#include "profinit.h"
#undef PS

    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_RESTART;            

    act.sa_handler=(sighandler_t)sigprof;
    if (sigaction(SIGPROF,&act,0))
        syserr("sigaction SIGPROF");
    it.it_interval.tv_sec=0;
    it.it_interval.tv_usec=20000;
    it.it_value.tv_sec=0;
    it.it_value.tv_usec=1;
    setitimer(ITIMER_PROF, &it, 0);
    PROF("init");
    kbd_lag=0;
    mud_lag=0;
}

void profile_command(struct session *ses, char *arg)
{
    struct listnode *hl,*ln;
    
    tintin_printf(0, "#KBtin profiling data:");
    hl=hash2list(prof_count,"*");
    ln=hl;
    while((ln=ln->next))
        tintin_printf(0, "%-26s %6d", ln->left, (int)ln->right);
    zap_list(hl);
    if (kbd_cnt)
        tintin_printf(0, "Avg. response time for kbd input: %d.%06d",
            (kbd_lag/kbd_cnt)/1000000, (kbd_lag/kbd_cnt)%1000000);
    if (mud_cnt)
        tintin_printf(0, "Avg. response time for mud input: %d.%06d",
            (mud_lag/mud_cnt)/1000000, (mud_lag/mud_cnt)%1000000);
}

#endif
