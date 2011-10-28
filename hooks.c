#include "tintin.h"
#include "protos/action.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/utils.h"

extern char tintin_char;
extern int hooknum;
extern pvars_t *pvars;  /* the %0, %1, %2,....%9 variables */
extern int in_alias;

static int magic_close_hook=1;

char *hook_names[]=
{
    "open",
    "close",
    "zap",
    "end",
    "send",
    "activate",
    "deactivate",
};

/*********************/
/* the #hook command */
/*********************/
void hooks_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    int t, flag;

    arg=get_arg(arg, left, 0, ses);
    arg=space_out(arg);
    if (!*left || !*arg)
    {
        flag=1;
        for(t=0;t<NHOOKS;t++)
            if (ses->hooks[t] && (!*left || is_abrev(left, hook_names[t])))
            {
                if (flag && !*left)
                    tintin_printf(ses, "#Defined hooks:"), flag=0;
                tintin_printf(ses, "%-10s: {%s}", hook_names[t], ses->hooks[t]);
                if (*left)
                    return;
            }
        if(flag)
            tintin_printf(ses, *left?"#No such hook.":"#No hooks defined.");
        return;
    }
    arg=get_arg_in_braces(arg, right, 1);
    for(t=0;t<NHOOKS;t++)
        if (is_abrev(left, hook_names[t]))
        {
            SFREE(ses->hooks[t]);
            ses->hooks[t]=mystrdup(right);
            if(ses->mesvar[MSG_HOOK])
                tintin_printf(ses, "#Ok, will do {%s} on %s.", right, hook_names[t]);
            magic_close_hook=0;
            hooknum++;
            return;
        }
    tintin_eprintf(ses, "#Invalid hook: {%s}", left);
}

/***********************/
/* the #unhook command */
/***********************/
void unhook_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    int t, flag;

    arg=get_arg(arg, left, 1, ses);
    if (!*left)
    {
        flag=1;
        for(t=0;t<NHOOKS;t++)
            if (ses->hooks[t])
            {
                if (flag)
                    tintin_printf(ses, "#Defined hooks:"), flag=0;
                tintin_printf(ses, "%-10s: {%s}", hook_names[t], ses->hooks[t]);
            }
        if(flag)
            tintin_printf(ses, "#No hooks defined.");
        return;
    }
    for(t=0;t<NHOOKS;t++)
        if (is_abrev(left, hook_names[t]))
        {
            if(ses->mesvar[MSG_HOOK])
                tintin_printf(ses, ses->hooks[t]?"#Removing hook on {%s}":
                    "#There was no hook on {%s} anyway", hook_names[t]);
            SFREE(ses->hooks[t]);
            ses->hooks[t]=0;
            return;
        }
    tintin_eprintf(ses, "#Invalid hook: {%s}", left);
}

struct session* do_hook(struct session *ses, int t, char *data, int blockzap)
{
    pvars_t vars,*lastvars;
    int i, oldclos=oldclos;

    if (!ses->hooks[t])
        return ses;

    if (blockzap)
    {
        oldclos=ses->closing;
        ses->closing=-1;
    }
    lastvars=pvars;
    pvars=&vars;
    for(i=0;i<10;i++)
        vars[i][0]=0;
    if (data)
    {
        strcpy(vars[0], data);
        strcpy(vars[1], data);
    }
    if (ses->mesvar[MSG_HOOK]&&!magic_close_hook)
    {
        char buffer[BUFFER_SIZE];

        prepare_actionalias(ses->hooks[t], buffer, ses);
        tintin_printf(ses,"[HOOK: %s]", buffer);
    }
    in_alias=1;
    ses=parse_input(ses->hooks[t],1,ses);
    if (blockzap)
        ses->closing=oldclos;
    pvars=lastvars;
    return ses;
}

void set_magic_hook(struct session *ses)
{
    char temp[BUFFER_SIZE];

    sprintf(temp,"%cif {1==%clistlength {$SESSIONS}} %cend",
        tintin_char,tintin_char,tintin_char);
    SFREE(ses->hooks[1]);
    ses->hooks[1]=mystrdup(temp);
}
