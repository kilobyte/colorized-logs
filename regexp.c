#include "tintin.h"
#include "protos.h"
#ifdef HAVE_REGCOMP
#include <sys/types.h>
#include <regex.h>

extern char tintin_char;
extern pvars_t *pvars;	/* the %0, %1, %2,....%9 variables */

extern struct session *if_command(char *arg, struct session *ses);

int check_regexp(char *line, char *action, pvars_t *vars, int inside, struct session *ses)
{
    regex_t preg;
    regmatch_t pmatch[10];
    int i;

    if (regcomp(&preg,action,REG_EXTENDED))
    {
        tintin_eprintf(ses,"#invalid regular expression: {%s}",action);
        return 0;
    }
    if (regexec(&preg,line,vars?10:0,pmatch,inside?REG_NOTBOL:0))
    {
        regfree(&preg);
        return FALSE;
    }
    if (vars)
        for (i = 0; i < 10; i++)
        {
            if (pmatch[i].rm_so != -1)
            {
                strncpy((*vars)[i], line+pmatch[i].rm_so,
                                    pmatch[i].rm_eo-pmatch[i].rm_so);
                *((*vars)[i] + pmatch[i].rm_eo-pmatch[i].rm_so) = '\0';
            }
            else
                (*vars)[i][0]=0;
        }
    regfree(&preg);
    return TRUE;
}

/*********************/
/* the #grep command */
/*********************/
void grep_command(char *arg, struct session *ses)
{
    pvars_t vars,*lastpvars;
    char left[BUFFER_SIZE], line[BUFFER_SIZE], right[BUFFER_SIZE];
    int flag=0;
    
    arg=get_arg(arg, left, 0, ses);
    arg=get_arg(arg, line, 0, ses);
    arg=get_arg_in_braces(arg, right, 0);
    
    if (!*left || !*right)
    {
        tintin_eprintf(ses,"#ERROR: valid syntax is: #grep <pattern> <line> <command> [#else ...]");
        return;
    }

    if (check_regexp(line, left, &vars, 0, ses))
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
        tintin_eprintf(ses,"#ERROR: cruft after #grep: {%s}",left);
}


/********************/
/* the #grep inline */
/********************/
int grep_inline(char *arg, struct session *ses)
{
    char left[BUFFER_SIZE], line[BUFFER_SIZE];
    
    arg=get_arg(arg, left, 0, ses);
    arg=get_arg(arg, line, 1, ses);
    
    if (!*left)
    {
        tintin_eprintf(ses,"#ERROR: valid syntax is: (#grep <pattern> <line>)");
        return 0;
    }
    return check_regexp(line, left, 0, 0, ses);
}


#endif
