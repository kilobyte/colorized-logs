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
#include "tintin.h"

#include <stdlib.h>

/* externs */
extern struct session *sessionlist,*activesession,*nullsession;
extern struct completenode *complete_head;
extern char tintin_char;
extern int keypad,retain;
extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */
extern char status[BUFFER_SIZE];
extern int margins,marginl,marginr;
extern FILE *mypopen(char *command);
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
extern void show_status(void);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);
extern void textout(char *txt);
extern void textout_draft(char *txt);
extern void tintin_puts(char *cptr,struct session *ses);
extern void tintin_puts1(char *cptr,struct session *ses);
extern void tintin_printf(struct session *ses, char *format, ...);
extern void tintin_eprintf(struct session *ses, char *format, ...);
extern void user_beep(void);
extern void user_done(void);
extern void user_keypad(int kp);
extern void user_retain(void);
extern void write_line_mud(char *line, struct session *ses);
extern void set_variable(char *left,char *right,struct session *ses);
extern void do_all_high(char *line,struct session *ses);
extern void do_all_sub(char *line, struct session *ses);
#ifdef EXT_INLINE
extern inline int getcolor(char **ptr,int *color,const int flag);
#else
extern int getcolor(char **ptr,int *color,const int flag);
#endif
extern void user_pause(void);
extern void user_resume(void);
extern void kill_all(struct session *ses, int mode);
extern void do_in_MUD_colors(char *txt,int quotetype);

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

void togglebool(int *b, char *arg, struct session *ses, char *msg1, char *msg2)
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
    tintin_printf(ses, "#You are using KBtin %s\n", VERSION_NUM);
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
struct session *all_command(char *arg,struct session *ses)
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
    if (ispunct(*arg) || ((unsigned char)(*arg)>127))
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
        if (sesptr!=nullsession)
            cleanup_session(sesptr);
    }
    ses = NULL;  /* a message from DIKUs... we can leave it though */
    tintin_printf(ses,"TINTIN suffers from bloodlack, and the lack of a beating heart...");
    tintin_printf(ses,"TINTIN is dead! R.I.P.");
    tintin_printf(ses,"Your blood freezes as you hear TINTIN's death cry.");
    user_done();
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
    togglebool(&ses->verbose,arg,ses,
               "#Output from #reads will now be shown.",
               "#The #read command will no longer output messages.");
}

/************************/
/* the #margins command */
/************************/
void margins_command(char *arg,struct session *ses)
{
    int l,r;
    char num[BUFFER_SIZE], *tmp;

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
    /*
    if(ses->logfile)
       fprintf(ses->logfile,"%s\n",result);
    */
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

char *msNAME[]=
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
    if (ses!=activesession)
        return;
    get_arg(arg,arg,1,ses);
    if (*arg)
        strncpy(status,arg,BUFFER_SIZE);
    else
        strcpy(status,EMPTY_LINE);
    show_status();
}


/***********************/
/* the #system command */
/***********************/
void system_command(char *arg,struct session *ses)
{
    get_arg(arg, arg, 1, ses);
    if (*arg)
    {
        FILE *output;
        char buf[BUFFER_SIZE];
        
        if (ses->mesvar[9])
            tintin_puts1("#EXECUTING SHELL COMMAND.", ses);
        if (!(output = mypopen(arg)))
        {
            tintin_puts1("#ERROR EXECUTING SHELL COMMAND.",ses);
            prompt(NULL);
            return;
        };
        while (fgets(buf,BUFFER_SIZE,output))
        {
            do_in_MUD_colors(buf,1);
            textout(buf);
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
    get_arg(arg, arg, 1, ses);
    if (*arg)
    {
        if (ses->mesvar[9])
            tintin_puts1("#EXECUTING SHELL COMMAND.", ses);
        user_pause();
        system(arg);
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
struct session *zap_command(char *arg, struct session *ses)
{
    if (*arg)
    {
        tintin_eprintf(ses, "#ZAP <ses> is still unimplemented."); /* FIXME */
        return ses;
    }
    if (ses!=nullsession)
    {
        tintin_puts("#ZZZZZZZAAAAAAAAPPPP!!!!!!!!! LET'S GET OUTTA HERE!!!!!!!!", ses);
        cleanup_session(ses);
        return newactive_session();
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
    tintin_printf(ses, "Terminal size: %dx%d,  keypad: %s,  retain: %d",
        COLS,LINES,keypad?"alt mode":"cursor/numeric mode",retain);
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
            if (!getcolor(&line,&c,0))
                ch='~';
        }
        else
            if (!isspace(*line))
                ch=*line;
    return (strchr("?:>.*$#]&",ch) && !(c&0x70));
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
    if (!*left || !*right)
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
 I receive a _bug_ report that this command is named "nop" not "nope".
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
