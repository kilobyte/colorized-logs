/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: misc.c - misc commands                                      */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>
#include "tintin.h"
#include "ui.h"


/* externs */
extern struct session *sessionlist,*activesession,*nullsession;
extern struct completenode *complete_head;
extern char tintin_char;
extern int keypad,retain;
extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */
extern char status[BUFFER_SIZE];
int margins,marginl,marginr;
extern FILE *mypopen(const char *command, int wr);
extern int LINES,COLS;
extern void cleanup_session(struct session *ses);
extern int count_list(struct listnode *listhead);
extern int count_routes(struct session *ses);
extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern char *get_arg(char *s,char *arg,int flag,struct session *ses);
extern int is_abrev(char *s1, char *s2);
extern struct session *newactive_session(void);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern void prepare_actionalias(char *string, char *result, struct session *ses);
extern void prompt(struct session *ses);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);
extern void tintin_puts(char *cptr,struct session *ses);
extern void tintin_puts1(char *cptr,struct session *ses);
extern void tintin_printf(struct session *ses, char *format, ...);
extern void tintin_eprintf(struct session *ses, char *format, ...);
extern void write_line_mud(char *line, struct session *ses);
extern void set_variable(char *left,char *right,struct session *ses);
extern void do_all_high(char *line,struct session *ses);
extern void do_all_sub(char *line, struct session *ses);
extern int getcolor(char **ptr,int *color,const int flag);
extern void kill_all(struct session *ses, int mode);
extern void do_in_MUD_colors(char *txt,int quotetype);
extern int puts_echoing,in_read;
extern struct session* do_hook(struct session *ses, int t, char *data, int blockzap);
extern char *logtypes[];
extern int real_quiet;
extern char *user_charset_name;
extern void utf8_to_local(char *d, char *s);
extern void local_to_utf8(char *d, char *s, int maxb, mbstate_t *cs);
extern int new_conv(struct charset_conv *conv, char *name, int dir);
extern void cleanup_conv(struct charset_conv *conv);
extern char* mystrdup(char *s);
extern int wc_to_utf8(char *d, const wchar_t *s, int n, int maxb);
extern char *space_out(char *s);
extern int utf8_to_wc(wchar_t *d, char *s, int n);


int yes_no(char *txt)
{
    if (!*txt)
        return -2;
    if (!strcmp(txt,"0"))
        return 0;
    if (!strcmp(txt,"1"))
        return 1;
    if (!strcasecmp(txt,"NO"))
        return 0;
    if (!strcasecmp(txt,"YES"))
        return 1;
    if (!strcasecmp(txt,"OFF"))
        return 0;
    if (!strcasecmp(txt,"ON"))
        return 1;
    if (!strcasecmp(txt,"FALSE"))
        return 0;
    if (!strcasecmp(txt,"TRUE"))
        return 1;
    return -1;
}

static void togglebool(int *b, char *arg, struct session *ses, char *msg1, char *msg2)
{
    char tmp[BUFFER_SIZE];
    int old=*b;

    get_arg(arg,tmp,1,ses);
    if (*tmp)
    {
        switch(yes_no(tmp))
        {
        case 0:
            *b=0; break;
        case 1:
            *b=1; break;
        default:
            tintin_eprintf(ses,"#Valid boolean values are: 1/0, YES/NO, TRUE/FALSE, ON/OFF. Got {%s}.",tmp);
        }
    }
    else
        *b=!*b;
    if (*b!=old)
        tintin_printf(ses,*b?msg1:msg2);
}

/****************************/
/* the #cr command          */
/****************************/
void cr_command(char *arg,struct session *ses)
{
    if (ses != nullsession)
        write_line_mud("", ses);
}

/****************************/
/* the #version command     */
/****************************/
void version_command(char *arg,struct session *ses)
{
    tintin_printf(ses, "#You are using KBtin %s", VERSION);
    prompt(ses);
}

/****************************/
/* the #verbatim command    */
/****************************/
void verbatim_command(char *arg,struct session *ses)
{
    togglebool(&ses->verbatim,arg,ses,
               "#All text is now sent 'as is'.",
               "#Text is no longer sent 'as is'.");
}

/************************/
/* the #send command    */
/************************/
void send_command(char *arg,struct session *ses)
{
    char temp1[BUFFER_SIZE];
    if (ses==nullsession)
    {
        tintin_eprintf(ses, "#No session -> can't #send anything");
        return;
    }
    if (!*arg)
    {
        tintin_eprintf(ses, "#send what?");
        return;
    }
    arg = get_arg(arg, temp1, 1, ses);
    write_line_mud(temp1,ses);
}

/********************/
/* the #all command */
/********************/
struct session* all_command(char *arg,struct session *ses)
{
    struct session *sesptr;

    if ((sessionlist!=nullsession)||(nullsession->next))
    {
        get_arg(arg, arg, 1, ses);
        for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
            if (sesptr!=nullsession)
                parse_input(arg,1,sesptr);
    }
    else
        tintin_eprintf(ses,"#all: BUT THERE ISN'T ANY SESSION AT ALL!");
    return ses;
}

/*********************/
/* the #bell command */
/*********************/
void bell_command(char *arg,struct session *ses)
{
    user_beep();
}


/*********************/
/* the #char command */
/*********************/
void char_command(char *arg,struct session *ses)
{
    get_arg_in_braces(arg, arg, 1);
    /* It doesn't make any sense to use a variable here. */
#ifdef UTF8
    if (ispunct(*arg) || ((unsigned char)(*arg)>127))
#else
    if (ispunct(*arg) && ((unsigned char)(*arg)<127)) /* possible feature... */
#endif
    {
        tintin_char = *arg;
        tintin_printf(ses, "#OK. TINTIN-CHAR is now {%c}", tintin_char);
    }
    else
        tintin_eprintf(ses,"#SPECIFY A PROPER TINTIN-CHAR! SOMETHING LIKE # OR /!");
}


/*********************/
/* the #echo command */
/*********************/
void echo_command(char *arg,struct session *ses)
{
    togglebool(&ses->echo,arg,ses,
               "#ECHO IS NOW ON.",
               "#ECHO IS NOW OFF.");
}

/***********************/
/* the #keypad command */
/***********************/
void keypad_command(char *arg,struct session *ses)
{
    if (!ui_keyboard)
    {
        tintin_eprintf(ses, "#UI: no access to keyboard => no keybindings");
        return;
    }

    togglebool(&keypad,arg,ses,
               "#KEYPAD NOW WORKS IN THE ALTERNATE MODE.",
               "#KEYPAD KEYS ARE NOW EQUAL TO NON-KEYPAD ONES.");
    user_keypad(keypad);
}

/***********************/
/* the #retain command */
/***********************/
void retain_command(char *arg,struct session *ses)
{
    if (!ui_sep_input)
    {
        tintin_eprintf(ses, "#UI: no managed windows => no input bar");
        return;
    }

    togglebool(&retain,arg,ses,
               "#INPUT BAR WILL NOW RETAIN THE LAST LINE TYPED.",
               "#INPUT BAR WILL NOW BE CLEARED EVERY LINE.");
    user_retain();
}

/*********************/
/* the #end command */
/*********************/
void end_command(char *arg, struct session *ses)
{
    struct session *sp;
    struct session *sesptr;

    for (sesptr = sessionlist; sesptr; sesptr = sp)
    {
        sp = sesptr->next;
        if (sesptr!=nullsession && !sesptr->closing)
        {
            sesptr->closing=1;
            do_hook(sesptr, HOOK_ZAP, 0, 1);
            sesptr->closing=0;
            cleanup_session(sesptr);
        }
    }
    activesession = nullsession;
    do_hook(nullsession, HOOK_END, 0, 1);
    activesession = NULL;
    if (ui_own_output)
    {
        tintin_printf(0,"Goodbye!");
        user_done();
    }
    exit(0);
}

/***********************/
/* the #ignore command */
/***********************/
void ignore_command(char *arg,struct session *ses)
{
    if (ses!=nullsession)
        togglebool(&ses->ignore,arg,ses,
                   "#ACTIONS ARE IGNORED FROM NOW ON.",
                   "#ACTIONS ARE NO LONGER IGNORED.");
    else
        /* don't pierce !#verbose, as this can be set in a config file */
        tintin_printf(ses,"#No session active => Nothing to ignore!");
}

/**********************/
/* the #presub command */
/**********************/
void presub_command(char *arg,struct session *ses)
{
    togglebool(&ses->presub,arg,ses,
               "#ACTIONS ARE NOW PROCESSED ON SUBSTITUTED BUFFER.",
               "#ACTIONS ARE NO LONGER DONE ON SUBSTITUTED BUFFER.");
}

/**********************/
/* the #blank command */
/**********************/
void blank_command(char *arg,struct session *ses)
{
    togglebool(&ses->blank,arg,ses,
               "#INCOMING BLANK LINES ARE NOW DISPLAYED.",
               "#INCOMING BLANK LINES ARE NO LONGER DISPLAYED.");
}

/**************************/
/* the #togglesubs command */
/**************************/
void togglesubs_command(char *arg,struct session *ses)
{
    togglebool(&ses->togglesubs,arg,ses,
               "#SUBSTITUTES ARE NOW IGNORED.",
               "#SUBSTITUTES ARE NO LONGER IGNORED.");
}

/************************/
/* the #verbose command */
/************************/
void verbose_command(char *arg,struct session *ses)
{
    real_quiet=1;
    togglebool(&ses->verbose,arg,ses,
               "#Output from #reads will now be shown.",
               "#The #read command will no longer output messages.");
    if (in_read)
        puts_echoing=ses->verbose;
}

/************************/
/* the #margins command */
/************************/
void margins_command(char *arg,struct session *ses)
{
    int l,r;
    char num[BUFFER_SIZE], *tmp;

    if (!ui_sep_input)
    {
        tintin_eprintf(ses, "#UI: no input bar => no input margins");
        return;
    }

    if (margins)
    {
        margins=0;
        tintin_printf(ses,"#MARGINS DISABLED.");
    }
    else
    {
        l=marginl;
        r=marginr;
        if (arg)
        {
            arg=get_arg(arg,num,0,ses);
            if (*num)
            {
                l=strtoul(num,&tmp,10);
                if (*tmp||(l<=0))
                {
                    tintin_eprintf(ses,"#Left margin must be a positive number! Got {%s}.",num);
                    return;
                }
                if (l>=BUFFER_SIZE)
                {
                    tintin_eprintf(ses,"#Left margin too big (%d)!",l);
                    return;
                }
            };
            arg=get_arg(arg,num,1,ses);
            if (*num)
            {
                r=strtoul(num,&tmp,10);
                if (*tmp||(r<l))
                {
                    tintin_eprintf(ses,"#Right margin must be a number greater than the left margin! Got {%s}.",tmp);
                    return;
                }
                if (r>=BUFFER_SIZE)
                {
                    tintin_eprintf(ses,"#Right margin too big (%d)!",r);
                    return;
                }
            }
        };
        marginl=l;
        marginr=r;
        margins=1;
        tintin_printf(ses,"#MARGINS ENABLED.");
    }
}


/***********************/
/* the #showme command */
/***********************/
void showme_command(char *arg,struct session *ses)
{
    get_arg(arg, arg, 1, ses);
    tintin_printf(ses,"%s",arg);	/* KB: no longer check for actions */
}

/***********************/
/* the #loop command   */
/***********************/
void loop_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    int flag, bound1, bound2, counter;
    pvars_t vars,*lastpvars;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg_in_braces(arg, right, 1);
    flag = 1;
    if (sscanf(left, "%d,%d", &bound1, &bound2) != 2)
        tintin_eprintf(ses,"#Wrong number of arguments in #loop: {%s}.",left);
    else
    {
        if (pvars)
            for (counter=1; counter<10; counter++)
                strcpy(vars[counter],(*pvars)[counter]);
        else
            for (counter=1; counter<10; counter++)
                strcpy(vars[counter],"");
        lastpvars=pvars;
        pvars=&vars;
    
        flag = 1;
        counter = bound1;
        while (flag == 1)
        {
            sprintf(vars[0], "%d", counter);
            parse_input(right,1,ses);
            if (bound1 < bound2)
            {
                counter++;
                if (counter > bound2)
                    flag = 0;
            }
            else
            {
                counter--;
                if (counter < bound2)
                    flag = 0;
            }
        }
        pvars=lastpvars;
    }
}

static char *msNAME[]=
    {
        "aliases",
        "actions",
        "substitutes",
        "events",
        "highlights",
        "variables",
        "routes",
        "gotos",
        "binds",
        "#system",
        "paths",
        "errors",
        "hooks",
        "logging",
        "all"
    };

/*************************/
/* the #messages command */
/*************************/
void messages_command(char *arg,struct session *ses)
{
    char offon[2][20];
    int mestype;
    char type[BUFFER_SIZE],onoff[BUFFER_SIZE];

    strcpy(offon[0], "OFF.");
    strcpy(offon[1], "ON.");
    arg=get_arg(arg, type, 0, ses);
    arg=get_arg(arg, onoff, 1, ses);
    if (!*type)
    {
        for (mestype=0;mestype<MAX_MESVAR;++mestype)
            tintin_printf(ses, "#Messages concerning %s are %s",
                    msNAME[mestype], offon[ses->mesvar[mestype]]);
        return;
    };
    mestype = 0;
    while ((mestype<MAX_MESVAR+1)&&(!is_abrev(type, msNAME[mestype])))
        mestype++;
    if (mestype == MAX_MESVAR+1)
        tintin_eprintf(ses,"#Invalid message type to toggle: {%s}",type);
    else
    {
        if (mestype<MAX_MESVAR)
        {
            switch(yes_no(onoff))
            {
            case 0:
                ses->mesvar[mestype]=0;
                break;
            case 1:
                ses->mesvar[mestype]=1;
                break;
            case -1:
                tintin_eprintf(ses, "#messages: Hey! What should I do with %s? Specify a boolean value, not {%s}.",
                        msNAME[mestype],onoff);
                return;
            default:
                ses->mesvar[mestype]=!ses->mesvar[mestype];
            };
            tintin_printf(ses,"#Ok. messages concerning %s are now %s",
                    msNAME[mestype], offon[ses->mesvar[mestype]]);
        }
        else
        {
            int b=yes_no(onoff);
            if (b==-1)
            {
                tintin_eprintf(ses,"#messages: Hey! What should I do with all messages? Specify a boolean, not {%s}.",onoff);
                return;
            };
            if (b==-2)
            {
                b=1;
                for (mestype=0;mestype<MAX_MESVAR;mestype++)
                    if (ses->mesvar[mestype])	/* at least one type is ON? */
                        b=0;			/* disable them all */
            };
            for (mestype=0;mestype<MAX_MESVAR;mestype++)
                ses->mesvar[mestype]=b;
            if (b)
                tintin_printf(ses,"#Ok. All messages are now ON.");
            else
                tintin_printf(ses,"#Ok. All messages are now OFF.");
        }
    }
}

/**********************/
/* the #snoop command */
/**********************/
void snoop_command(char *arg,struct session *ses)
{
    struct session *sesptr = ses;

    if (ses)
    {
        get_arg(arg, arg, 1, ses);
        if (*arg)
        {
            for (sesptr = sessionlist; sesptr && strcmp(sesptr->name, arg); sesptr = sesptr->next) ;
            if (!sesptr)
            {
                tintin_eprintf(ses,"#There is no session named {%s}!",arg);
                return;
            }
        }
        if (sesptr->snoopstatus)
        {
            sesptr->snoopstatus = FALSE;
            tintin_printf(ses, "#UNSNOOPING SESSION '%s'", sesptr->name);
        }
        else
        {
            sesptr->snoopstatus = TRUE;
            tintin_printf(ses,"#SNOOPING SESSION '%s'", sesptr->name);
        }
    }
    else
        tintin_printf(ses,"#NO SESSION ACTIVE => NO SNOOPING");
}

/**************************/
/* the #speedwalk command */
/**************************/
void speedwalk_command(char *arg,struct session *ses)
{
    togglebool(&ses->speedwalk,arg,ses,
               "#SPEEDWALK IS NOW ON.",
               "#SPEEDWALK IS NOW OFF.");
}


/***********************/
/* the #status command */
/***********************/
void status_command(char *arg,struct session *ses)
{
    if (!ui_sep_input)
    {
        tintin_eprintf(ses, "#UI: no managed windows => no status bar");
        return;
    }

    if (ses!=activesession)
        return;
    get_arg(arg,arg,1,ses);
    if (*arg)
        strncpy(status,arg,BUFFER_SIZE);
    else
        strcpy(status,EMPTY_LINE);
    user_show_status();
}


/***********************/
/* the #system command */
/***********************/
void system_command(char *arg,struct session *ses)
{
    FILE *output;
    char buf[BUFFER_SIZE],ustr[BUFFER_SIZE];
    mbstate_t cs;
    
    get_arg(arg, arg, 1, ses);
    if (*arg)
    {
        if (ses->mesvar[9])
            tintin_puts1("#EXECUTING SHELL COMMAND.", ses);
#ifdef UTF8
        utf8_to_local(buf, arg);
        if (!(output = mypopen(buf,0)))
#else
        if (!(output = mypopen(arg,0)))
#endif
        {
            tintin_puts1("#ERROR EXECUTING SHELL COMMAND.",ses);
            prompt(NULL);
            return;
        };
#ifdef UTF8
        memset(&cs, 0, sizeof(cs));
#endif
        
        while (fgets(buf,BUFFER_SIZE,output))
        {
            do_in_MUD_colors(buf,1);
#ifdef UTF8
            local_to_utf8(ustr, buf, BUFFER_SIZE, &cs);
            user_textout(ustr);
#else
            user_textout(buf);
#endif
        }
        fclose(output);
        if (ses->mesvar[9])
            tintin_puts1("#OK COMMAND EXECUTED.", ses);
    }
    else
        tintin_eprintf(ses,"#EXECUTE WHAT COMMAND?");
    prompt(NULL);

}

/**********************/
/* the #shell command */
/**********************/
void shell_command(char *arg,struct session *ses)
{
    char cmd[BUFFER_SIZE*4];

    get_arg(arg, arg, 1, ses);
    if (*arg)
    {
        if (ses->mesvar[9])
            tintin_puts1("#EXECUTING SHELL COMMAND.", ses);
#ifdef UTF8
        utf8_to_local(cmd, arg);
#endif
        if (ui_own_output)
            user_pause();
#ifdef UTF8
        system(cmd);
#else
        system(arg);
#endif
        if (ui_own_output)
            user_resume();
        if (ses->mesvar[9])
            tintin_puts1("#OK COMMAND EXECUTED.", ses);
    }
    else
        tintin_eprintf(ses,"#EXECUTE WHAT COMMAND?");
    prompt(NULL);

}


/********************/
/* the #zap command */
/********************/
struct session* zap_command(char *arg, struct session *ses)
{
    int flag=(ses==activesession);

    if (*arg)
    {
        tintin_eprintf(ses, "#ZAP <ses> is still unimplemented."); /* FIXME */
        return ses;
    }
    if (ses!=nullsession)
    {
    	if(ses->closing)
    	{
    	    if (ses->closing==-1)
    	    	tintin_eprintf(ses, "#You can't use #ZAP from here.");
    	    return ses;
    	}
        tintin_puts("#ZZZZZZZAAAAAAAAPPPP!!!!!!!!! LET'S GET OUTTA HERE!!!!!!!!", ses);
        ses->closing=1;
        do_hook(ses, HOOK_ZAP, 0, 1);
        ses->closing=0;
        cleanup_session(ses);
        return flag?newactive_session():activesession;
    }
    else
        end_command("end", (struct session *)NULL);
    return 0;   /* stupid lint */
}

void news_command(char *arg, struct session *ses)
{
    char line[BUFFER_SIZE];
    FILE* news=fopen( NEWS_FILE , "r");
#ifdef DATA_PATH
    if (!news)
        news=fopen( DATA_PATH "/" NEWS_FILE , "r");
#endif
    if (news)
    {
        tintin_printf(ses,"~2~");
        while (fgets(line,BUFFER_SIZE,news))
        {
            *(char *)strchr(line,'\n')=0;
            tintin_printf(ses,"%s",line);
        }
        tintin_printf(ses,"~7~");
    }
    else
#ifdef DATA_PATH
        tintin_eprintf(ses,"#'%s' file not found in '%s'",
            NEWS_FILE, DATA_PATH);
#else
        tintin_eprintf(ses,"#'%s' file not found!", NEWS_FILE);
#endif
    prompt(ses);
}


#if 0
/*********************************************************************/
/*   tablist will display the all items in the tab completion file   */
/*********************************************************************/
void tablist(struct completenode *tcomplete)
{
    int count, done;
    char tbuf[BUFFER_SIZE];
    struct completenode *tmp;

    done = 0;
    if (tcomplete == NULL)
    {
        tintin_eprintf(0,"Sorry.. But you have no words in your tab completion file");
        return;
    }
    count = 1;
    *tbuf = '\0';

    /*
       I'll search through the entire list, printing thre names to a line then
       outputing the line.  Creates a nice 3 column effect.  To increase the # 
       if columns, just increase the mod #.  Also.. decrease the # in the %s's
     */

    for (tmp = tcomplete->next; tmp != NULL; tmp = tmp->next)
    {
        if ((count % 3))
        {
            if (count == 1)
                sprintf(tbuf, "%25s", tmp->strng);
            else
                sprintf(tbuf, "%s%25s", tbuf, tmp->strng);
            done = 0;
            ++count;
        }
        else
        {
            tintin_printf(0,"%s%25s", tbuf, tmp->strng);
            done = 1;
            *tbuf = '\0';
            ++count;
        }
    }
    if (!done)
        tintin_printf(0,"%s",tbuf);
    prompt(NULL);
}

void tab_add(char *arg, struct session *ses)
{
    struct completenode *tmp, *tmpold, *tcomplete;
    struct completenode *newt;
    char *newcomp, buff[BUFFER_SIZE];

    tcomplete = complete_head;

    if ((arg == NULL) || (strlen(arg) <= 0))
    {
        tintin_puts("Sorry, you must have some word to add.", NULL);
        prompt(NULL);
        return;
    }
    get_arg(arg, buff, 1, ses);

    if ((newcomp = (char *)(malloc(strlen(buff) + 1))) == NULL)
    {
        user_done();
        fprintf(stderr, "Could not allocate enough memory for that Completion word.\n");
        exit(1);
    }
    strcpy(newcomp, buff);
    tmp = tcomplete;
    while (tmp->next != NULL)
    {
        tmpold = tmp;
        tmp = tmp->next;
    }

    if ((newt = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL)
    {
        user_done();
        fprintf(stderr, "Could not allocate enough memory for that Completion word.\n");
        exit(1);
    }
    newt->strng = newcomp;
    newt->next = NULL;
    tmp->next = newt;
    tmp = newt;
    sprintf(buff, "#New word %s added to tab completion list.", arg);
    tintin_puts(buff, NULL);
    prompt(NULL);
}

void tab_delete(char *arg, struct session *ses)
{
    struct completenode *tmp, *tmpold, *tmpnext, *tcomplete;
    char s_buff[BUFFER_SIZE], c_buff[BUFFER_SIZE];

    tcomplete = complete_head;

    if ((arg == NULL) || (strlen(arg) <= 0))
    {
        tintin_puts("#Sorry, you must have some word to delete.", NULL);
        prompt(NULL);
        return;
    }
    get_arg(arg, s_buff, 1, ses);
    tmp = tcomplete->next;
    tmpold = tcomplete;
    if (tmpold->strng == NULL)
    {                          /* (no list if the second node is null) */
        tintin_puts("#There are no words for you to delete!", NULL);
        prompt(NULL);
        return;
    }
    strcpy(c_buff, tmp->strng);
    while ((tmp->next != NULL) && (strcmp(c_buff, s_buff) != 0))
    {
        tmpold = tmp;
        tmp = tmp->next;
        strcpy(c_buff, tmp->strng);
    }
    if (tmp->next != NULL)
    {
        tmpnext = tmp->next;
        tmpold->next = tmpnext;
        free(tmp);
        tintin_puts("#Tab word deleted.", NULL);
        prompt(NULL);
    }
    else
    {
        if (strcmp(c_buff, s_buff) == 0)
        {	/* for the last node to delete */
            tmpold->next = NULL;
            free(tmp);
            tintin_puts("#Tab word deleted.", NULL);
            prompt(NULL);
            return;
        }
        tintin_puts("Word not found in list.", NULL);
        prompt(NULL);
    }
}
#endif

void info_command(char *arg, struct session *ses)
{
    char buffer[BUFFER_SIZE], *bptr;
    int actions   = 0;
    int practions = 0;
    int aliases   = 0;
    int vars      = 0;
    int subs      = 0;
    int antisubs  = 0;
    int highs     = 0;
    int locs      = 0;
    int routes    = 0;
    int binds     = 0;
    int pathdirs  = 0;

    actions   = count_list(ses->actions);
    practions = count_list(ses->prompts);
    aliases   = ses->aliases->nval;
    subs      = count_list(ses->subs);
    antisubs  = count_list(ses->antisubs);
    vars      = ses->myvars->nval;
    highs     = count_list(ses->highs);
    binds     = ses->binds->nval;
    pathdirs  = ses->pathdirs->nval;
    {
        int i;
        for (i=0;i<MAX_LOCATIONS;i++)
            if (ses->locations[i])
                locs++;
    }
    routes=count_routes(ses);
    if (ses==nullsession)
        tintin_printf(ses, "Session : {%s}  (null session)", ses->name);
    else
        tintin_printf(ses, "Session : {%s}  Type: %s  %s : {%s}", ses->name,
            ses->issocket?"TCP/IP":"pty", ses->issocket?"Address":
            "Command line", ses->address);
    if (ses->issocket)
        tintin_printf(ses, "MCCP compression : %s", ses->mccp?"enabled":"disabled");
    tintin_printf(ses,"You have defined the following:");
    tintin_printf(ses, "Actions : %d  Promptactions: %d", actions,practions);
    tintin_printf(ses, "Aliases : %d", aliases);
    tintin_printf(ses, "Substitutes : %d  Antisubstitutes : %d", subs,antisubs);
    tintin_printf(ses, "Variables : %d", vars);
    tintin_printf(ses, "Highlights : %d", highs);
    tintin_printf(ses, "Routes : %d between %d locations",routes,locs);
    tintin_printf(ses, "Binds : %d",binds);
    tintin_printf(ses, "Pathdirs : %d",pathdirs);
    tintin_printf(ses, "Flags: echo=%d, speedwalking=%d, blank=%d, verbatim=%d",
        ses->echo, ses->speedwalk, ses->blank, ses->verbatim);
    tintin_printf(ses, " toggle subs=%d, ignore actions=%d, PreSub=%d, verbose=%d",
        ses->togglesubs, ses->ignore, ses->presub, ses->verbose);
    tintin_printf(ses, "Ticker is %s (ticksize=%d, pretick=%d)",
        ses->tickstatus?"enabled":"disabled", ses->tick_size, ses->pretick);
    if (ui_own_output)
    {
        bptr=buffer;
        if (LINES>1 && COLS>0)
            bptr+=sprintf(bptr, "Terminal size: %dx%d", COLS, LINES);
        else
            bptr+=sprintf(bptr, "Terminal size: unknown");
        if (ui_keyboard)
            bptr+=sprintf(bptr, ", keypad: %s", keypad?"alt mode":"cursor/numeric mode");
        if (ui_sep_input)
            bptr+=sprintf(bptr, ", retain: %d", retain);
        tintin_printf(ses, buffer);
    }
    else
        tintin_printf(ses, "Non-fullscreen mode");
#ifdef UTF8
    tintin_printf(ses, "Local charset: %s, remote charset: %s",
        user_charset_name, ses->charset);
    tintin_printf(ses, "Log type: %s, log charset: %s",
        logtypes[ses->logtype], logcs_name(ses->logcharset));
#else
    tintin_printf(ses, "Log type: %s", logtypes[ses->logtype]);
#endif
    if(ses->logfile)
        tintin_printf(ses, "Logging to: {%s}", ses->logname);
    else
        tintin_printf(ses, "Not logging");
    if (ses->debuglogfile)
        tintin_printf(ses, "Debuglog: {%s}", ses->debuglogname);
    if (ses->closing)
    	tintin_printf(ses, "The session has it's closing mark set to %d!", ses->closing);
    prompt(ses);
}

int isnotblank(char *line,int flag)
{
    int c;

    if (!strcmp(line,EMPTY_LINE))
        return 0;
    if (flag)
        return 1;
    if (!*line)
        return 0;
    while (*line)
        if (*line=='~')
            if (!getcolor(&line,&c,1))
                return 1;
            else
                line++;
        else
            if (isspace(*line))
                line++;
            else
               return 1;
    return 0;
}

int iscompleteprompt(char *line)
{
    int c=7;
    char ch=' ';
    
    for (;*line;line++)
        if (*line=='~')
        {
            if (!getcolor(&line,&c,1))
                ch='~';
        }
        else
            if (!isspace(*line))
                ch=*line;
    return (strchr("?:>.*$#]&)",ch) && !((c==-1)?0:c&0x70));
}


/******************************/
/* the #dosubstitutes command */
/******************************/
void dosubstitutes_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left || !*right)
        tintin_eprintf(ses,"#Syntax: #dosubstitutes <var> <text>");
    else
    {
        do_all_sub(right, ses);
        set_variable(left,right,ses);
    }
}

/*****************************/
/* the #dohighlights command */
/*****************************/
void dohighlights_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left || !*right)
        tintin_eprintf(ses,"#Syntax: #dohighlights <var> <text>");
    else
    {
        do_all_high(right, ses);
        set_variable(left,right,ses);
    }
}

/***************************/
/* the #decolorize command */
/***************************/
void decolorize_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE], *a, *b;
    int c;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left || !*right)
        tintin_eprintf(ses,"#Syntax: #decolorize <var> <text>");
    else
    {
        b=right;
        for (a=right;*a;a++)
            if (*a=='~')
            {
                if (!getcolor(&a,&c,1))
                    *b++='~';
            }
            else
                *b++=*a;
        *b=0;
        set_variable(left,right,ses);
    }
}

/*********************/
/* the #atoi command */
/*********************/
void atoi_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE], *a;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left)
        tintin_eprintf(ses,"#Syntax: #atoi <var> <text>");
    else
    {
        if (*(a=right)=='-')
            a++;
        for (;isdigit(*a);a++);
        *a=0;
        if ((a==right+1) && (*right=='-'))
            *right=0;
        set_variable(left,right,ses);
    }
}

/***********************/
/* the #remark command */
/***********************/
void remark_command(char *arg, struct session *ses)
{
}

/***********************/
/* the #nop(e) command */
/***********************/
/*
 I received a _bug_ report that this command is named "nop" not "nope".
 Even though that's a ridiculous idea, it won't hurt those of us who
 can spell. :p
*/
void nope_command(char *arg, struct session *ses)
{
}

void else_command(char *arg, struct session *ses)
{
    tintin_eprintf(ses, "#ELSE WITHOUT IF.");
}

void elif_command(char *arg, struct session *ses)
{
    tintin_eprintf(ses, "#ELIF WITHOUT IF.");
}

void killall_command(char *arg, struct session *ses)
{
    kill_all(ses, CLEAN);
}

/****************************/
/* the #timecommand command */
/****************************/
void timecommands_command(char *arg, struct session *ses)
{
    struct timeval tv1,tv2;
    char sec[BUFFER_SIZE],usec[BUFFER_SIZE],right[BUFFER_SIZE];
    
    arg = get_arg(arg, sec, 0, ses);
    arg = get_arg(arg, usec, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*right)
    {
        tintin_eprintf(ses,"#Syntax: #timecommand <sec> <usec> <command>");
        return;
    }
    gettimeofday(&tv1, 0);
    parse_input(right,1,ses);
    gettimeofday(&tv2, 0);
    tv2.tv_sec-=tv1.tv_sec;
    tv2.tv_usec-=tv1.tv_usec;
    if (tv2.tv_usec<0)
    {
        tv2.tv_sec--;
        tv2.tv_usec+=1000000;
    }
    if (*sec || *usec)
    {
        if (*sec)
        {
            sprintf(right, "%d", (int)tv2.tv_sec);
            set_variable(sec, right, ses);
        }
        if (*usec)
        {
            sprintf(right, "%d", (int)tv2.tv_usec);
            set_variable(usec, right, ses);
        }
    }
    else
        tintin_printf(ses, "#Time elapsed: %d.%06d", (int)tv2.tv_sec, (int)tv2.tv_usec);
}

#ifdef UTF8
/************************/
/* the #charset command */
/************************/
void charset_command(char *arg, struct session *ses)
{
    struct charset_conv nc;

    get_arg(arg, arg, 1, ses);
    
    if (!*arg)
    {
        tintin_printf(ses, "#Remote charset: %s", ses->charset);
        return;
    }
    if (!new_conv(&nc, arg, 0))
    {
        tintin_eprintf(ses, "#No such charset: {%s}", arg);
        return;
    }
    SFREE(ses->charset);
    ses->charset=mystrdup(arg);
    if (ses!=nullsession)
    {
        cleanup_conv(&ses->c_io);
        memcpy(&ses->c_io, &nc, sizeof(nc));
    }
    else
        cleanup_conv(&nc);
    tintin_printf(ses, "#Charset set to %s", arg);
}
#endif


/********************/
/* the #chr command */
/********************/
void chr_command(char *arg, struct session *ses)
{
    char destvar[BUFFER_SIZE], left[BUFFER_SIZE], *lp;
    char res[BUFFER_SIZE], *r;
#ifdef UTF8
    WC v;
#else
    unsigned int v;
#endif
    
    arg=get_arg(arg, destvar, 0, ses);
    if (!*destvar)
    {
        tintin_eprintf(ses, "#Syntax: #chr <var> <char> [...]");
        return;
    }
    r=res;
    while(*arg)
    {
        arg=get_arg(arg, left, 0, ses);
        lp=left;
        while(*lp)
        {
            v=0;
            if (*lp=='u' || *lp=='U')
            {
                lp++;
                if (*lp=='+')
                    lp++;
            hex:
                switch(*lp)
                {
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    v=v*16 + *lp++-'0';
                    goto hex;
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                    v=v*16 + *lp+++10-'a';
                    goto hex;
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                    v=v*16 + *lp+++10-'A';
                    goto hex;
                case 0: case ' ': case '\t':
                    break;
                default:
                    tintin_eprintf(ses, "#chr: not a valid hex number in {%s}", left);
                    return;
                }
            }
            else if (lp[0]=='0' && lp[1]=='x')
            {
                lp+=2;
                goto hex;
            }
            else
            {
                while(isadigit(*lp))
                    v=v*10 + *lp++-'0';
                if (*lp!=0 && *lp!=' ' && *lp!='\t')
                {
                    tintin_eprintf(ses, "#chr: not a valid number in {%s}", left);
                    return;
                }
            }
            if (!v)
            {
                tintin_eprintf(ses, "#chr: can't represent 0 in {%s}", left);
                return;
            }
#ifdef UTF8
            if (v>0x10ffff)
            {
                tintin_eprintf(ses, "#chr: not an Unicode value -- got %d=0x%x in {%s}",
                    v, v, left);
                return;
            }
            r+=wc_to_utf8(r, &v, 1, res-r+BUFFER_SIZE);
#else
            if (v>255)
            {
                tintin_eprintf(ses, "#chr: only 8-bit values allowed -- got %d=0x%x in {%s}",
                    v, v, left);
                return;
            }
            if (r-res<BUFFER_SIZE-1)
                *r++=v;
#endif
            lp=space_out(lp);
        }
    }
    *r=0;
    set_variable(destvar, res, ses);
}


/********************/
/* the #ord command */
/********************/
void ord_command(char *arg, struct session *ses)
{
    char destvar[BUFFER_SIZE], left[BUFFER_SIZE], res[BUFFER_SIZE], *r;
    WC right[BUFFER_SIZE], *cptr;
    
    arg=get_arg(arg, destvar, 0, ses);
    if (!*destvar)
    {
        tintin_eprintf(ses, "#Syntax: #ord <var> <string>");
        return;
    }
    r=res;
    get_arg(arg, left, 1, ses);
    utf8_to_wc(right, left, BUFFER_SIZE-1);
    for(cptr=right; *cptr; cptr++)
    {
        if (r-res<BUFFER_SIZE-9)
            r+=sprintf(r, " %u", (unsigned int)*cptr);
        else
        {
            tintin_eprintf(ses, "#ord: result too long");
            goto end;
        }
    }
end:
    if (r==res)
    {
        set_variable(destvar, "", ses);
        return;
    }
    *r=0;
    set_variable(destvar, res+1, ses);
}


/***********************/
/* the #hexord command */
/***********************/
void hexord_command(char *arg, struct session *ses)
{
    char destvar[BUFFER_SIZE], left[BUFFER_SIZE], res[BUFFER_SIZE], *r;
    WC right[BUFFER_SIZE], *cptr;
    
    arg=get_arg(arg, destvar, 0, ses);
    if (!*destvar)
    {
        tintin_eprintf(ses, "#Syntax: #hexord <var> <string>");
        return;
    }
    r=res;
    get_arg(arg, left, 1, ses);
    utf8_to_wc(right, left, BUFFER_SIZE-1);
    for(cptr=right; *cptr; cptr++)
    {
        if (r-res<BUFFER_SIZE-9)
            r+=sprintf(r, " U+%04X", (unsigned int)*cptr);
        else
        {
            tintin_eprintf(ses, "#hexord: result too long");
            goto end;
        }
    }
end:
    if (r==res)
    {
        set_variable(destvar, "", ses);
        return;
    }
    *r=0;
    set_variable(destvar, res+1, ses);
}

/*******************/
/* the #ord inline */
/*******************/
int ord_inline(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE];
    WC ch[2];

    get_arg(arg, left, 0, ses);
    if (!*left)
    {
        tintin_eprintf(ses, "#ord: no argument");
        return 0;
    }
    utf8_to_wc(ch, left, 1);
    return ch[0];
}
