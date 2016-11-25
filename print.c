#include "tintin.h"
#include "protos/action.h"
#include "protos/antisub.h"
#include "protos/globals.h"
#include "protos/highlight.h"
#include "protos/print.h"
#include "protos/misc.h"
#include "protos/substitute.h"
#include "protos/user.h"

bool puts_echoing = true;

/****************************************************/
/* output to screen should go through this function */
/* text gets checked for actions                    */
/****************************************************/
void tintin_puts(const char *cptr, struct session *ses)
{
    char line[BUFFER_SIZE];
    strcpy(line, cptr);
    if (ses)
    {
        _=line;
        check_all_actions(line, ses);
        _=0;
    }
    tintin_printf(ses, line);
}

/****************************************************/
/* output to screen should go through this function */
/* text gets checked for substitutes and actions    */
/****************************************************/
void tintin_puts1(const char *cptr, struct session *ses)
{
    char line[BUFFER_SIZE];

    strcpy(line, cptr);

    _=line;
    if (!ses->presub && !ses->ignore)
        check_all_actions(line, ses);
    if (!ses->togglesubs)
        if (!do_one_antisub(line, ses))
            do_all_sub(line, ses);
    if (ses->presub && !ses->ignore)
        check_all_actions(line, ses);
    if (!ses->togglesubs)
        do_all_high(line, ses);
    if (isnotblank(line, ses->blank))
        if (ses==activesession)
        {
            char *cp=strchr(line, 0);
            if (cp-line>=BUFFER_SIZE-2)
                cp=line+BUFFER_SIZE-2;
            cp[0]='\n';
            cp[1]=0;
            user_textout(line);
        }
    _=0;
}

void tintin_printf(struct session *ses, const char *format, ...)
{
    va_list ap;
    char buf[BUFFER_SIZE];

    if ((ses == activesession || ses == nullsession || !ses) && puts_echoing)
    {
        va_start(ap, format);
        int n=vsnprintf(buf, BUFFER_SIZE-1, format, ap);
        if (n>BUFFER_SIZE-2)
            buf[BUFFER_SIZE-3]='>';
        va_end(ap);
        strcpy(buf+n, "\n");
        user_textout(buf);
    }
}

void tintin_eprintf(struct session *ses, const char *format, ...)
{
    va_list ap;
    char buf[BUFFER_SIZE];

    /* note: the behavior on !ses is wrong */
    if ((ses == activesession || ses == nullsession || !ses)
        && (puts_echoing||!ses||ses->mesvar[MSG_ERROR]))
    {
        va_start(ap, format);
        int n=vsnprintf(buf, BUFFER_SIZE-1, format, ap);
        if (n>BUFFER_SIZE-2)
            buf[BUFFER_SIZE-3]='>';
        va_end(ap);
        strcpy(buf+n, "\n");
        user_textout(buf);
    }
}
