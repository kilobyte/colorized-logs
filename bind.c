#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#include "tintin.h"

extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern void tintin_printf(struct session *ses,char *format,...);
extern char* get_hash(struct hashtable *h, char *key);
extern void set_hash(struct hashtable *h, char *key, char *value);
extern void show_hashlist(struct session *ses, struct hashtable *h, char *pat, const char *msg_all, const char *msg_none);
extern void delete_hashlist(struct session *ses, struct hashtable *h, char *pat, const char *msg_ok, const char *msg_none);
extern struct hashtable* init_hash();
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);


extern int bindnum;

struct hashtable *keynames;

char *KEYNAMES[]=
    {
        "ESC[[A",	"F1",
        "ESC[[B",	"F2",
        "ESC[[C",	"F3",
        "ESC[[D",	"F4",
        "ESC[[E",	"F5",
        "ESC[11~",	"F1",
        "ESC[12~",	"F2",
        "ESC[13~",	"F3",
        "ESC[14~",	"F4",
        "ESC[15~",	"F5",
        "ESC[17~",	"F6",
        "ESC[18~",	"F7",
        "ESC[19~",	"F8",
        "ESC[20~",	"F9",
        "ESC[21~",	"F10",
        "ESC[23~",	"F11",
        "ESC[24~",	"F12",
        "ESC[A",	"UpArrow",
        "ESC[B",	"DownArrow",
        "ESC[C",	"RightArrow",
        "ESC[D",	"LeftArrow",
        "ESC[G",	"MidArrow",
        "ESC[P",	"Pause",
        "ESC[1~",	"Home",
        "ESC[2~",	"Ins",
        "ESC[3~",	"Del",
        "ESC[4~",	"End",
        "ESC[5~",	"PgUp",
        "ESC[6~",	"PgDn",
        "ESC[OA",	"UpArrow",	/* alternate cursor mode */
        "ESC[OB",	"DownArrow",
        "ESC[OC",	"RightArrow",
        "ESC[OD",	"LeftArrow",
        "ESCOM",	"KpadEnter",    /* alternate keypad mode */
        "ESCOP",	"KpadNumLock",
        "ESCOQ",	"KpadDivide",
        "ESCOR",	"KpadMultiply",
        "ESCOS",	"KpadMinus",
        "ESCOk",	"KpadPlus",
        "ESCOl",	"KpadPlus",
        "ESCOn",	"KpadDot",
        "ESCOo",	"KpadMinus",
        "ESCOp",	"Kpad0",
        "ESCOq",	"Kpad1",
        "ESCOr",	"Kpad2",
        "ESCOs",	"Kpad3",
        "ESCOt",	"Kpad4",
        "ESCOu",	"Kpad5",
        "ESCOv",	"Kpad6",
        "ESCOw",	"Kpad7",
        "ESCOx",	"Kpad8",
        "ESCOy",	"Kpad9",
        "",		"",
    };

/*********************/
/* the #bind command */
/*********************/
void bind_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);

    if (*left && *right)
    {
        set_hash(ses->binds, left, right);
        if (ses->mesvar[8])
            tintin_printf(ses,"#Ok. {%s} is now bound to {%s}.", left, right);
        bindnum++;
        return;
    }
    show_hashlist(ses, ses->binds, left,
        "#Bound keys:",
        "#No match(es) found for {%s}");
}

/***********************/
/* the #unbind command */
/***********************/
void unbind_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], result[BUFFER_SIZE];

    arg = get_arg_in_braces(arg, left, 1);
    substitute_vars(left, result);
    substitute_myvars(result, left, ses);
    delete_hashlist(ses, ses->binds, left,
        ses->mesvar[8]? "#Ok. {%s} is no longer bound." : 0,
        ses->mesvar[8]? "#No match(es) found for {%s}" : 0);
}


int find_bind(char *key,int msg,struct session *ses)
{
    char *val;

    if ((val=get_hash(ses->binds,key)))
    {          /* search twice, both for raw key code and key name */
        parse_input(val,1,ses);
        return 1;
    };
    if ((val=get_hash(keynames,key)))
    {
        key=val;
        if ((val=get_hash(ses->binds,key)))
        {
            parse_input(val,1,ses);
            return 1;
        }
    }
    if (msg)
        tintin_printf(ses,"#Unbound keycode: %s",key);
    return 0;
}


void init_bind(void)
{
    char**n;
    keynames=init_hash();
    for(n=KEYNAMES;**n;n+=2)
        set_hash(keynames,n[0],n[1]);
}
