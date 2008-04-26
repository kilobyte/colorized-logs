/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
#include "config.h"
#include "tintin.h"
#include <ctype.h>
int stacks[100][4];

#ifdef HAVE_UNISTD_H
#include <stdlib.h>
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern char *space_out(char *s);
extern char *get_inline(char *s,char *dest);
static int conv_to_ints(char *arg,struct session *ses);
static int do_one_inside(int begin, int end);
int eval_expression(char *arg,struct session *ses);
extern int finditem_inline(char *arg,struct session *ses);
extern int is_abrev(char *s1, char *s2);
extern int isatom_inline(char *arg,struct session *ses);
extern int listlength_inline(char *arg,struct session *ses);
extern int match(char *regex, char *string);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern int random_inline(char *arg, struct session *ses);
extern void set_variable(char *left,char *right,struct session *ses);
extern int strlen_inline(char *arg, struct session *ses);
extern int grep_inline(char *arg, struct session *ses);
extern int strcmp_inline(char *arg, struct session *ses);
extern int match_inline(char *arg, struct session *ses);
extern int ord_inline(char *arg, struct session *ses);
extern void substitute_myvars(char *arg,char *result,struct session *ses);
extern void substitute_vars(char *arg, char *result);
extern void tintin_printf(struct session *ses,char *format,...);
extern void tintin_eprintf(struct session *ses,char *format,...);
extern char *get_arg(char *s,char *arg,int flag,struct session *ses);


extern char tintin_char;

/*********************/
/* the #math command */
/*********************/
void math_command(char *line, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE];
    int i;

    line = get_arg(line, left, 0, ses);
    line = get_arg(line, right, 1, ses);
    if (!*left||!*right)
    {
        tintin_eprintf(ses,"#Syntax: #math <variable> <expression>");
        return;
    };
    i = eval_expression(right,ses);
    sprintf(temp, "%d", i);
    set_variable(left, temp, ses);
}

/*******************/
/* the #if command */
/*******************/
struct session *if_command(char *line, struct session *ses)
{
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    /* int i; */
    line = get_arg(line, left, 0, ses);
    line = get_arg_in_braces(line, right, 1);

    if (!*left || !*right)
    {
        tintin_eprintf(ses,"#ERROR: valid syntax is: if <condition> <command> [#elif <condition> <command>] [...] [#else <command>]");
        return ses;
    }

    if (eval_expression(left,ses))
        ses=parse_input(right,1,ses);
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


int do_inline(char *line,int *res,struct session *ses)
{
    char command[BUFFER_SIZE],*ptr;

    ptr=command;
    while (*line&&(*line!=' '))
        *ptr++=*line++;
    *ptr=0;
    line=space_out(line);
    /*
       tintin_printf(ses,"#executing inline command [%c%s] with [%s]",tintin_char,command,line);
    */	
    if (is_abrev(command,"finditem"))
        *res=finditem_inline(line,ses);
    else if (is_abrev(command,"isatom"))
        *res=isatom_inline(line,ses);
    else if (is_abrev(command,"listlength"))
        *res=listlength_inline(line,ses);
    else if (is_abrev(command,"strlen"))
        *res=strlen_inline(line,ses);
    else if (is_abrev(command,"random"))
        *res=random_inline(line,ses);
    else if (is_abrev(command,"grep"))
        *res=grep_inline(line,ses);
    else if (is_abrev(command,"strcmp"))
        *res=strcmp_inline(line,ses);
    else if (is_abrev(command,"match"))
        *res=match_inline(line,ses);
    else if (is_abrev(command,"ord"))
        *res=ord_inline(line,ses);
    else
    {
        tintin_eprintf(ses,"#Unknown inline command [%c%s]!",tintin_char,command);
        return 0;
    };

    return 1;
}


int eval_expression(char *arg,struct session *ses)
{
    int i, begin, end, flag, prev;

    i = conv_to_ints(arg,ses);
    if (i)
    {
        while (1)
        {
            i = 0;
            flag = 1;
            begin = -1;
            end = -1;
            prev = -1;
            while (stacks[i][0] && flag)
            {
                if (stacks[i][1] == 0)
                {
                    begin = i;
                }
                else if (stacks[i][1] == 1)
                {
                    end = i;
                    flag = 0;
                }
                prev = i;
                i = stacks[i][0];
            }
            if ((flag && (begin != -1)) || (!flag && (begin == -1)))
            {
                tintin_eprintf(ses,"#Unmatched parentheses error in {%s}.",arg);
                return 0;
            }
            if (flag)
            {
                if (prev == -1)
                    return (stacks[0][2]);
                begin = -1;
                end = i;
            }
            i = do_one_inside(begin, end);
            if (!i)
            {
                tintin_eprintf(ses, "#Invalid expression to evaluate in {%s}", arg);
                return 0;
            }
        }
    }
    else
        return 0;
}

static int conv_to_ints(char *arg,struct session *ses)
{
    int i, flag, result;
    int m; /* =0 should match, =1 should differ */
    int regex; /* =0 strncmp, =1 regex match */
    char *ptr, *tptr;
    char temp[BUFFER_SIZE];
    char left[BUFFER_SIZE], right[BUFFER_SIZE];

    regex=0; /* die lint die */
    i = 0;
    ptr = arg;
    while (*ptr)
    {
        if (*ptr == ' ') ;
        else if (*ptr == tintin_char)
            /* inline commands */
        {
            ptr=get_inline(ptr+1,temp)-1;
            if (!do_inline(temp,&(stacks[i][2]),ses))
                return 0;
            stacks[i][1]=15;
        }
        /* jku: comparing strings with = and != */
        else if (*ptr == '[')
        {
            ptr++;
            tptr=left;
            while((*ptr) && (*ptr != ']') && (*ptr != '=') && (*ptr != '!')) {
                *tptr = *ptr;
                ptr++;
                tptr++;
            }
            *tptr='\0';
            if(!*ptr)
                return 0; /* error */
            if(*ptr == ']')
                tintin_eprintf(ses, "#Compare %s to what ? (only one var between [ ])", left);
            /* fprintf(stderr, "Left argument = '%s'\n", left); */
            switch(*ptr)
            {
            case '!' :
                ptr++;
                m=1;
                switch(*ptr)
                {
                case '=' : regex=0; ptr++; break;
                case '~' : regex=1; ptr++; break;
                default : return 0;
                }
                break;
            case '=' :
                ptr++;
                m=0;
                switch(*ptr)
                {
                case '=' : regex=0; ptr++; break;
                case '~' : regex=1; ptr++; break;
                default : break;
                }
                break;
            default : return 0;
            }

            /* fprintf(stderr, "%c - %s match\n", (m) ? '=' : '!', (regex) ? "regex" : "string"); */

            tptr=right;
            while((*ptr) && (*ptr != ']'))
            {
                *tptr = *ptr;
                ptr++;
                tptr++;
            }
            *tptr='\0';
            /* fprintf(stderr, "Right argument = '%s'\n", right); */
            if(!*ptr)
                return 0;
            if(regex)
                result = match(right, left) ? 0 : 1;
            else
                result = strcmp(left, right);
            if((result == 0 && m == 0) || (result != 0 && m != 0))
            { /* success */
                stacks[i][1] = 15;
                stacks[i][2] = 1;
                /* fprintf(stderr, "Expr TRUE\n"); */
            }
            else
            {
                stacks[i][1] = 15;
                stacks[i][2] = 0;
                /* fprintf(stderr, "Expr FALSE\n"); */
            }
        }
        /* jku: end of comparing strings */
        /* jku: undefined variables are now assigned value 0 (false) */
        else if (*ptr == '$')
        {
            if (ses->mesvar[5])
                tintin_eprintf(ses, "#Undefined variable in {%s}.", arg);
            stacks[i][1] = 15;
            stacks[i][2] = 0;
            if (*(++ptr)=='{')
            {
                ptr=get_arg_in_braces(ptr,temp,0);
            }
            else
            {
                while (isalpha(*ptr) || *ptr=='_' || isadigit(*ptr))
                    ptr++;
            }
            ptr--;
        }
        /* jku: end of changes */
        else if (*ptr == '(') {
            stacks[i][1] = 0;
        } else if (*ptr == ')') {
            stacks[i][1] = 1;
        } else if (*ptr == '!') {
            if (*(ptr + 1) == '=') {
                stacks[i][1] = 12;
                ptr++;
            } else
                stacks[i][1] = 2;
        } else if (*ptr == '*') {
            stacks[i][1] = 3;
            stacks[i][3] = 0;
        } else if (*ptr == '/') {
            stacks[i][1] = 3;
            stacks[i][3] = 1;
        } else if (*ptr == '+') {
            stacks[i][1] = 5;
            stacks[i][3] = 2;
        } else if (*ptr == '-') {
            flag = -1;
            if (i > 0)
                flag = stacks[i - 1][1];
            if (flag == 15)
            {
                stacks[i][1] = 5;
                stacks[i][3] = 3;
            }
            else {
                tptr = ptr;
                ptr++;
                while (isdigit(*ptr))
                    ptr++;
                sscanf(tptr, "%d", &stacks[i][2]);
                stacks[i][1] = 15;
                ptr--;
            }
        } else if (*ptr == '>') {
            if (*(ptr + 1) == '=') {
                stacks[i][1] = 8;
                stacks[i][3] = 4;
                ptr++;
            } else {
                stacks[i][1] = 8;
                stacks[i][3] = 5;
            }
        } else if (*ptr == '<') {
            if (*(ptr + 1) == '=') {
                ptr++;
                stacks[i][1] = 8;
                stacks[i][3] = 6;
            } else {
                stacks[i][1] = 8;
                stacks[i][3] = 7;
            }
        } else if (*ptr == '=') {
            stacks[i][1] = 11;
            if (*(ptr + 1) == '=')
                ptr++;
        } else if (*ptr == '&') {
            stacks[i][1] = 13;
            if (*(ptr + 1) == '&')
                ptr++;
        } else if (*ptr == '|') {
            stacks[i][1] = 14;
            if (*(ptr + 1) == '|')
                ptr++;
        } else if (isdigit(*ptr)) {
            stacks[i][1] = 15;
            tptr = ptr;
            while (isdigit(*ptr))
                ptr++;
            sscanf(tptr, "%d", &stacks[i][2]);
            ptr--;
        } else if (*ptr == 'T') {
            stacks[i][1] = 15;
            stacks[i][2] = 1;
        } else if (*ptr == 'F') {
            stacks[i][1] = 15;
            stacks[i][2] = 0;
        } else {
            tintin_eprintf(ses,"#Error. Invalid expression in #if or #math in {%s}.",arg);
            return 0;
        }
        if (*ptr != ' ') {
            stacks[i][0] = i + 1;
            i++;
        }
        ptr++;
    }
    if (i > 0)
        stacks[i][0] = 0;
    return 1;
}

static int do_one_inside(int begin, int end)
{
    int prev, ptr, highest, loc, ploc, next;

    while (1)
    {
        ptr = 0;
        if (begin > -1)
            ptr = stacks[begin][0];
        highest = 16;
        loc = -1;
        ploc = -1;
        prev = -1;
        while (ptr < end)
        {
            if (stacks[ptr][1] < highest)
            {
                highest = stacks[ptr][1];
                loc = ptr;
                ploc = prev;
            }
            prev = ptr;
            ptr = stacks[ptr][0];
        }
        if (highest == 15) {
            if (begin > -1) {
                stacks[begin][1] = 15;
                stacks[begin][2] = stacks[loc][2];
                stacks[begin][0] = stacks[end][0];
                return 1;
            } else {
                stacks[0][0] = stacks[end][0];
                stacks[0][1] = 15;
                stacks[0][2] = stacks[loc][2];
                return 1;
            }
        } else if (highest == 2) {
            next = stacks[loc][0];
            if (stacks[next][1] != 15 || stacks[next][0] == 0) {
                return 0;
            }
            stacks[loc][0] = stacks[next][0];
            stacks[loc][1] = 15;
            stacks[loc][2] = !stacks[next][2];
        } else {
            next = stacks[loc][0];
            if (ploc == -1 || stacks[next][0] == 0 || stacks[next][1] != 15)
                return 0;
            if (stacks[ploc][1] != 15)
                return 0;
            switch (highest) {
            case 3:			/* highest priority is *,/ */
                stacks[ploc][0] = stacks[next][0];
                if (stacks[loc][3]==0)
                    stacks[ploc][2] *= stacks[next][2];
                else
                    if (stacks[next][2])
                        stacks[ploc][2] /= stacks[next][2];
                    else
                    {
                        stacks[ploc][2]=0;
                        tintin_eprintf(0, "#Error: Division by zero.");
                    }
                break;
            case 5:			/* highest priority is +,- */
                stacks[ploc][0] = stacks[next][0];
                if (stacks[loc][3]==2)
                    stacks[ploc][2] += stacks[next][2];
                else
                    stacks[ploc][2] -= stacks[next][2];
                break;
            case 8:			/* highest priority is >,>=,<,<= */
                stacks[ploc][0] = stacks[next][0];
                switch(stacks[loc][3])
                {
                case 5:
                    stacks[ploc][2] = (stacks[ploc][2] > stacks[next][2]);
                    break;
                case 4:
                    stacks[ploc][2] = (stacks[ploc][2] >= stacks[next][2]);
                    break;
                case 7:
                    stacks[ploc][2] = (stacks[ploc][2] < stacks[next][2]);
                    break;
                case 6:
                    stacks[ploc][2] = (stacks[ploc][2] <= stacks[next][2]);
                }
                break;
            case 11:			/* highest priority is == */
                stacks[ploc][0] = stacks[next][0];
                stacks[ploc][2] = (stacks[ploc][2] == stacks[next][2]);
                break;
            case 12:			/* highest priority is != */
                stacks[ploc][0] = stacks[next][0];
                stacks[ploc][2] = (stacks[ploc][2] != stacks[next][2]);
                break;
            case 13:			/* highest priority is && */
                stacks[ploc][0] = stacks[next][0];
                stacks[ploc][2] = (stacks[ploc][2] && stacks[next][2]);
                break;
            case 14:			/* highest priority is || */
                stacks[ploc][0] = stacks[next][0];
                stacks[ploc][2] = (stacks[ploc][2] || stacks[next][2]);
                break;
            default:
                tintin_eprintf(0,"#Programming error *slap Bill*");
                return 0;
            }
        }
    }
}
