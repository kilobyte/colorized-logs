/* $Id: action.c,v 1.4 1998/10/11 18:51:51 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: action.c - funtions related to the action command           */
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
#include "tintin.h"
#include <stdlib.h>

extern struct session *activesession, *nullsession;
extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */
extern int term_echoing;
extern char tintin_char;
extern int acnum;
extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern char *get_arg(char *s,char *arg,int flag,struct session *ses);
extern struct listnode *search_node_with_wild(struct listnode *listhead, char *cptr);
extern struct listnode *searchnode_list(struct listnode *listhead, char *cptr);
void substitute_vars(char *arg, char *result);
extern void deletenode_list(struct listnode *listhead, struct listnode *nptr);
extern void show_list_action(struct listnode *listhead);
extern void prompt(struct session *ses);
extern void shownode_list_action(struct listnode *nptr);
extern void insertnode_list(struct listnode *listhead, char *ltext, char *rtext, char *prtext, int mode);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
int check_one_action(char *line, char *action, pvars_t *vars, int inside, struct session *ses);
int check_a_action(char *line, char *action, int inside, struct session *ses);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern int is_abrev(char *s1, char *s2);
extern struct session *if_command(char *arg, struct session *ses);
extern void tintin_printf(struct session *ses,char *format,...);
extern void tintin_eprintf(struct session *ses,char *format,...);
extern int in_alias;
extern void debuglog(struct session *ses, const char *format, ...);
extern int match(char *regex, char *string);
int var_len[10];
char *var_ptr[10];
extern int aborting, recursion;
extern char *space_out(char *s);
extern char *mystrdup(char *s);

int inActions=0;
int deletedActions=0;
char *match_start,*match_end;

int kill_action(struct listnode *head,struct listnode *nptr)
{
    if (inActions)
    {
        if (!strcmp(nptr->left,K_ACTION_MAGIC))
            return 0;
        SFREE(nptr->left);
        nptr->left=MALLOC(strlen(K_ACTION_MAGIC)+1);
        strcpy(nptr->left,K_ACTION_MAGIC);
        deletedActions++;
    }
    else
        deletenode_list(head,nptr);
    return 1;
}

void zap_actions(struct session *ses)
{
    struct listnode *l,*ln,*ol;

    ln =(ol=ses->actions)->next;
    while (ln)
        if (strcmp(ln->left,K_ACTION_MAGIC))
            ln=(ol=ln)->next;
        else
        {
            l=ln;
            ol->next=ln=ln->next;
            SFREE(l->left);
            SFREE(l->right);
            SFREE(l->pr);
            LFREE(l);
        }
    ln =(ol=ses->prompts)->next;
    while (ln)
        if (strcmp(ln->left,K_ACTION_MAGIC))
            ln=(ol=ln)->next;
        else
        {
            l=ln;
            ol->next=ln=ln->next;
            SFREE(l->left);
            SFREE(l->right);
            SFREE(l->pr);
            LFREE(l);
        }
    deletedActions=0;
}

/***********************/
/* the #action command */
/***********************/

/*  Priority code added by Joann Ellsworth 2/2/94 */

void action_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    char pr[BUFFER_SIZE];
    struct listnode *myactions, *ln;
    int flag;

    myactions = ses->actions;
    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);
    arg = get_arg_in_braces(arg, pr, 1);
    if (!*pr)
        strcpy(pr, "5");		/* defaults priority to 5 if no value given */
    if (!*left)
    {
        tintin_printf(ses,"#Defined actions:");
        show_list_action(myactions);
        prompt(ses);
    }
    else if (*left && !*right)
    {
        flag=0;
        while ((myactions = search_node_with_wild(myactions, left)) != NULL)
            if (strcmp(myactions->left,K_ACTION_MAGIC))
                shownode_list_action(myactions), flag++;
            prompt(ses);
        if (!flag && ses->mesvar[1])
            tintin_printf(ses,"#That action (%s) is not defined.",left);
        prompt(ses);
    }
    else
    {
        if ((ln = searchnode_list(myactions, left)) != NULL)
            kill_action(myactions, ln);
        insertnode_list(myactions, left, right, pr, PRIORITY);
        if (ses->mesvar[1])
            tintin_printf(ses,"#Ok. {%s} now triggers {%s} @ {%s}", left, right, pr);
        acnum++;
    }
}

/*****************************/
/* the #promptaction command */
/*****************************/
void promptaction_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    char pr[BUFFER_SIZE];
    struct listnode *myprompts, *ln;
    int flag;

    myprompts = ses->prompts;
    arg = get_arg_in_braces(arg, left, 0);
    arg = get_arg_in_braces(arg, right, 1);
    arg = get_arg_in_braces(arg, pr, 1);
    if (!*pr)
        strcpy(pr, "5");		/* defaults priority to 5 if no value given */
    if (!*left)
    {
        tintin_printf(ses,"#Defined prompts:");
        show_list_action(myprompts);
        prompt(ses);
    }
    else if (*left && !*right)
    {
        flag=0;
        while ((myprompts = search_node_with_wild(myprompts, left)))
            if (strcmp(myprompts->left,K_ACTION_MAGIC))
                shownode_list_action(myprompts), flag++;
            prompt(ses);
        if (!flag && ses->mesvar[1])
            tintin_printf(ses,"#That promptaction (%s) is not defined.",left);
        prompt(ses);
    }
    else
    {
        if ((ln = searchnode_list(myprompts, left)) != NULL)
            kill_action(myprompts, ln);
        insertnode_list(myprompts, left, right, pr, PRIORITY);
        if (ses->mesvar[1])
            tintin_printf(ses,"#Ok. {%s} now triggers {%s} @ {%s}", left, right, pr);
    }
}


/*************************/
/* the #unaction command */
/*************************/
void unaction_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode **ptr,*ln;
    int flag;

    arg = get_arg_in_braces(arg, left, 1);
    if (!*left)
    {
        tintin_eprintf(ses, "#Syntax: #unaction <pattern>");
        return;
    }
    flag = 0;
    ptr = &ses->actions->next;
    while (*ptr)
    {
        ln=*ptr;
        if (strcmp(ln->left,K_ACTION_MAGIC)&&match(left,ln->left))
        {
            flag=1;
            if (ses->mesvar[1])
                tintin_printf(ses,"#Ok. {%s} is no longer an action.", ln->left);
            SFREE(ln->left);
            if (inActions)
            {
                ln->left=MALLOC(strlen(K_ACTION_MAGIC)+1);
                strcpy(ln->left,K_ACTION_MAGIC);
                deletedActions++;
                ptr=&(*ptr)->next;
            }
            else
            {
                *ptr=ln->next;
                SFREE(ln->right);
                SFREE(ln->pr);
                LFREE(ln);
            }
        }
        else
            ptr=&(*ptr)->next;
    }
    if (!flag && ses->mesvar[1])    /* is it an error or not? */
        tintin_printf(ses, "#No match(es) found for {%s}", left);
}

/*******************************/
/* the #unpromptaction command */
/*******************************/
void unpromptaction_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];
    struct listnode **ptr,*ln;
    int flag;

    arg = get_arg_in_braces(arg, left, 1);
    if (!*left)
    {
        tintin_eprintf(ses, "#Syntax: #unpromptaction <pattern>");
        return;
    }
    flag = 0;
    ptr = &ses->prompts->next;
    while (*ptr)
    {
        ln=*ptr;
        if (strcmp(ln->left,K_ACTION_MAGIC)&&match(left,ln->left))
        {
            flag=1;
            if (ses->mesvar[1])
                tintin_printf(ses,"#Ok. {%s} is no longer a promptaction.", ln->left);
            SFREE(ln->left);
            if (inActions)
            {
                ln->left=MALLOC(strlen(K_ACTION_MAGIC)+1);
                strcpy(ln->left,K_ACTION_MAGIC);
                deletedActions++;
                ptr=&(*ptr)->next;
            }
            else
            {
                *ptr=ln->next;
                SFREE(ln->right);
                SFREE(ln->pr);
                LFREE(ln);
            }
        }
        else
            ptr=&(*ptr)->next;
    }
    if (!flag && ses->mesvar[1])    /* is it an error or not? */
        tintin_printf(ses, "#No match(es) found for {%s}", left);
}

/**************************************************************************/
/* run throught each of the commands on the right side of an alias/action */
/* expression, call substitute_text() for all commands but #alias/#action */
/**************************************************************************/
void prepare_actionalias(char *string, char *result, struct session *ses)
{
    char arg[BUFFER_SIZE];

    substitute_vars(string, arg);
    substitute_myvars(arg, result, ses);
}

/*************************************************************************/
/* copy the arg text into the result-space, but substitute the variables */
/* %0..%9 with the real variables                                        */
/*************************************************************************/
void substitute_vars(char *arg, char *result)
{
    int nest = 0;
    int numands, n;
    char *ptr,*ARG=arg;
    int valuelen,len=strlen(arg);

    if (!pvars)
    {
        strcpy(result, arg);
        return;
    }
    while (*arg)
    {
        if (*arg == '%')
        {		/* substitute variable */
            numands = 1;        /* at least one */
            while (*(arg + numands) == '%')
                numands++;
            if (isdigit(*(arg + numands)) && numands == (nest + 1))
            {
                n = *(arg + numands) - '0';
                valuelen=strlen((*pvars)[n]);
                if ((len+=valuelen-numands-1) > BUFFER_SIZE-10)
                {
                    len-=valuelen-numands-1;
                    if (!aborting)
                    {                    
                        tintin_eprintf(0,"#ERROR: command+vars too long in {%s}.",ARG);
                        aborting=1;
                    }
                    goto novar1;
                }
                strcpy(result, (*pvars)[n]);
                arg = arg + numands + 1;
                result += valuelen;
            }
            else
            {
novar1:
                strncpy(result, arg, numands + 1);
                arg += numands + 1;
                result += numands + 1;
            }
            in_alias=0; /* not a simple alias */
        }
        if (*arg == '$')
        {		/* substitute variable */
            numands = 1;        /* at least one */
            while (*(arg + numands) == '$')
                numands++;
            if (isdigit(*(arg + numands)) && numands == (nest + 1))
            {
                n = *(arg + numands) - '0';
                valuelen=strlen((*pvars)[n]);
                if ((len+=valuelen-numands-1) > BUFFER_SIZE-10)
                {
                    len-=valuelen-numands-1;
                    if (!aborting)
                    {
                        tintin_eprintf(0,"#ERROR: command+vars too long in {%s}.",ARG);
                        aborting=1;
                    }
                    goto novar2;
                }
                ptr = (*pvars)[n];
                while (*ptr)
                {
                    if (*ptr == ';')
                        ptr++;      /* we don't care if len is too small */
                    else
                        *result++ = *ptr++;
                }
                arg = arg + numands + 1;
            }
            else
            {
novar2:
                strncpy(result, arg, numands);
                arg += numands;
                result += numands;
            }
            in_alias=0; /* not a simple alias */
        }
        else if (*arg == DEFAULT_OPEN)
        {
            nest++;
            *result++ = *arg++;
        }
        else if (*arg == DEFAULT_CLOSE)
        {
            nest--;
            *result++ = *arg++;
        }
        else if (*arg == '\\' && nest == 0)
        {
            while (*arg == '\\')
                *result++ = *arg++;
            if (*arg == '%')
            {
                result--;
                *result++ = *arg++;
                *result++ = *arg++;
            }
        }
        else
            *result++ = *arg++;
    }
    *result = '\0';
}



/**********************************************/
/* check actions from a sessions against line */
/**********************************************/
void check_all_actions(char *line, struct session *ses)
{
    struct listnode *ln;
    pvars_t vars,*lastpvars;

    ln = ses->actions;
    inActions=1;

    while ((ln = ln->next))
    {
        if (check_one_action(line, ln->left, &vars, 0, ses))
        {
            lastpvars = pvars;
            pvars = &vars;
            if (ses->mesvar[1] && activesession == ses)
            {
                char buffer[BUFFER_SIZE];

                prepare_actionalias(ln->right, buffer, ses);
                tintin_printf(ses,"[ACTION: %s]", buffer);
            }
            if (ses->debuglogfile)
                debuglog(ses, "ACTION: {%s}->{%s}", line, ln->right);
            parse_input(ln->right,1,ses);
            recursion=0;
            pvars = lastpvars;
            /*      return;*/		/* KB: we want ALL actions to be done */
        }
    }
    if (deletedActions)
        zap_actions(ses);
    inActions=0;
}


/**********************************************/
/* check actions from a sessions against line */
/**********************************************/
void check_all_promptactions(char *line, struct session *ses)
{
    struct listnode *ln;
    pvars_t vars,*lastpvars;

    ln = ses->prompts;
    inActions=1;

    while ((ln = ln->next))
    {
        if (check_one_action(line, ln->left, &vars, 0, ses))
        {
            lastpvars=pvars;
            pvars=&vars;
            if (ses->mesvar[1] && activesession == ses)
            {
                char buffer[BUFFER_SIZE];

                prepare_actionalias(ln->right, buffer, ses);
                tintin_printf(ses, "[PROMPT-ACTION: %s]", buffer);
            }
            if (ses->debuglogfile)
                debuglog(ses, "PROMPTACTION: {%s}->{%s}", line, ln->right);
            parse_input(ln->right,1,ses);
            recursion=0;
            pvars=lastpvars;
            /*      return;*/		/* KB: we want ALL actions to be done */
        }
    }
    if (deletedActions)
        zap_actions(ses);
    inActions=0;
}


/**********************/
/* the #match command */
/**********************/
void match_command(char *arg, struct session *ses)
{
    pvars_t vars,*lastpvars;
    char left[BUFFER_SIZE], line[BUFFER_SIZE], right[BUFFER_SIZE],
         temp[BUFFER_SIZE];
    int flag=0;
    
    arg=get_arg_in_braces(arg, left, 0);
    arg=get_arg_in_braces(arg, line, 0);
    arg=get_arg_in_braces(arg, right, 0);
    substitute_vars(line, temp);
    substitute_myvars(temp, line, ses);
    
    if (!*left || !*right)
    {
        tintin_eprintf(ses,"#ERROR: valid syntax is: #match <pattern> <line> <command> [#else ...]");
        return;
    }

    if (check_one_action(line, left, &vars, 0, ses))
    {
        lastpvars = pvars;
        pvars = &vars;
        parse_input(right,1,ses);
        pvars = lastpvars;
        flag=1;
    }
    arg=get_arg_in_braces(arg, left, 0);
    if (*left == tintin_char)
    {
        if (is_abrev(left + 1, "else"))
        {
            get_arg_in_braces(arg, right, 1);
            if (!flag)
                parse_input(right,1,ses);
            return;
        }
        if (is_abrev(left + 1, "elif"))
        {
            if (!flag)
                if_command(arg, ses);
            return;
        }
    }
    if (*left)
        tintin_eprintf(ses,"#ERROR: cruft after #match: {%s}",left);
}


/*********************/
/* the #match inline */
/*********************/
int match_inline(char *arg, struct session *ses)
{
    pvars_t vars;
    char left[BUFFER_SIZE], line[BUFFER_SIZE];
    
    arg=get_arg(arg, left, 0, ses);
    arg=get_arg(arg, line, 1, ses);
    
    if (!*left)
    {
        tintin_eprintf(ses,"#ERROR: valid syntax is: (#match <pattern> <line>)");
        return 0;
    }

    return check_one_action(line, left, &vars, 0, ses);
}


int match_a_string(char *line, char *mask)
{
    char *lptr, *mptr;

    lptr = line;
    mptr = mask;
    while (*lptr && *mptr && !(*mptr == '%' && isdigit(*(mptr + 1))))
        if (*lptr++ != *mptr++)
            return -1;
    if (!*mptr || (*mptr == '%' && isdigit(*(mptr + 1))))
        return (int)(lptr - line);
    return -1;
}

int check_one_action(char *line, char *action, pvars_t *vars, int inside, struct session *ses)
{
    int i;

    if (check_a_action(line, action, inside, ses))
    {
        for (i = 0; i < 10; i++)
        {
            if (var_len[i] != -1)
            {
                strncpy((*vars)[i], var_ptr[i], var_len[i]);
                *((*vars)[i] + var_len[i]) = '\0';
            }
            else
                (*vars)[i][0]=0;
        }
        return TRUE;
    }
    else
        return FALSE;
}

/******************************************************************/
/* check if a text triggers an action and fill into the variables */
/* return TRUE if triggered                                       */
/******************************************************************/
int check_a_action(char *line, char *action, int inside, struct session *ses)
{
    char result[BUFFER_SIZE];
    char *temp2, *tptr, *lptr, *lptr2;
    int i, flag_anchor, len;

    for (i = 0; i < 10; i++)
        var_len[i] = -1;
    flag_anchor = FALSE;
    lptr = line;
    substitute_myvars(action, result, ses);
    tptr = result;
    if (*tptr == '^')
    {
        if (inside)
            return 0;
        tptr++;
        flag_anchor = TRUE;
        /* CHANGED to fix a bug with #action {^%0 foo}
         * Thanks to Spencer Sun for the bug report (AND fix!)
         if (*tptr!=*line)
         return FALSE;
         */
    }
    if (flag_anchor)
    {
        if ((len = match_a_string(lptr, tptr)) == -1)
            return FALSE;
        match_start=lptr;
        lptr += len;
        tptr += len;
    }
    else
    {
        len = -1;
        while (*lptr)
            if ((len = match_a_string(lptr, tptr)) != -1)
                break;
            else
                lptr++;
        if (len != -1)
        {
            match_start=lptr;
            lptr += len;
            tptr += len;
        }
        else
            return FALSE;
    }
    while (*lptr && *tptr)
    {
        temp2 = tptr + 2;
        if (!*temp2)
        {
            var_len[*(tptr + 1) - 48] = strlen(lptr);
            var_ptr[*(tptr + 1) - 48] = lptr;
            match_end=""; /* NOTE: to use (match_end-line) change this line */
            return TRUE;
        }
        lptr2 = lptr;
        len = -1;
        while (*lptr2)
        {
            if ((len = match_a_string(lptr2, temp2)) != -1)
                break;
            else
                lptr2++;
        }
        if (len != -1)
        {
            var_len[*(tptr + 1) - 48] = lptr2 - lptr;
            var_ptr[*(tptr + 1) - 48] = lptr;
            lptr = lptr2 + len;
            tptr = temp2 + len;
        }
        else
            return FALSE;
    }
    if ((*tptr=='%')&&isdigit(*(tptr+1)))
    {
        var_len[*(tptr+1)-48]=0;
        var_ptr[*(tptr+1)-48]=lptr;
        tptr+=2;
    };
    match_end=lptr;
    if (*tptr)
        return FALSE;
    else
        return TRUE;
}


void doactions_command(char *arg, struct session *ses)
{
    char line[BUFFER_SIZE];
    
    get_arg(arg, line, 1, ses);
    /* the line provided may be empty */
    
    check_all_actions(line, ses);
}


void dopromptactions_command(char *arg, struct session *ses)
{
    char line[BUFFER_SIZE];
    
    get_arg(arg, line, 1, ses);
    /* the line provided may be empty */
    
    check_all_promptactions(line, ses);
}
