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
extern struct listnode *search_node_with_wild(struct listnode *listhead, char *cptr);
extern struct listnode *searchnode_list(struct listnode *listhead, char *cptr);
extern struct listnode *init_list(void);
extern void deletenode_list(struct listnode *listhead, struct listnode *nptr);
extern void insertnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext, int mode);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern void prompt(struct session *ses);
extern void show_list(struct listnode *listhead);
extern void shownode_list(struct listnode *nptr);
extern void tintin_printf(struct session *ses,char *format,...);


extern int bindnum;
extern int mesvar[];

struct listnode *keynames;

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
    struct listnode *mybinds, *ln;

    mybinds = ses->binds;
    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);

    if (!*left)
    {
        tintin_printf(ses,"#Bound keys:");
        show_list(mybinds);
        prompt(ses);
    }
    else if (*left && !*right)
    {
        if ((ln = search_node_with_wild(mybinds, left)) != NULL)
        {
            while ((mybinds = search_node_with_wild(mybinds, left)) != NULL)
                shownode_list(mybinds);
            prompt(ses);
        }
        else if (mesvar[8])
            tintin_printf(ses, "#No match(es) found for {%s}", left);
    }
    else
    {
        if ((ln = searchnode_list(mybinds, left)) != NULL)
            deletenode_list(mybinds, ln);
        insertnode_list(mybinds, left, right, 0, ALPHA);
        if (mesvar[8])
            tintin_printf(ses,"#Ok. {%s} is now bound to {%s}.", left, right);
        bindnum++;
    }
}

/***********************/
/* the #unbind command */
/***********************/
void unbind_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode *mybinds, *ln, *temp;
    int flag;

    flag = FALSE;
    mybinds = ses->binds;
    temp = mybinds;
    arg = get_arg_in_braces(arg, left, 1);
    while ((ln = search_node_with_wild(temp, left)) != NULL)
    {
        flag = TRUE;
        if (mesvar[8])
            tintin_printf(ses,"#Ok. {%s} is no longer bound.", ln->left);
        /* temp=ln; */
        deletenode_list(mybinds, ln);
    }
    if (!flag && mesvar[8])
        tintin_printf(ses, "#No match(es) found for {%s}", left);
}


int find_bind(char *key,int msg,struct session *ses)
{
    struct listnode *ln;

    if ((ln=searchnode_list(ses->binds,key)))
    {          /* search twice, both for raw key code and key name */
        parse_input(ln->right,1,ses);
        return 1;
    };
    if ((ln=searchnode_list(keynames,key)))
        key=ln->right;
    if ((ln=searchnode_list(ses->binds,key)))
    {
        parse_input(ln->right,1,ses);
        return 1;
    }
    if (msg)
        tintin_printf(ses,"#Unbound keycode: %s",key);
    return 0;
}

void init_bind(void)
{
    char**n;
    keynames=init_list();
    for(n=KEYNAMES;**n;n+=2)
        insertnode_list(keynames,n[0],n[1],0,ALPHA);
}
