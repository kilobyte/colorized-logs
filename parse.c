/* $Id: parse.c,v 2.8 1999/01/19 08:46:22 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: parse.c - some utility-functions                            */
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
#include <signal.h>
#include "tintin.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

struct session *parse_tintin_command(char *command, char *arg,struct session *ses);
void do_speedwalk(char *cp, struct session *ses);
char *get_arg_with_spaces(char *s, char *arg);
extern struct listnode *searchnode_list_begin(struct listnode *listhead, char *cptr, int mode);

extern void            action_command(char *arg, struct session *ses);
extern struct session *all_command(char *arg, struct session *ses);
extern struct session *read_command(char *filename, struct session *ses);
extern struct session *session_command(char *arg,struct session *ses);
extern struct session *status_command(char*arg,struct session *ses);
extern struct session *run_command(char *arg,struct session *ses);
extern void            unhighlight_command(char *arg, struct session *ses);
extern void            promptaction_command(char *arg, struct session *ses);
extern void            unpromptaction_command(char *arg, struct session *ses);
extern void            write_command(char *filename, struct session *ses);
extern void            writesession_command(char *filename, struct session *ses);
extern struct session *zap_command(struct session *ses);
extern void            alias_command(char *arg, struct session *ses);
extern void            unalias_command(char *arg, struct session *ses);
extern void            bell_command(struct session *ses);
extern void            bind_command(char *arg, struct session *ses);
extern void            unbind_command(char *arg, struct session *ses);
extern void            blank_command(char *arg,struct session *ses);
extern void            char_command(char *arg,struct session *ses);
extern void            check_insert_path(char *command, struct session *ses, int force);
extern void            condump_command(char *arg, struct session *ses);
extern void            cr_command(struct session *ses);
extern void            ctime_command(char *arg,struct session *ses);
extern void            deathlog_command(char *arg, struct session *ses);
extern void            delay_command(char *arg, struct session *ses);
extern void            deleteitems_command(char *arg,struct session *ses);
extern void            display_info(struct session *ses);
extern void            echo_command(char *arg,struct session *ses);
extern void            end_command(char *command, struct session *ses);
extern void            explode_command(char *arg, struct session *ses);
extern void            finditem_command(char *arg,struct session *ses);
extern void            firstupper_command(char *arg,struct session *ses);
extern void            foreach_command(char *arg,struct session *ses);
extern void            getitem_command(char *arg,struct session *ses);
extern void            goto_command(char *arg,struct session *ses);
extern void            help_command(char *arg);
extern void            history_command(struct session *ses);
extern void            if_command(char *line, struct session *ses);
extern void            ignore_command(char *arg,struct session *ses);
extern void            implode_command(char *arg, struct session *ses);
extern void            isatom_command(char *line,struct session *ses);
extern void            keypad_command(char *arg,struct session *ses);
extern void            kill_all(struct session *ses, int mode);
extern void            listlength_command(char *arg,struct session *ses);
extern void            deathlog_command(char *arg, struct session *ses);
extern void            log_command(char *arg, struct session *ses);
extern void            loop_command(char *arg, struct session *ses);
extern void            map_command(char *arg,struct session *ses);
extern void            unmap_command(struct session *ses);
extern void            showpath_command(struct session *ses);
extern void            margins_command(char *arg,struct session *ses);
extern void            mark_command(struct session *ses);
extern void            math_command(char *line, struct session *ses);
extern void            message_command(char *arg,struct session *ses);
extern void            mudcolors_command(char *arg,struct session *ses);
extern void            news_command(struct session *ses);
extern void            parse_antisub(char *arg, struct session *ses);
extern void            parse_high(char *arg, struct session *ses);
extern void            parse_sub(char *arg,int gag,struct session *ses);
extern void            path_command(char *arg, struct session *ses);
extern void            savepath_command(char *arg, struct session *ses);
extern void            pathdir_command(char *arg,struct session *ses);
extern void            postpad_command(char *arg,struct session *ses);
extern void            prepad_command(char *arg,struct session *ses);
extern void            presub_command(char *arg,struct session *ses);
extern void            random_command(char *arg,struct session *ses);
extern void            read_file(char *arg, struct session *ses);
extern void            return_command(char *arg,struct session *ses);
extern void            reverse_command(char *arg,struct session *ses);
extern void            route_command(char *arg,struct session *ses);
extern void            unroute_command(char *arg,struct session *ses);
extern void            savepath_command(char *arg, struct session *ses);
extern void            showme_command(char *arg,struct session *ses);
extern void            snoop_command(char *arg,struct session *ses);
extern void            speedwalk_command(char *arg,struct session *ses);
extern void            splitlist_command(char *arg,struct session *ses);
extern void            strip_command(char *arg,struct session *ses);
extern void            strlen_command(char *arg, struct session *ses);
extern void            substr_command(char *arg,struct session *ses);
extern void            shell_command(char *arg,struct session *ses);
extern void            system_command(char *arg,struct session *ses);
extern void            tick_command(struct session *ses);
extern void            tickoff_command(struct session *ses);
extern void            tickon_command(struct session *ses);
extern void            ticksize_command(char *arg,struct session *ses);
extern void            ctime_command(char *arg,struct session *ses);
extern void            ctime_command(char *arg,struct session *ses);
extern void            time_command(char *arg,struct session *ses);
extern void            togglesubs_command(char *arg,struct session *ses);
extern void            tolower_command(char *arg,struct session *ses);
extern void            toupper_command(char *arg,struct session *ses);
extern void            unaction_command(char *arg, struct session *ses);
extern void            unalias_command(char *arg, struct session *ses);
extern void            unantisubstitute_command(char *arg, struct session *ses);
extern void            unbind_command(char *arg, struct session *ses);
extern void            unlink_file(char *arg, struct session *ses);
extern void            unmap_command(struct session *ses);
extern void            unroute_command(char *arg,struct session *ses);
extern void            unsubstitute_command(char *arg,int gag,struct session *ses);
extern void            unvar_command(char *arg,struct session *ses);
extern void            var_command(char *arg,struct session *ses);
extern void            verbatim_command(char *arg,struct session *ses);
extern void            version_command(void);
extern void            remove_command(char *arg, struct session *ses);
extern void            dosubstitutes_command(char *arg,struct session *ses);
extern void            dohighlights_command(char *arg,struct session *ses);
extern void            sortlist_command(char *arg,struct session *ses);
extern void            match_command(char *arg, struct session *ses);
extern void            decolorize_command(char *arg,struct session *ses);
extern void            atoi_command(char *arg,struct session *ses);

extern int is_abrev(char *s1, char *s2);
extern int is_speedwalk_dirs(char *cp);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);
extern void tintin_printf(struct session *ses,char *format,...);
extern void write_line_mud(char *line, struct session *ses);
extern int do_goto(char *txt,struct session *ses);
extern void do_out_MUD_colors(char *line);
extern struct completenode *complete_head;
extern struct listnode *searchnode_list(struct listnode *listhead, char *cptr);
extern char *space_out(char *s);
extern char *get_arg_in_braces(char *s,char *arg,int flag);
void write_com_arg_mud(char *command, char *argument, int nsp, struct session *ses);
void prompt(struct session *ses);

extern struct session *sessionlist, *activesession, *nullsession;
extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */
extern char tintin_char, verbatim_char;
extern int term_echoing, verbatim;
extern int speedwalk;
extern char *get_arg_stop_spaces(char *s, char *arg);
extern char *get_command(char *s, char *arg);
extern char *cryptkey;
extern char *get_arg_all(char *s, char *arg);
extern void tstphandler(int sig);
int in_alias=0;
extern int in_read;
extern int aborting;

/**************************************************************************/
/* parse input, check for TINTIN commands and aliases and send to session */
/**************************************************************************/
struct session *parse_input(char *input,int override_verbatim,struct session *ses)
{
    char command[BUFFER_SIZE], arg[BUFFER_SIZE], result[BUFFER_SIZE];
    char *a,*b;
    struct listnode *ln;
    int nspaces;

    if (!ses->server_echo && activesession == ses)
        term_echoing = 1;
    if (*input == '\0')
    {
        if (ses!=nullsession)
            write_line_mud("", ses);
        else
        {
            if (!in_read)
                write_com_arg_mud("", "", 0, ses);
        }
        return ses;
    }
    if ((*input==tintin_char) && is_abrev(input + 1, "verbatim"))
    {
        verbatim_command("",ses);
        return ses;
    }
    if (verbatim && !override_verbatim && (ses!=nullsession))
    {
        write_line_mud(input, ses);
        return ses;
    }
    if (*input==verbatim_char && (ses!=nullsession))
    {
        input++;
        write_line_mud(input, ses);
        return ses;
    }

    while (*input)
    {
        while (*input == ';')
            input=space_out(input+1);
        if (pvars)
        {
            input = get_command(input, command);
            substitute_vars( command, result);
            substitute_myvars( result, command, ses);
            nspaces=0;
            while (*input==' ')
            {
                input++;
                nspaces++;
            }
            input = get_arg_all(input, arg);
            substitute_vars( arg, result);
            substitute_myvars( result, arg, ses);
        }
        else
        {
            input = get_command(input, result);            
            substitute_myvars( result, command, ses);
            nspaces=0;
            while (*input==' ')
            {
                input++;
                nspaces++;
            }
            input = get_arg_all(input, result);
            substitute_myvars( result, arg, ses);
        }

        if (in_alias)
        {
            if (!*input && *(*pvars)[0])
            {
                if (strlen(arg)+1+strlen((*pvars)[0]) > BUFFER_SIZE-10)
                {
                    if (!aborting)
                    {
                        tintin_printf(ses,"#ERROR: arguments too long in %s %s %s",command,arg,(*pvars)[0]);
                        aborting=1;
                    }
                }
                if (*arg)
                    strcat(arg," ");
                strcat(arg,(*pvars)[0]);
            };
            in_alias=0;
        }
        if (aborting)
        {
            aborting=0;
            return ses;
        }
    
        if (*command == tintin_char)
            ses = parse_tintin_command(command + 1, arg, ses);
        else if ((ln = searchnode_list_begin(ses->aliases, command, ALPHA)) != NULL)
        {
            int i;
            char *cpsource;
            pvars_t vars,*lastpvars;

            strcpy(vars[0], arg);

            for (i = 1, cpsource = arg; i < 10; i++)
                cpsource=get_arg_in_braces(cpsource,vars[i],0);
            in_alias=1;
            lastpvars=pvars;
            pvars=&vars;
            strcpy(arg, ln->right); /* alias can #unalias itself */
            ses = parse_input(arg,1,ses);
            pvars=lastpvars;
        }
        else if (speedwalk && !*arg && is_speedwalk_dirs(command))
            do_speedwalk(command, ses);
        else
#ifdef GOTO_CHAR
            if ((*arg)||!do_goto(command,ses))
#endif
            {
                get_arg_with_spaces(arg, arg);
                write_com_arg_mud(command, arg, nspaces, ses);
            }
    }
    aborting=0;
    return (ses);
}

/************************************************************************/
/* return TRUE if commands only consists of lowercase letters N,S,E ... */
/************************************************************************/
int is_speedwalk_dirs(char *cp)
{
    int flag;

    flag = FALSE;
    while (*cp)
    {
        if (*cp != 'n' && *cp != 'e' && *cp != 's' && *cp != 'w' && *cp != 'u' && *cp != 'd' &&
                !isdigit(*cp))
            return FALSE;
        if (!isdigit(*cp))
            flag = TRUE;
        cp++;
    }
    return flag;
}

/**************************/
/* do the speedwalk thing */
/**************************/
void do_speedwalk(char *cp, struct session *ses)
{
    char sc[2];
    char *loc;
    int multflag, loopcnt, i;

    strcpy(sc, "x");
    while (*cp)
    {
        loc = cp;
        multflag = FALSE;
        while (isdigit(*cp))
        {
            cp++;
            multflag = TRUE;
        }
        if (multflag && *cp)
        {
            sscanf(loc, "%d%c", &loopcnt, sc);
            i = 0;
            while (i++ < loopcnt)
                write_com_arg_mud(sc, "", 0, ses);
        }
        else if (*cp)
        {
            sc[0] = *cp;
            write_com_arg_mud(sc, "", 0, ses);
        }
        /* Added the if to make sure we didn't move the pointer outside the
           bounds of the origional pointer.  Corrects the bug with speedwalking
           where if you typed "u7" tintin would go apeshit. (JE)
         */
        if (*cp)
            cp++;
    }
}


int do_goto(char *txt,struct session *ses)
{
    char *ch;

    if (!(ch=strchr(txt,GOTO_CHAR)))
        return 0;
    if (ch+1>=strchr(txt,0))
        return 0;
    if (ch!=txt)
    {
        *ch=' ';
        goto_command(txt,ses);
    }
    else
    {
        char tmp[BUFFER_SIZE];
        struct listnode *ln;

        if (!(ln=searchnode_list(ses->myvars, "loc"))||(!*ln->right))
        {
            tintin_printf(ses,"#Cannot goto from $loc, it is not set!");
            return 1;
        }
        sprintf(tmp,"{%s} {%s}",ln->right,txt+1);
        goto_command(tmp,ses);
    }
    return 1;
}

/*************************************/
/* parse most of the tintin-commands */
/*************************************/
struct session *parse_tintin_command(char *command, char *arg,struct session *ses)
{
    struct session *sesptr;

    for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
        if ((strcmp(sesptr->name, command) == 0)&&(sesptr!=nullsession))
        {
            if (*arg)
            {
                get_arg_with_spaces(arg, arg);
                parse_input(arg,1, sesptr);	/* was: #sessioname commands */
                return (ses);
            }
            else
            {
                activesession = sesptr;
                tintin_printf(ses, "#SESSION '%s' ACTIVATED.", sesptr->name);
                return (sesptr);
            }
        }
    if (isdigit(*command))
    {
        int i = atoi(command);

        if (i > 0)
        {
            get_arg_in_braces(arg, arg, 1);
            while (i-- > 0)
                ses = parse_input(arg,1, ses);
        }
        else
        {
            tintin_printf(ses,"#Cannot repeat a command a non-positive number of times.");
            prompt(ses);
        }
        return (ses);
    }
        
    else if (is_abrev(command, "verbatim")) /* possible in blah;#verb */
        verbatim_command(arg, ses);
        
    else if (is_abrev(command, "action"))
        action_command(arg, ses);

    else if (is_abrev(command, "alias"))
        alias_command(arg, ses);

    else if (is_abrev(command, "all"))
        ses = all_command(arg, ses);

    else if (is_abrev(command, "antisubstitute"))
        parse_antisub(arg, ses);

    else if (is_abrev(command, "bell"))
        bell_command(ses);

    else if (is_abrev(command, "bind"))
        bind_command(arg, ses);

    else if (is_abrev(command, "blank"))
        blank_command(arg,ses);

    else if (is_abrev(command, "char"))
        char_command(arg, ses);

#ifdef UI_FULLSCREEN
    else if (is_abrev(command, "condump"))
        condump_command(arg, ses);
#endif

    else if (is_abrev(command, "ctime"))
        ctime_command(arg, ses);

    else if (is_abrev(command, "cr"))
        cr_command(ses);

    else if (is_abrev(command, "delay"))
        delay_command(arg, ses);
    else if (is_abrev(command, "event"))
        delay_command(arg, ses);

    else if (is_abrev(command, "deleteitems"))
        deleteitems_command(arg, ses);

    else if (is_abrev(command, "deathlog"))
        deathlog_command(arg, ses);

    else if (is_abrev(command, "echo"))
        echo_command(arg,ses);

    else if (is_abrev(command, "end"))
        end_command(command, ses);

    else if (is_abrev(command, "explode"))
        explode_command(arg, ses);

    else if (is_abrev(command, "foreach"))
        foreach_command(arg, ses);

    else if (is_abrev(command, "firstupper"))
        firstupper_command(arg, ses);

    else if (is_abrev(command, "help"))
        help_command(arg);

    else if (is_abrev(command, "highlight"))
        parse_high(arg, ses);

    else if (is_abrev(command, "history"))
        history_command(ses);

    else if (is_abrev(command, "if"))
        if_command(arg, ses);

    else if (is_abrev(command, "else"))
        tintin_printf(ses,"#ELSE WITHOUT IF.");

    else if (is_abrev(command, "elif"))
        tintin_printf(ses,"#ELIF WITHOUT IF.");

    else if (is_abrev(command, "finditem"))
        finditem_command(arg, ses);

    else if (is_abrev(command, "getitem"))
        getitem_command(arg, ses);

    else if (is_abrev(command, "goto"))
        goto_command(arg, ses);

    else if (is_abrev(command, "ignore"))
        ignore_command(arg, ses);

    else if (is_abrev(command, "implode"))
        implode_command(arg, ses);
    else if (is_abrev(command, "info"))
        display_info(ses);

    else if (is_abrev(command, "isatom"))
        isatom_command(arg, ses);

    else if (is_abrev(command, "keypad"))
        keypad_command(arg, ses);

    else if (is_abrev(command, "killall"))
        kill_all(ses, CLEAN);

    else if (is_abrev(command, "listlength"))
        listlength_command(arg, ses);

    else if (is_abrev(command, "log"))
        log_command(arg, ses);

    else if (is_abrev(command, "loop"))
        loop_command(arg, ses);

            /*
               some people think it's not #No-OPeration,
               but something else.  It doesn't hurt to check
               for the longer version.
            */
    else if (is_abrev(command, "nope")) ;

    else if (is_abrev(command, "map"))
        map_command(arg, ses);

#ifdef UI_FULLSCREEN
    else if (is_abrev(command, "margins"))
        margins_command(arg, ses);
#endif

    else if (is_abrev(command, "math"))
        math_command(arg, ses);
        
    else if (is_abrev(command, "match"))
        match_command(arg, ses);

    else if (is_abrev(command, "mark"))
        mark_command(ses);

    else if (is_abrev(command, "messages"))
        message_command(arg, ses);

    else if (is_abrev(command, "mudcolors"))
        mudcolors_command(arg, ses);

    else if (is_abrev(command, "news"))
        news_command(ses);

    else if (is_abrev(command, "path"))
        showpath_command(ses);

    else if (is_abrev(command, "pathdir"))
        pathdir_command(arg, ses);

    else if (is_abrev(command, "presub"))
        presub_command(arg, ses);

    else if (is_abrev(command, "prepad"))
        prepad_command(arg, ses);

    else if (is_abrev(command, "promptaction"))
        promptaction_command(arg, ses);

    else if (is_abrev(command, "postpad"))
        postpad_command(arg, ses);

    else if (is_abrev(command, "random"))
        random_command(arg, ses);

    else if (is_abrev(command, "remark")) ;
    
    else if (is_abrev(command, "removeevent"))
        remove_command(arg, ses);
    else if (is_abrev(command, "unevent"))
        remove_command(arg, ses);
    
    /*
      else if (is_abrev(command, "retab"))
        read_complete();
    */
    else if (is_abrev(command, "return"))
        return_command(arg, ses);

    else if (is_abrev(command, "read"))
        ses = read_command(arg, ses);

    else if (is_abrev(command, "reverse"))
        reverse_command(arg, ses);

    else if (is_abrev(command, "route"))
        route_command(arg, ses);

    else if (is_abrev(command, "savepath"))
        savepath_command(arg, ses);

    else if (is_abrev(command, "session"))
        ses = session_command(arg, ses);

    else if (is_abrev(command, "run"))
        ses = run_command(arg, ses);

    else if (is_abrev(command, "showme"))
        showme_command(arg, ses);
/*
    else if (is_abrev(command, "showpath"))
        showpath_command(ses);
*/
    else if (is_abrev(command, "snoop"))
        snoop_command(arg, ses);

    else if (is_abrev(command, "speedwalk"))
        speedwalk_command(arg, ses);

    else if (is_abrev(command, "splitlist"))
        splitlist_command(arg, ses);

    else if (is_abrev(command, "strip"))
        strip_command(arg, ses);

    /* CHANGED to allow suspending from within tintin. */
    /* I know, I know, this is a hack *yawn* */
    else if (is_abrev(command, "suspend"))
        tstphandler(SIGTSTP);
        
    else if (is_abrev(command, "shell"))
        shell_command(arg,ses);

    else if (is_abrev(command, "status"))
        status_command(arg,ses);
    /*
      else if (is_abrev(command, "tablist"))
        tablist(complete_head);

      else if (is_abrev(command, "tabadd"))
        tab_add(arg);

      else if (is_abrev(command, "tabdelete"))
        tab_delete(arg);
    */
    else if (is_abrev(command, "textin"))
        read_file(arg, ses);

    else if (is_abrev(command, "time"))
        time_command(arg, ses);

    else if (!strcmp(command, "unlink"))
        unlink_file(arg, ses);
        
    else if (is_abrev(command, "sortlist"))
        sortlist_command(arg, ses);

    else if (is_abrev(command, "substitute"))
        parse_sub(arg, 0, ses);

    else if (is_abrev(command, "gag"))
    {
        if (*arg)
        {
            if (*arg != '{')
            {
                strcpy(command, arg);
                strcpy(arg, "{");
                strcat(arg, command);
                strcat(arg, "} ");
            }
            strcat(arg, " {"EMPTY_LINE"}");
            parse_sub(arg, 1, ses);
        }
        else
            parse_sub("", 1, ses);
    }

    else if (is_abrev(command, "substring"))
        substr_command(arg, ses);

    else if (is_abrev(command, "strlen"))
        strlen_command(arg,ses);

    else if (is_abrev(command, "system"))
        system_command(arg, ses);

    else if (is_abrev(command, "tick"))
        tick_command(ses);

    else if (is_abrev(command, "tickoff"))
        tickoff_command(ses);

    else if (is_abrev(command, "tickon"))
        tickon_command(ses);

    else if (is_abrev(command, "ticksize"))
        ticksize_command(arg, ses);

    else if (is_abrev(command, "tolower"))
        tolower_command(arg, ses);

    else if (is_abrev(command, "togglesubs"))
        togglesubs_command(arg,ses);

    else if (is_abrev(command, "toupper"))
        toupper_command(arg, ses);

    else if (is_abrev(command, "unaction"))
        unaction_command(arg, ses);

    else if (is_abrev(command, "unalias"))
        unalias_command(arg, ses);

    else if (is_abrev(command, "unantisubstitute"))
        unantisubstitute_command(arg, ses);

    else if (is_abrev(command, "unbind"))
        unbind_command(arg, ses);

    else if (is_abrev(command, "unhighlight"))
        unhighlight_command(arg, ses);

    else if (is_abrev(command, "unsubstitute"))
        unsubstitute_command(arg, 0, ses);

    else if (is_abrev(command, "ungag"))
        unsubstitute_command(arg, 1, ses);

    else if (is_abrev(command, "unpath"))
        unmap_command(ses);

    else if (is_abrev(command, "unpromptaction"))
        unpromptaction_command(arg, ses);

    else if (is_abrev(command, "unmap"))
        unmap_command(ses);

    else if (is_abrev(command, "unroute"))
        unroute_command(arg, ses);

    else if (is_abrev(command, "variable"))
        var_command(arg, ses);

    else if (is_abrev(command, "version"))
        version_command();

    else if (is_abrev(command, "unvariable"))
        unvar_command(arg, ses);

    else if (is_abrev(command, "write"))
        write_command(arg, ses);

    else if (is_abrev(command, "writesession"))
        writesession_command(arg, ses);

    else if (is_abrev(command, "zap"))
        ses = zap_command(ses);

    else if (is_abrev(command, "dosubstitutes"))
        dosubstitutes_command(arg, ses);

    else if (is_abrev(command, "dohighlights"))
        dohighlights_command(arg, ses);
        
    else if (is_abrev(command, "decolorize"))
        decolorize_command(arg, ses);
        
    else if (is_abrev(command, "atoi"))
        atoi_command(arg, ses);
                
    else
    {
        tintin_printf(ses,"#UNKNOWN TINTIN-COMMAND: [%c%s]",tintin_char,command);
        prompt(ses);
    }
    return (ses);
}


/**********************************************/
/* get all arguments - don't remove "s and \s */
/**********************************************/
char *get_arg_all(char *s, char *arg)
{
    /* int inside=FALSE; */
    int nest = 0;

    s = space_out(s);
    while (*s)
    {

        if (*s == '\\')
        {
            *arg++ = *s++;
            if (*s)
                *arg++ = *s++;
        }
        else if (*s == ';' && nest < 1)
        {
            break;
        }
        else if (*s == DEFAULT_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == DEFAULT_CLOSE)
        {
            nest--;
            *arg++ = *s++;
        }
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}

/****************************************************/
/* get an inline command - terminated with 0 or ')' */
/****************************************************/
char *get_inline(char *s, char *arg)
{
    /* int inside=FALSE; */
    int nest = 0;

    s = space_out(s);
    while (*s)
    {
        if (*s == '\\')
        {
            *arg++ = *s++;
            if (*s)
                *arg++ = *s++;
        }
        else if (*s == ')' && nest < 1)
            break;
        else if (*s == DEFAULT_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == DEFAULT_CLOSE)
        {
            nest--;
            *arg++ = *s++;
        }
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}

/**************************************/
/* get all arguments - remove "s etc. */
/* Example:                           */
/* In: "this is it" way way hmmm;     */
/* Out: this is it way way hmmm       */
/**************************************/
char *get_arg_with_spaces(char *s, char *arg)
{
    int nest = 0;

    /* int inside=FALSE; */

    s = space_out(s);
    while (*s)
    {

        if (*s == '\\')
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s == ';' && nest == 0)
            break;
        else if (*s == DEFAULT_OPEN)
        {
            nest++;
            *arg++ = *s++;
        }
        else if (*s == DEFAULT_CLOSE)
        {
            *arg++ = *s++;
            nest--;
        }
        else
            *arg++ = *s++;
    }
    *arg = '\0';
    return s;
}

char *get_arg_in_braces(char *s,char *arg,int flag)
{
    int nest = 0;
    char *ptr;

    s = space_out(s);
    ptr = s;
    if (*s != DEFAULT_OPEN)
    {
        if (flag == 0)
            s = get_arg_stop_spaces(ptr, arg);
        else
            s = get_arg_with_spaces(ptr, arg);
        return s;
    }
    s++;
    while (*s != '\0' && !(*s == DEFAULT_CLOSE && nest == 0))
    {
        if (*s=='\\')
            ;
        else
            if (*s == DEFAULT_OPEN)
                nest++;
            else
                if (*s == DEFAULT_CLOSE)
                    nest--;
        *arg++ = *s++;
    }
    if (!*s)
        tintin_printf(0,"#Unmatched braces error! Bad argument is \"%s\".", ptr);
    else
        s++;
    *arg = '\0';
    return s;

}
/**********************************************/
/* get one arg, stop at spaces                */
/* remove quotes                              */
/**********************************************/
char *get_arg_stop_spaces(char *s, char *arg)
{
    int inside = FALSE;

    s = space_out(s);

    while (*s)
    {
        if (*s == '\\')
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s == '"')
        {
            s++;
            inside = !inside;
        }
        else if (*s == ';')
        {
            if (inside)
                *arg++ = *s++;
            else
                break;
        }
        else if (!inside && *s == ' ')
            break;
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}

/**********************************************/
/* get the command, stop at spaces            */
/* remove quotes                              */
/**********************************************/
char *get_command(char *s, char *arg)
{
    int inside = FALSE;

    while (*s)
    {
        if (*s == '\\')
        {
            if (*++s)
                *arg++ = *s++;
        }
        else if (*s == '"')
        {
            s++;
            inside = !inside;
        }
        else if (*s == ';')
        {
            if (inside)
                *arg++ = *s++;
            else
                break;
        }
        else if (!inside && *s == ' ')
            break;
        else
            *arg++ = *s++;
    }

    *arg = '\0';
    return s;
}

/*********************************************/
/* spaceout - advance ptr to next none-space */
/* return: ptr to the first none-space       */
/*********************************************/
char *space_out(char *s)
{
    while (isspace(*s))
        s++;
    return s;
}

/************************************/
/* send command+argument to the mud */
/************************************/
void write_com_arg_mud(char *command, char *argument, int nsp, struct session *ses)
{
    char outtext[BUFFER_SIZE];
    int i;

    if (ses==nullsession)
    {
        tintin_printf(ses, "#NO SESSION ACTIVE. USE THE %cSESSION COMMAND TO START ONE.", tintin_char);
        prompt(NULL);
    }
    else
    {
        check_insert_path(command, ses, 0);
        strncpy(outtext, command, BUFFER_SIZE);
        if (*argument)
        {
            if (nsp<1)
                nsp=1;
            i=BUFFER_SIZE-1-strlen(outtext);
            while (nsp--)
                strncat(outtext, " ", i),i--;
            strncat(outtext, argument, i);
        }
        do_out_MUD_colors(outtext);
        write_line_mud(outtext, ses);
        outtext[i = strlen(outtext)] = '\n';
        if (ses->logfile)
            if (fwrite(outtext, i + 1, 1, ses->logfile)<1)
            {
                ses->logfile=0;
                tintin_printf(ses, "#WRITE ERROR -- LOGGING DISABLED.  Disk full?");
            };
    }
}


/***************************************************************/
/* show a prompt - mud prompt if we're connected/else just a > */
/***************************************************************/
void prompt(struct session *ses)
{
#if FORCE_PROMPT
    if (ses && !PSEUDO_PROMPT)
        write_line_mud("", ses);
    else tintin_printf(">",ses);
#endif
}
