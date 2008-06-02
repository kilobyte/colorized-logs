#include "tintin.h"
#include "ui.h"
#include "protos/action.h"
#include "protos/alias.h"
#include "protos/hash.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/variables.h"


extern int bindnum;
extern int recursion;

static struct hashtable *keynames;

static char *KEYNAMES[]=
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
        "ESC[25~",	"F13",
        "ESC[26~",	"F14",
        "ESC[27~",	"F15",
        "ESC[28~",	"F16",
        "ESC[29~",	"F17",
        "ESC[30~",	"F18",
        "ESC[31~",	"F19",
        "ESC[32~",	"F20",
        "ESC[33~",	"F21",
        "ESC[34~",	"F22",
        "ESC[35~",	"F23",
        "ESC[36~",	"F24",
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

    if (!ui_keyboard)
    {
        tintin_eprintf(ses, "#UI: no access to keyboard => no keybindings");
        return;
    }
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

    if (!ui_keyboard)
    {
        tintin_eprintf(ses, "#UI: no access to keyboard => no keybindings");
        return;
    }
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
        recursion=0;
        return 1;
    };
    if ((val=get_hash(keynames,key)))
    {
        key=val;
        if ((val=get_hash(ses->binds,key)))
        {
            parse_input(val,1,ses);
            recursion=0;
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
    if (!ui_keyboard)
        return;
    for(n=KEYNAMES;**n;n+=2)
        set_hash(keynames,n[0],n[1]);
}
