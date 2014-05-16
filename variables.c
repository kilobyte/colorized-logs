/*********************************************************************/
/* file: variables.c - functions related the the variables           */
/*                             TINTIN ++                             */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "unicode.h"
#include "protos/action.h"
#include "protos/alias.h"
#include "protos/chinese.h"
#include "protos/glob.h"
#include "protos/hash.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/path.h"
#include "protos/session.h"
#include "protos/ticks.h"
#include "protos/unicode.h"
#include "protos/utils.h"

extern int varnum;
extern pvars_t *pvars;
extern int LINES,COLS;
extern int in_alias;
extern int aborting;
extern char *_;
extern struct session *activesession;
extern char tintin_char;
extern time_t time0;
extern int utime0;

extern struct session *if_command(char *arg, struct session *ses);

void set_variable(char *left,char *right,struct session *ses)
{
    set_hash(ses->myvars, left, right);
    varnum++;       /* we don't care for exactness of this */
    if (ses->mesvar[5])
        tintin_printf(ses, "#Ok. $%s is now set to {%s}.", left, right);
}

/*************************************************************************/
/* copy the arg text into the result-space, but substitute the variables */
/* $<string> with the values they stand for                              */
/*                                                                       */
/* Notes by Sverre Normann (sverreno@stud.ntnu.no):                      */
/* xxxxxxxxx 1996: added 'simulation' of secstotick variable             */
/* Tue Apr 8 1997: added ${<string>} format to allow use of non-alpha    */
/*                 characters in variable name                           */
/*************************************************************************/
/* Note: $secstotick will return number of seconds to tick (from tt's    */
/*       internal counter) provided no variable named 'secstotick'       */
/*       already exists                                                  */
/*************************************************************************/
void substitute_myvars(char *arg,char *result,struct session *ses)
{
    char varname[BUFFER_SIZE], value[BUFFER_SIZE], *v;
    int nest = 0, counter, varlen, valuelen;
    int specvar;
    int len=strlen(arg);

    while (*arg)
    {

        if (*arg == '$')
        {                           /* substitute variable */
            counter = 0;
            while (*(arg + counter) == '$')
                counter++;
            varlen = 0;

            /* ${name} code added by Sverre Normann */
            if(*(arg+counter) != DEFAULT_OPEN)
            {
                /* ordinary variable which name contains no spaces and special characters  */
                specvar = FALSE;
                while (isalpha(*(arg+varlen+counter))||
                       (*(arg+varlen+counter)=='_')||
                       isdigit(*(arg+varlen+counter)))
                    varlen++;
                if (varlen>0)
                    strncpy(varname,arg+counter,varlen);
                *(varname + varlen) = '\0';
            }
            else
            {
                /* variable with name containing non-alpha characters e.g ${a b} */
                specvar = TRUE;
                get_arg_in_braces(arg + counter, varname, 0);
                varlen=strlen(varname);
                substitute_vars(varname, value);
                substitute_myvars(value, varname, ses);      /* RECURSIVE CALL */
            }

            if (specvar) varlen += 2; /* 2*DELIMITERS e.g. {} */

            if (counter == nest + 1)
            {
                if ((v=get_hash(ses->myvars, varname)))
                {
                    valuelen = strlen(v);
                    if ((len+=valuelen-counter-varlen) > BUFFER_SIZE-10)
                    {
                        if (!aborting)
                        {
                            tintin_eprintf(ses,"#ERROR: command+variables too long while substituting $%s%s%s.",
                                specvar?"{":"",varname,specvar?"}":"");
                            aborting=1;
                        }
                        len-=valuelen-counter-varlen;
                        goto novar;
                    }
                    strcpy(result, v);
                    result += valuelen;
                    arg += counter + varlen;
                }
                else
                {
                    /* secstotick code added by Sverre Normann */
                    if (strcmp(varname,"secstotick")==0)
                        sprintf(value,"%d",timetilltick(ses));
                    else
                    if (strcmp(varname,"LINES")==0)
                        sprintf(value,"%d",LINES);
                    else
                    if (strcmp(varname,"COLS")==0)
                        sprintf(value,"%d",COLS);
                    else
                    if (strcmp(varname,"PATH")==0)
                        path2var(value,ses);
                    else
                    if (strcmp(varname,"IDLETIME")==0)
                        sprintf(value,"%ld", (long int)(time(0)-ses->idle_since));
                    else
                    if (_ && (strcmp(varname,"LINE")==0 ||
                        strcmp(varname,"_")==0))
                        strcpy(value,_);
                    else
                    if (strcmp(varname,"SESSION")==0)
                        strcpy(value,ses->name);
                    else
                    if (strcmp(varname,"SESSIONS")==0)
                        seslist(value);
                    else
                    if (strcmp(varname,"ASESSION")==0)
                        strcpy(value,activesession->name);
                    else
                    if (strcmp(varname,"LOGFILE")==0)
                        strcpy(value,ses->logfile?ses->logname:"");
                    else
                    if (strcmp(varname,"_random")==0)
                        sprintf(value,"%d", rand());
                    else
                    if (strcmp(varname,"_time")==0 || strcmp(varname,"time")==0)
                        sprintf(value,"%ld", (long int)time0);
                    else
                    if (strcmp(varname,"_clock")==0)
                        sprintf(value,"%ld", (long int)time(0));
                    else
                    if (strcmp(varname,"_msec")==0)
                    {
                        struct timeval tv;
                        gettimeofday(&tv, 0);
                        sprintf(value,"%ld", (long int)((tv.tv_sec-time0)*1000+(tv.tv_usec-utime0)/1000));
                    }
                    else
                    if (strcmp(varname,"HOME")==0)
                    {
                        v=getenv("HOME");
                        if (v)
                            snprintf(value, BUFFER_SIZE, "%s", v);
                        else
                            *value=0;
                    }
                    else
                        goto novar;
                    valuelen=strlen(value);
                    if ((len+=valuelen-counter-varlen) > BUFFER_SIZE-10)
                    {
                        if (!aborting)
                        {
                            tintin_eprintf(ses,"#ERROR: command+variables too long while substituting $%s.",varname);
                            aborting=1;
                        }
                        len-=valuelen-counter-varlen;
                        goto novar;
                    }
                    strcpy(result,value);
                    result+=valuelen;
                    arg += counter + varlen;
                    /* secstotick code end */
                } /* end if ...searchnode... */
            }
            else
            {
novar:
                strncpy(result, arg, counter + varlen);
                result += varlen + counter;
                arg += varlen + counter;
            }
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
        else if (*arg == '\\' && *(arg + 1) == '$' && nest == 0)
        {
            arg++;
            *result++ = *arg++;
            len--;
        }
        else
            *result++ = *arg++;
    }

    *result = '\0';
}


/*************************/
/* the #variable command */
/*************************/
void variable_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    int r;

    /* char right2[BUFFER_SIZE]; */
    arg = get_arg(arg, left, 0,ses);
    r=*space_out(arg);
    arg = get_arg(arg, right, 1,ses);
    if (*left && r)
    {
        set_variable(left,right,ses);
        return;
    }
    show_hashlist(ses, ses->myvars, left,
        "#THESE VARIABLES HAVE BEEN SET:",
        "#THAT VARIABLE IS NOT DEFINED.");
}

/**********************/
/* the #unvar command */
/**********************/
void unvariable_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE];

    arg = get_arg(arg, left, 1, ses);
    delete_hashlist(ses, ses->myvars, left,
        ses->mesvar[5]? "#Ok. $%s is no longer a variable." : 0,
        ses->mesvar[5]? "#THAT VARIABLE (%s) IS NOT DEFINED." : 0);
}


/*****************************************************************/
/* the #listlength command * By Sverre Normann                   */
/*****************************************************************/
/* Syntax: #listlength {destination variable} {list}             */
/*****************************************************************/
/* Note: This will return the number of items in the list.       */
/*       An item is either a word, or grouped words in brackets. */
/*       Ex:  #listl {listlength} {smile {say Hi!} flip bounce}  */
/*            -> listlength = 4                                  */
/*****************************************************************/
void listlength_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], list[BUFFER_SIZE],
    temp[BUFFER_SIZE];
    int  i;

    arg = get_arg(arg, left,0,ses);
    if (!*left)
        tintin_eprintf(ses,"#Error - Syntax: #listlength {dest var} {list}");
    else
    {
        get_arg(arg, list, 1, ses);
        arg = list;
        i=0;
        do
        {
            if(*arg) i++;
            arg = get_arg_in_braces(arg, temp, 0);
        } while(*arg);
        sprintf(temp,"%d",i);
        set_variable(left,temp,ses);
    }
}

/**************************/
/* the #listlength inline */
/**************************/
int listlength_inline(char *arg,struct session *ses)
{
    char list[BUFFER_SIZE],temp[BUFFER_SIZE];
    int  i;

    arg=get_arg(arg, list, 0, ses);
    arg = list;
    i=0;
    do {
        if(*arg) i++;
        arg = get_arg_in_braces(arg, temp, 0);
    } while(*arg);
    return i;
}


static int find_item(char *item,char *list)
{
    char temp[BUFFER_SIZE];
    int i;

    /*  sprintf(temp,"#searching for [%s] in list [%s]",item,list);
        tintin_printf(0,temp);
    */
    i=0;
    do {
        i++;
        list = get_arg_in_braces(list, temp, 0);
        if (match(item,temp))
            return i;
    } while(*list);
    return 0;
}

/*************************/
/* the #finditem command */
/*************************/
void finditem_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], list[BUFFER_SIZE], item[BUFFER_SIZE];

    arg = get_arg(arg, left,0,ses);
    arg = get_arg(arg, item,0,ses);
    arg = get_arg(arg, list,1,ses);
    if (!*left)
        tintin_eprintf(ses,"#Error - Syntax: #finditem {dest var} {item} {list}");
    else
    {
        sprintf(item,"%d",find_item(item,list));
        set_variable(left,item,ses);
    }
}

/************************/
/* the #finditem inline */
/************************/
int finditem_inline(char *arg,struct session *ses)
{
    char list[BUFFER_SIZE], item[BUFFER_SIZE];

    arg = get_arg(arg, item,0,ses);
    arg = get_arg(arg, list,1,ses);
    return find_item(item,list);
}


/********************************************************************/
/* the #getitem command * By Sverre Normann                         */
/********************************************************************/
/* Syntax: #getitem {destination variable} {item number} {list}     */
/********************************************************************/
/* Note: This will return a specified item from a list.             */
/*       An item is either a word, or grouped words in brackets.    */
/*       FORTRAN type indices (first element has index '1' not '0') */
/*       Ex:  #geti {dothis} {2} {smile {say Hi!} flip bounce}      */
/*            -> dothis = say Hi!                                   */
/********************************************************************/
void getitem_command(char *arg,struct session *ses)
{
    char destvar[BUFFER_SIZE], itemnrtxt[BUFFER_SIZE],
    list[BUFFER_SIZE], temp1[BUFFER_SIZE];
    int  i, itemnr;

    arg = get_arg(arg, destvar, 0, ses);
    arg = get_arg(arg, itemnrtxt, 0, ses);

    if (!*destvar || !*itemnrtxt)
    {
        tintin_eprintf(ses,"#Error - Syntax: #getitem {destination variable} {item number} {list}");
    }
    else
    {
        if (sscanf(itemnrtxt,"%d",&itemnr) != 1)
            tintin_eprintf(ses,"#Error in #getitem - expected a _number_ as item number, got {%s}.",itemnrtxt);
        else
        {
            get_arg(arg, list, 1, ses);
            arg = list;
            i=0;
            if (itemnr>0)
            {
                do {
                    arg = get_arg_in_braces(arg, temp1, 0);
                    i++;
                } while (i!=itemnr);

                if (*temp1)
                    set_variable(destvar,temp1,ses);
                else
                {
                    set_variable(destvar,"",ses);
                    if (ses->mesvar[5])
                        tintin_printf(ses,"#Item doesn't exist!");
                }
            }
        }
    }
}

/*************************************************************/
/* the #isatom command * By Jakub Narebski                   */
/*************************************************************/
/* Syntax: #isatom {variable} {expression}                   */
/*************************************************************/
/*  Note: this will set variable to 0 if expression is list  */
/*        and 1 if it contains single token = atom           */
/*        i.e. it checks if expression is simple (is atom)   */
/* NOTE1: 'atom' is atom but '{atom}' is list                */
/* NOTE2: all variables substitutions in expression are done */
/*************************************************************/
/*    Ex: #isatom {log} {atom}   -> log = 1                  */
/*        #isatom {log} {{atom}} -> log = 0                  */
/*        #isatom {log} {a list} -> log = 0                  */
/*                                                           */
/* To be substituted by appriopriate #if expression          */
/*************************************************************/

/* First we have function which does necessary stuff */
/* Argument: after all substitutions, with unnecessary surrounding */
/*           spaces removed (e.g. ' {atom}' is _not_ an atom */
int isatom(char *arg)
{
    int last = strlen(arg);
    if((arg[0]    == DEFAULT_OPEN) &&
            (arg[last] == DEFAULT_CLOSE))
        /* one element list = '{elem}' */
        return FALSE;

    if (strchr(arg,' '))
        /* argument contains spaces i.e. = 'elem1 elem2' */
        /* this is incompatibile with supposed " behaviour */
        return FALSE;

    /* else */
    return TRUE;
}

/***********************/
/* the #isatom command */
/***********************/
void isatom_command(char *line,struct session *ses)
{
    /* char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], */
    char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[8];
    int i;

    line = get_arg(line, left, 0, ses);
    if (!*left)
    {
        tintin_eprintf(ses,"#Syntax: #isatom <dest. var> <list>");
        return;
    };
    line = get_arg(line, right, 1, ses);
    i = isatom(right);
    sprintf(temp, "%d", i);
    set_variable(left,temp,ses);
}


/**********************/
/* the #isatom inline */
/**********************/
int isatom_inline(char *arg,struct session *ses)
{
    char list[BUFFER_SIZE];

    arg = get_arg(arg, list,1, ses);
    return isatom(list);
}


/*******************************************************************/
/* the #splitlist command * By Jakub Narebski                      */
/*******************************************************************/
/* Syntax: #splitlist {head} {tail} {list} [{head length}]         */
/*******************************************************************/
/*  Note: <head> and <tail> are destination variables,             */
/*        <list> is expression which is list (possibly             */
/*        containing only atom eg. <list>='1').                    */
/*        This will set <head> to head of the list i.e. it will    */
/*        contain elements 1..<head length> from list;             */
/*        <tail> will be set to rest of the list i.e. it will      */
/*        contain elements from element number <head length>+1     */
/*        to the last element of the list.                         */
/*        Both <head> and <tail> can be empty after command.       */
/*        DEFAULT <head length> is 1.                              */
/*******************************************************************/
/*    Ex: #splitlist {head} {tail} {smile {say Hi!} flip bounce}   */
/*        -> head = smile                                          */
/*        -> tail = {say Hi!} bounce                               */
/*    Ex: #splitlist {head} {tail} {{smile wild} Hi all}           */
/*        -> head = smile wild                                     */
/*        -> tail = Hi all                                         */
/*    Ex: #splitlist {head} {tail} {smile {say Hi} {bounce a}} {2} */
/*        -> head = smile {say Hi}                                 */
/*        -> tail = bounce a                                       */
/*        CHANGE: now it's tail-> {bounce a} to allow iterators    */
/*******************************************************************/

/* FUNCTION:  get_split_pos - where to split                 */
/* ARGUMENTS: list - list (after all substitutions done)     */
/*            head_length - where to split                   */
/* RESULT:    pointer to elements head_length+1...           */
/*            or pointer to '\0' or empty string ""          */
/*            i.e. to character after element No head_length */
static char* get_split_pos(char *list, int head_length)
{
    int i = 0;
    char temp[BUFFER_SIZE];

    if( list[0] == '\0' ) /* empty string */
        return list;

    if (head_length > 0)
    { /* modified #getitemnr code */
        do {
            list = get_arg_in_braces(list, temp, STOP_AT_SPACES);
            i++;
        } while (i != head_length);

        return list;
    }
    else /* head_length == 0 */
        return list;
}


/* FUNCTION:  is_braced_atom - checks if list is braced atom          */
/* ARGUMENTS: beg - points to the first character of list             */
/*            end - points to the element after last (usually '\0')   */
/*            ses - session; used only for error handling             */
/* RESULT:    TRUE if list is braced atom e.g. '{atom}'               */
/*            i.e. whole list begins with DEFAULT_OPEN end ends with  */
/*            DEFAULT_CLOSE and whole is inside group (inside braces) */
static int is_braced_atom_2(char *beg,char *end,struct session *ses)
{
    /* we define where list ends */
#define AT_END(beg,end) (((*beg) == '\0') || (beg >= end))
#define NOT_AT_END(beg,end) (((*beg) != '\0') && (beg < end))

    int nest = 0;

    if(AT_END(beg,end)) /* string is empty */
        return FALSE;

    if (*beg!=DEFAULT_OPEN)
        return FALSE;

    while (NOT_AT_END(beg,end) && !(*beg == DEFAULT_CLOSE && nest == 0))
    {
        if (*beg=='\\') /* next element is taken verbatim i.e. as is */
            beg++;
        else
            if (*beg == DEFAULT_OPEN)
                nest++;
            else
                if (*beg == DEFAULT_CLOSE)
                    nest--;

        if(NOT_AT_END(beg,end)) /* in case '\\' is the last character */
            beg++;

        if(nest == 0 && NOT_AT_END(beg,end))  /* we are at outer level and not at end */
            return FALSE;
    }

    /* we can check only if there are too many opening delimiters */
    /* this should not happen anyway */
    if(nest > 0)
    {
        tintin_eprintf(ses,"Unmatched braces error - too many '%c'",DEFAULT_OPEN);
        return FALSE;
    }

    return TRUE;
}

/* FUNCTION:  simplify_list - removes unwanted characters from        */
/*                            beginning and end of string             */
/* ARGUMENTS: *beg - points to the first character of list            */
/*            *end - points to the element after last (usually '\0')  */
/*            flag - should we remove outside braces if possible?     */
/* RESULT:    modifies *beg and/or *end such that spaces from         */
/*            the beginning and the end of list are removed and if    */
/*            list contains single braced atom (see is_braced_atom    */
/*            for further description) then group delimiters are      */
/*            removed - similar to #getitem command behavior          */
/*            in the case like this                                   */
/*            If you don't like this behavior simply undefine         */
/*            REMOVE_ONEELEM_BRACES.                                  */
/*            see also: getitemnr_command, REMOVE_ONEELEM_BRACES      */
static void simplify_list(char **beg, char **end, int flag, struct session *ses)
{
    /* remember: we do not check arguments (e.g. if they are not NULL) */

    /* removing spaces */
    /* we use 'isspace' because 'space_out' does */
    while ((**beg) && ((*beg) < (*end)) && isspace(**beg))
        (*beg)++;
    while (((*end) > (*beg)) && isspace(*(*end - 1)))
        (*end)--;

#ifdef REMOVE_ONEELEM_BRACES
    /* removing unnecessary braces (one pair at most) */
    if (flag&&is_braced_atom_2((*beg),(*end),ses))
    {
        (*beg)++;
        (*end)--;
    }
#endif
}

/* FUNCTION:  copy_part - string copying                 */
/* ARGUMENTS: dest - destination string                  */
/*            beg - points to first character to copy    */
/*            end - points after last character to copy  */
/* RESULT:    zero-ended string from beg to end          */
static char* copy_part(char *dest,char *beg,char *end)
{
    strcpy(dest, beg);
    dest[end - beg] = '\0';
    return dest;
}

/* FUNCTION:  split_list - main function                                   */
/* ARGUMENTS: list - list to split                                         */
/*            head_length - number of elements head will have              */
/* RESULT:    head - head of the list (elements 1..head_length             */
/*            tail - tail of the list (elements head_length+1..list_size)  */
/* NOTE:      both head and tail could be empty strings;                   */
/*            if head or/and tail contains only one element                */
/*            and REMOVE_ONEELEM_BRACES is defined the element is unbraced */
/*            if necessary                                                 */
static void split_list(char *head,char *tail,char *list,int head_length,struct session *ses)
{
    /* these are pointers, not strings */
    char *headbeg, *headend;
    char *tailbeg, *tailend;

    if (!*list)
    {
        head[0] = tail[0] = '\0';
        return;
    }

    /* where to split */
    tailbeg = get_split_pos(list, head_length);
    /* head */
    headbeg = list;
    headend = tailbeg;

    simplify_list(&headbeg, &headend, 1, ses);
    copy_part(head, headbeg, headend);
    /* tail */
    tailend = tailbeg + strlen(tailbeg);
    simplify_list(&tailbeg, &tailend, 0, ses);
    copy_part(tail, tailbeg, tailend);
}

/**************************/
/* the #splitlist command */
/**************************/
void splitlist_command(char *arg,struct session *ses)
{
    /* command arguments */
    char headvar[BUFFER_SIZE], tailvar[BUFFER_SIZE];
    char list[BUFFER_SIZE];
    char headlengthtxt[BUFFER_SIZE];
    /* temporary variables */
    char head[BUFFER_SIZE], tail[BUFFER_SIZE];
    int head_length;


    /** get command arguments */
    arg = get_arg(arg, headvar, 0, ses);
    arg = get_arg(arg, tailvar, 0, ses);

    if (!*headvar && !*tailvar)
    {
        tintin_eprintf(ses,"#Error - Syntax: #splitlist {head variable} {tail variable} "
                     "{list} [{head size}]");
        return; /* on ERROR */
    }

    arg = get_arg(arg, list, 1, ses);
    arg = get_arg(arg, headlengthtxt, 1, ses);

    if (!*headlengthtxt)
    {
        /* defaults number of elements in head to 1 if no value given */
        headlengthtxt[0] = '1'; headlengthtxt[1] = '\0';
        head_length = 1;
    }
    else
    {
        if (sscanf(headlengthtxt,"%d",&head_length) != 1)
        {
            tintin_eprintf(ses,"#Error in #splitlist - head size has to be number>=0, got {%s}.",headlengthtxt);
            return; /* on ERROR */
        }
        if (head_length < 0)
        {
            tintin_eprintf(ses,"#Error in #splitlist - head size could not be negative, got {%d}.",head_length);
            return; /* on ERROR */
        }
    } /* end if */

    /** invoke main procedure **/
    split_list(head, tail, list, head_length, ses);

    /** do all assignments **/
    if(*headvar) /* nonempty names only */
        set_variable(headvar,head,ses);
    if(*tailvar) /* nonempty names only */
        set_variable(tailvar,tail,ses);
}

/****************************/
/* the #deleteitems command */
/****************************/
void deleteitems_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], list[BUFFER_SIZE], item[BUFFER_SIZE],
    temp[BUFFER_SIZE], right[BUFFER_SIZE], *lpos, *rpos;

    arg = get_arg(arg, left,0,ses);
    if (!*left)
        tintin_eprintf(ses,"#Error - Syntax: #deleteitem {dest. variable} {list} {item}");
    else
    {
        arg=get_arg(arg, list, 0, ses);
        get_arg(arg,item,1,ses);
        arg = list;
        rpos=right;
        if (*arg)
            do {
                arg = get_arg_in_braces(arg, temp, 0);
                if (!match(item,temp))
                {
                    if (rpos!=right)
                        *rpos++=' ';
                    lpos=temp;
                    if (isatom(temp))
                        while (*lpos)
                            *rpos++=*lpos++;
                    else
                    {
                        *rpos++=DEFAULT_OPEN;
                        while (*lpos)
                            *rpos++=*lpos++;
                        *rpos++=DEFAULT_CLOSE;
                    }
                }
            } while(*arg);
        *rpos=0;
        set_variable(left,right,ses);
    }
}

/**************************/
/* the #foreach command   */
/**************************/
struct session *foreach_command(char *arg,struct session *ses)
{
    char *list, temp[BUFFER_SIZE], left[BUFFER_SIZE], right[BUFFER_SIZE], *p;
    pvars_t vars,*lastvars;
    int i;

    arg=get_arg(arg, left, 0, ses);
    get_arg_in_braces(arg,right,1);
    if (!*right)
    {
        tintin_eprintf(ses,"#SYNTAX: foreach {list} command");
        return ses;
    }
    lastvars=pvars;
    pvars=&vars;
    list = left;
    while (*list)
    {
        list = get_arg_in_braces(list, temp, 0);
        strcpy(vars[0], p=temp);
        for (i=1;i<10;i++)
            p=get_arg_in_braces(p, vars[i], 0);
        in_alias=1;
        ses=parse_input(right,1,ses);
    }
    pvars=lastvars;
    return ses;
}

struct session *forall_command(char *arg,struct session *ses)
{
    return foreach_command(arg, ses);
}

static int compar(const void *a,const void *b)
{
    return strcmp(*(char **)a,*(char **)b);
}

/***************************/
/* the #sortlist command   */
/***************************/
void sortlist_command(char *arg,struct session *ses)
{
    char *list, temp[BUFFER_SIZE], left[BUFFER_SIZE], right[BUFFER_SIZE];
    int i,n;
    char *tab[BUFFER_SIZE];

    arg=get_arg(arg, left, 0, ses);
    get_arg(arg,right,1,ses);
    if (!*left)
    {
        tintin_eprintf(ses,"#SYNTAX: sortlist var {list}");
        return;
    }

    n=0;
    list = right;
    while (*list)
    {
        list = get_arg_in_braces(list, temp, 0);
        tab[n++]=mystrdup(temp);
    };
    qsort(tab,n,sizeof(char*),compar);
    list=temp;
    for (i=0;i<n;i++)
    {
        if (list!=temp)
            *list++=' ';
        if (*tab[i]&&isatom(tab[i]))
            list+=sprintf(list, "%s", tab[i]);
        else
            list+=sprintf(list, "{%s}", tab[i]);
    }
    *list=0;
    set_variable(left, temp, ses);
}

/************************/
/* the #tolower command */
/************************/
void tolower_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    WC *p,txt[BUFFER_SIZE];

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left)
        tintin_eprintf(ses,"#Syntax: #tolower <var> <text>");
    else
    {
        TO_WC(txt, right);
        for (p = txt; *p; p++)
            *p = towlower(*p);
        WRAP_WC(right, txt);
        set_variable(left,right,ses);
    }
}

/************************/
/* the #toupper command */
/************************/
void toupper_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    WC *p,txt[BUFFER_SIZE];

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left)
        tintin_eprintf(ses,"#Syntax: #toupper <var> <text>");
    else
    {
        TO_WC(txt, right);
        for (p = txt; *p; p++)
            *p = towupper(*p);
        WRAP_WC(right, txt);
        set_variable(left,right,ses);
    }
}

/***************************/
/* the #firstupper command */
/***************************/
void firstupper_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];
    WC *p,txt[BUFFER_SIZE];

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left)
        tintin_eprintf(ses,"#Syntax: #firstupper <var> <text>");
    else
    {
        TO_WC(txt, right);
        for (p = txt; *p; p++)
            *p = towlower(*p);
        *txt=towupper(*txt);
        WRAP_WC(right, txt);
        set_variable(left,right,ses);
    }
}

/***********************/
/* the #strlen command */
/***********************/
void strlen_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left)
        tintin_eprintf(ses,"#Syntax: #strlen <var> <text>");
    else
    {
        sprintf(right, "%d", FLATlen(right));
        set_variable(left,right,ses);
    }
}

/**********************/
/* the #strlen inline */
/**********************/
int strlen_inline(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE];

    arg = get_arg(arg, left, 1, ses);
    return FLATlen(left);
}

/****************************************************/
/* the #reverse command * By Sverre Normann         */
/* with little changes by Jakub Narebski            */
/****************************************************/
/* Syntax: #reverse {destination variable} {string} */
/****************************************************/
/* Will return the reverse of a string.             */
/****************************************************/
/* can be used to get return path using #savepath   */
/****************************************************/

/* FUNCTION:  reverse - reverses string    */
/* ARGUMENTS: src - string to be reversed  */
/*            dest - destination string    */
/* RESULT:    reversed string              */
/* NOTE:      no check                     */
static WC* revstr(WC *dest, WC *src)
{
    int i;
    int ilast = WClen(src) - 1;

    for(i = ilast; i >= 0; i--)
        dest[ilast - i] = src[i];
    dest[ilast + 1] = '\0';

    return dest;
}

/************************/
/* the #reverse command */
/************************/
void reverse_command(char *arg,struct session *ses)
{
    char destvar[BUFFER_SIZE], strvar[BUFFER_SIZE];
    WC origstring[BUFFER_SIZE], revstring[BUFFER_SIZE];

    arg = get_arg(arg, destvar, 0, ses);
    arg = get_arg(arg, strvar, 1, ses);

    if (!*destvar)
        tintin_eprintf(ses,"#Error - Syntax: #reverse {destination variable} {string}");
    else
    {
        TO_WC(origstring, strvar);
        revstr(revstring, origstring);
        WRAP_WC(strvar, revstring);
        set_variable(destvar, strvar, ses);
    }
}


/************************/
/* the #explode command */
/************************/
void explode_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], del[BUFFER_SIZE], right[BUFFER_SIZE],
         *p, *n, *r,
         res[3*BUFFER_SIZE]; /* worst case is ",,,,," -> "{} {} {} {} {} {}" */

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, del, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left || !*del)
        tintin_eprintf(ses,"#Syntax: #explode <var> <delimiter> <text>");
    else
    {
        r=res;
        p=right;
        while ((n=strstr(p,del)))
        {
            *n=0;
            if (p!=right)
                *r++=' ';
            if (*p&&isatom(p))
                r+=sprintf(r, "%s", p);
            else
                r+=sprintf(r, "{%s}", p);
            p=n+strlen(del);
        }
        if (p!=right)
            *r++=' ';
        if (*p&&isatom(p))
            r+=sprintf(r, "%s", p);
        else
            r+=sprintf(r, "{%s}", p);

        if (strlen(res)>BUFFER_SIZE-10)
        {
            tintin_eprintf(ses,"#ERROR: exploded line too long in #explode {%s} {%s} {%s}",left,del,right);
            res[BUFFER_SIZE-10]=0;
        }
        set_variable(left,res,ses);
    }
}


/************************/
/* the #implode command */
/************************/
void implode_command(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], del[BUFFER_SIZE], right[BUFFER_SIZE],
         *p, res[BUFFER_SIZE], temp[BUFFER_SIZE], *r;
    int len,dellen;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, del, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left || !*del)
        tintin_eprintf(ses,"#Syntax: #implode <var> <delimiter> <list>");
    else
    {
        dellen=strlen(del);
        p = get_arg_in_braces(right, res, 0);
        r=strchr(res,0);
        len=r-res;
        while(*p)
        {
            p = get_arg_in_braces(p, temp, 0);
            if ((len+=strlen(temp)+dellen) > BUFFER_SIZE-10)
            {
                tintin_eprintf(ses,"#ERROR: imploded line too long in #implode {%s} {%s} {%s}",left,del,right);
                break;
            }
            r+=sprintf(r, "%s%s", del, temp);
        }
        set_variable(left,res,ses);
    }
}


/************************/
/* the #collate command */
/************************/
void collate_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], list[BUFFER_SIZE],
         cur[BUFFER_SIZE], last[BUFFER_SIZE], out[BUFFER_SIZE], *outptr;
    int i,j;

    arg = get_arg(arg, left,0,ses);
    if (!*left)
        tintin_eprintf(ses,"#Error - Syntax: #collate {dest var} {list}");
    else
    {
        strcpy(last, K_ACTION_MAGIC);
        *(outptr=out)=0;
        get_arg(arg, list, 1, ses);
        arg = list;
        i=0;
        while(*arg)
        {
            arg=space_out(arg);
            if (isdigit(*arg))
                j=strtol(arg, &arg, 10);
            else
                j=1;
            if (!*arg || *arg==' ')
                continue;
            arg = get_arg_in_braces(arg, cur, 0);
            if (j)
            {
                if(strcmp(cur,last))
                {
                    if (isatom(last))
                    {
                        if (i>1)
                            outptr+=sprintf(outptr,"%d",i);
                        if (i)
                            outptr+=sprintf(outptr,"%s ",last);
                    }
                    else
                    {
                        if (i>1)
                            outptr+=sprintf(outptr,"%d",i);
                        if (i)
                            outptr+=sprintf(outptr,"{%s} ",last);
                    }
                    strcpy(last, cur);
                    i=j;
                }
                else
                    i+=j;
             }
        };
        if (isatom(last))
        {
            if (i>1)
                outptr+=sprintf(outptr,"%d",i);
            if (i)
                outptr+=sprintf(outptr,"%s",last);
        }
        else
        {
            if (i>1)
                outptr+=sprintf(outptr,"%d",i);
            if (i)
                outptr+=sprintf(outptr,"{%s}",last);
        }
        set_variable(left,out,ses);
    }
}


/***********************/
/* the #expand command */
/***********************/
void expand_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], list[BUFFER_SIZE],
         cur[BUFFER_SIZE], out[BUFFER_SIZE], *outptr;
    int i,j;

    arg = get_arg(arg, left,0,ses);
    if (!*left)
        tintin_eprintf(ses,"#Error - Syntax: #expand {dest var} {list}");
    else
    {
        *(outptr=out)=0;
        get_arg(arg, list, 1, ses);
        arg = list;
        i=0;
        while(*arg)
        {
            arg=space_out(arg);
            if (isdigit(*arg))
                j=strtol(arg, &arg, 10);
            else
                j=1;
            if (!*arg || *arg==' ')
                continue;
            arg = get_arg_in_braces(arg, cur, 0);
            if(j>BUFFER_SIZE/2)
                    j=BUFFER_SIZE/2;
            for(i=0;i<j;i++)
            {
                if (isatom(cur))
                    outptr+=snprintf(outptr, out+BUFFER_SIZE-outptr, " %s", cur);
                else
                    outptr+=snprintf(outptr, out+BUFFER_SIZE-outptr, " {%s}", cur);
                if (outptr>=out+BUFFER_SIZE-1)
                {
                    tintin_eprintf(ses, "#ERROR: expanded line too long in {%s}", list);
                    return;
                }
            }
        }
        set_variable(left,(*out)?out+1:out,ses);
    }
}


/***********************/
/* the #random command */
/***********************/
void random_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE], dummy;
    int low, high, number;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    if (!*left || !*right)
        tintin_eprintf(ses,"#Syntax: #random <var> <low,high>");
    else
    {
        if (sscanf(right, "%d,%d%c", &low, &high, &dummy) != 2)
            tintin_eprintf(ses,"#Wrong number of range arguments in #random: got {%s}.",right);
        else if (low < 0 || high < 0)
            tintin_eprintf(ses,"#Both arguments of range in #random should be >0, got %d,%d.",low,high);
        else
        {
            if (low > high)
            {
                number = low;
                low = high, high = number;
            };
            number = low + rand() % (high - low + 1);
            sprintf(right, "%d", number);
            set_variable(left,right,ses);
        }
    }
}

/**********************/
/* the #random inline */
/**********************/
int random_inline(char *arg, struct session *ses)
{
    char right[BUFFER_SIZE];
    int low, high, tmp;

    arg = get_arg(arg, right, 1, ses);
    if (!*right)
        tintin_eprintf(ses,"#Syntax: #random <low,high>");
    else
    {
        if (sscanf(right, "%d,%d", &low, &high) != 2)
            tintin_eprintf(ses,"#Wrong number of range arguments in #random: got {%s}.",right);
        else if (low < 0 || high < 0)
            tintin_eprintf(ses,"#Both arguments of range in #random should be >0, got %d,%d.",low,high);
        else
        {
            if (low > high)
            {
                tmp = low;
                low = high, high = tmp;
            };
            return low + rand() % (high - low + 1);
        }
    }
    return 0;
}

/************************************************************************************/
/* cut a string to width len, putting the cut point into *rstr and return the width */
/************************************************************************************/
static int cutws(WC *str, int len, WC **rstr)
{
    int w,s;

    s=0;
    while(*str)
    {
        w=wcwidth(*str);
        if (w<0)
            w=0;
        if (w && s+w>len)
            break;
        str++;
        s+=w;
    }
    *rstr=str;
    return s;
}

/*****************************************************/
/* Syntax: #postpad {dest var} {length} {text}       */
/*****************************************************/
/* Truncates text if it's too long for the specified */
/* length. Pads with spaces at the end if the text   */
/* isn't long enough.                                */
/*****************************************************/
void postpad_command(char *arg,struct session *ses)
{
    char destvar[BUFFER_SIZE], lengthstr[BUFFER_SIZE], astr[BUFFER_SIZE], *aptr;
    WC bstr[BUFFER_SIZE], *bptr;
    int len, length;

    arg = get_arg(arg, destvar, 0, ses);
    arg = get_arg(arg, lengthstr, 0, ses);
    arg = get_arg(arg, astr, 1, ses);

    if (!*lengthstr)
        tintin_eprintf(ses,"#Error - Syntax: #postpad {dest var} {length} {text}");
    else
    {
        if (!sscanf(lengthstr,"%d",&length) || (length < 1) || (length > BUFFER_SIZE-10))
            tintin_eprintf(ses,"#Error in #postpad - length has to be a positive number >0, got {%s}.",lengthstr);
        else
        {
            TO_WC(bstr, astr);
            len=cutws(bstr, length, &bptr);
            aptr=astr+wc_to_utf8(astr, bstr, bptr-bstr, BUFFER_SIZE-3);
            while(len<length)
                len++, *aptr++=' ';
            *aptr=0;
            set_variable(destvar, astr, ses);
        }
    }
}

/*****************************************************/
/* the #prepad command * By Sverre Normann           */
/* with little changes by Jakub Narebski             */
/*****************************************************/
/* Syntax: #prepad {dest var} {length} {text}        */
/*****************************************************/
/* Truncates text if it's too long for the specified */
/* length. Pads with spaces at the start if the text */
/* isn't long enough.                                */
/*****************************************************/
void prepad_command(char *arg,struct session *ses)
{
    char destvar[BUFFER_SIZE], astr[BUFFER_SIZE], lengthstr[BUFFER_SIZE], *aptr;
    WC bstr[BUFFER_SIZE], *bptr;
    int len, length;

    arg = get_arg(arg, destvar, 0, ses);
    arg = get_arg(arg, lengthstr, 0, ses);
    arg = get_arg(arg, astr, 1, ses);

    if (!*lengthstr)
        tintin_eprintf(ses,"#Error - Syntax: #prepad {dest var} {length} {text}");
    else
    {
        if (!sscanf(lengthstr,"%d",&length) || (length < 1) || (length>BUFFER_SIZE-10))
            tintin_eprintf(ses,"#Error in #prepad - length has to be a positive number >0, got {%s}.",lengthstr);
        else
        {
            TO_WC(bstr, astr);
            len=cutws(bstr, length, &bptr);
            aptr=astr;
            while(len<length)
                len++, *aptr++=' ';
            aptr+=wc_to_utf8(aptr, bstr, bptr-bstr, BUFFER_SIZE-3);
            *aptr=0;
            set_variable(destvar, astr, ses);
        }
    }
}

#define INVALID_TIME 0x80000000

/************************************************************/
/* parse time, return # of seconds or INVALID_TIME on error */
/************************************************************/
static int time2secs(char *tt,struct session *ses)
{
    char *oldtt;
    int w,t=0;

    if (!*tt)
    {
bad:
        tintin_eprintf(ses,"#time format should be: <#y[ears][,] #w #d #h #m [and] #[s]> or just <#> of seconds.");
        tintin_eprintf(ses,"#got: {%s}.",tt);
        return INVALID_TIME;
    };
    t=0;
    for (;;)
    {
        w=strtol(oldtt=tt,&tt,10);
        if (tt==oldtt)
            goto bad;
        while (*tt==' ')
            tt++;
        switch (tolower(*tt))
        {
        case 'w':
            w*=7;
            goto day;
        case 'y':
            w*=365;
        day:
        case 'd':
            w*=24;
        case 'h':
            w*=60;
        case 'm':
            w*=60;
        case 's':
            while (isalpha(*tt))
                tt++;
        case 0:
            t+=w;
            break;
        default:
            goto bad;
        };
        if (*tt==',')
            tt++;
        while (*tt==' ')
            tt++;
        if (!strncmp(tt,"and",3))
            tt+=3;
        if (!*tt)
            break;
    };
    return t;
}

/**********************/
/* the #ctime command */
/**********************/
void ctime_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], arg2[BUFFER_SIZE], *ct, *p;
    time_t tt;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, arg2, 1, ses);
    if (*arg2)
        if ((tt=time2secs(arg2,ses))==INVALID_TIME)
            return;
        else;
    else
        tt=time(0);
    p = ct = ctime(&tt);
    while(p && *p)
    {
        if(*p == '\n')
        {
            *p = '\0';
            break;
        }
        p++;
    }
    if (!*left)
        tintin_printf(ses, "#%s.", ct);
    else
        set_variable(left,ct,ses);
}

/*********************/
/* the #time command */
/*********************/
void time_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], ct[BUFFER_SIZE];

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, ct, 1, ses);
    if (*ct)
    {
        int t=time2secs(ct,ses);
        if (t==INVALID_TIME)
            return;
        sprintf(ct,"%d",t);
    }
    else
        sprintf(ct,"%d",(int)time(0));
    if (!*left)
        tintin_printf(ses, "#%s.", ct);
    else
        set_variable(left,ct,ses);
}

/**************************/
/* the #localtime command */
/**************************/
void localtime_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], ct[BUFFER_SIZE];
    time_t t;
    struct tm ts;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, ct, 1, ses);
    if (*ct)
    {
        t=time2secs(ct,ses);
        if (t==INVALID_TIME)
            return;
        sprintf(ct, "%ld", (long)t);
    }
    else
        t=time(0);
    localtime_r(&t, &ts);
    sprintf(ct, "%02d %02d %02d  %02d %02d %04d  %d %d %d",
                    ts.tm_sec, ts.tm_min, ts.tm_hour,
                    ts.tm_mday, ts.tm_mon+1, ts.tm_year+1900,
                    ts.tm_wday, ts.tm_yday+1, ts.tm_isdst);
    if (!*left)
        tintin_printf(ses, "#%s.", ct);
    else
        set_variable(left,ct,ses);
}

/***********************/
/* the #gmtime command */
/***********************/
void gmtime_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], ct[BUFFER_SIZE];
    time_t t;
    struct tm ts;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, ct, 1, ses);
    if (*ct)
    {
        t=time2secs(ct,ses);
        if (t==INVALID_TIME)
            return;
        sprintf(ct, "%ld", (long)t);
    }
    else
        t=time(0);
    gmtime_r(&t, &ts);
    sprintf(ct, "%02d %02d %02d  %02d %02d %04d  %d %d %d",
                    ts.tm_sec, ts.tm_min, ts.tm_hour,
                    ts.tm_mday, ts.tm_mon+1, ts.tm_year+1900,
                    ts.tm_wday, ts.tm_yday+1, ts.tm_isdst);
    if (!*left)
        tintin_printf(ses, "#%s.", ct);
    else
        set_variable(left,ct,ses);
}


/**************************/
/* the #substring command */
/**************************/
void substring_command(char *arg,struct session *ses)
{
    char left[BUFFER_SIZE], mid[BUFFER_SIZE], right[BUFFER_SIZE], *p;
    WC buf[BUFFER_SIZE],*lptr,*rptr;
    int l,r,s,w;

    arg = get_arg(arg, left, 0, ses);
    arg = get_arg(arg, mid, 0, ses);
    arg = get_arg(arg, right, 1, ses);
    l=strtol(mid,&p,10);
    if (*p==',')
        r=strtol(p+1,&p,10);
    else
        r=l;
    if (l<1)
        l=1;

    if (!*left || (p==mid) || (*p) || (r<0))
        tintin_eprintf(ses, "#SYNTAX: substr <var> <l>[,<r>] <string>");
    else
    {
        p=mid;
        TO_WC(buf, right);
        lptr=buf;
        s=1;
        while(*lptr)
        {
            w=wcwidth(*lptr);
            if (w<0)
                w=0;
            if (w && s>=l)
                break;  /* skip incomplete CJK chars with all modifiers */
            lptr++;
            s+=w;
        }
        if (s>l)
            *p++=' ';   /* the left edge is cut in half */
        rptr=lptr;
        while(w=wcwidth(*rptr), *rptr)
        {
            if (w<0)
                w=0;
            if (w && s+w>r+1)
                break;  /* skip incomplete CJK chars with all modifiers */
            rptr++;
            s+=w;
        }
        if (rptr>lptr)
            p+=wc_to_utf8(p, lptr, rptr-lptr, BUFFER_SIZE-3);
        if (s==r && w==2)
            *p++=' ';   /* the right edge is cut */
        *p=0;
        set_variable(left,mid,ses);
    }
}

/***********************/
/* the #strcmp command */
/***********************/
struct session *strcmp_command(char *line, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE], cmd[BUFFER_SIZE];

    line = get_arg(line, left, 0, ses);
    line = get_arg(line, right, 0, ses);
    line = get_arg_in_braces(line, cmd, 1);

    if (!*cmd)
    {
        tintin_eprintf(ses, "#Syntax: #strcmp <a> <b> <command> [#else <command>]");
        return ses;
    }

    if (!strcmp(left,right))
    {
        ses=parse_input(cmd,1,ses);
    }
    else
    {
        line = get_arg_in_braces(line, left, 0);
        if (*left == tintin_char)
        {

            if (is_abrev(left + 1, "else"))
            {
                line = get_arg_in_braces(line, right, 1);
                ses=parse_input(right,1,ses);
            }
            if (is_abrev(left + 1, "elif"))
                ses=if_command(line, ses);
        }
    }
    return ses;
}

/**********************/
/* the #strcmp inline */
/**********************/
int strcmp_inline(char *line, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    line = get_arg(line, left, 0, ses);
    line = get_arg(line, right, 1, ses);

    return !strcmp(left,right);
}

/***************************************/
/* the #ifstrequal command             */
/***************************************/
/* (mainstream tintin++ compatibility) */
/***************************************/
struct session *ifstrequal_command(char *line, struct session *ses)
{
    return strcmp_command(line,ses);
}

/*************************/
/* the #ifexists command */
/*************************/
struct session *ifexists_command(char *line, struct session *ses)
{
    char left[BUFFER_SIZE],cmd[BUFFER_SIZE];

    line = get_arg(line, left, 0, ses);
    line = get_arg_in_braces(line, cmd, 1);

    if (!*cmd)
    {
        tintin_eprintf(ses, "#Syntax: #ifexists <varname> <command> [#else <command>]");
        return ses;
    }

    if (get_hash(ses->myvars,left))
        ses=parse_input(cmd,1,ses);
    else
    {
        line = get_arg_in_braces(line, left, 0);
        if (*left == tintin_char)
        {

            if (is_abrev(left + 1, "else"))
            {
                line = get_arg_in_braces(line, cmd, 1);
                ses=parse_input(cmd,1,ses);
            }
            if (is_abrev(left + 1, "elif"))
                ses=if_command(line, ses);
        }
    }
    return ses;
}

/*********************/
/* the #ctoi command */
/*********************/
void ctoi_command(char* arg, struct session* ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    arg=get_arg(arg, left, 0, ses);
    arg=get_arg(arg, right, 1, ses);

    if (!*left || !*right)
        tintin_eprintf(ses, "#Syntax: #ctoi <var> <text>");
    else
    {
        ctoi(right);
        set_variable(left, right, ses);
    }
}
