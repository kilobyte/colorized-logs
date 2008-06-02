#include "tintin.h"
#include "protos/action.h"
#include "protos/antisub.h"
#include "protos/bind.h"
#include "protos/colors.h"
#include "protos/files.h"
#include "protos/glob.h"
#include "protos/hash.h"
#include "protos/highlight.h"
#include "protos/history.h"
#include "protos/hooks.h"
#include "protos/llist.h"
#include "protos/print.h"
#include "protos/misc.h"
#include "protos/net.h"
#include "protos/parse.h"
#include "protos/session.h"
#include "protos/substitute.h"
#include "protos/ticks.h"
#include "protos/unicode.h"
#include "protos/user.h"
#include "protos/utils.h"
#include "protos/print.h"
#include "unicode.h"
#include "ui.h"

extern char *_;
extern struct session *sessionlist, *activesession, *nullsession;
int puts_echoing = TRUE;

/*****************************************************/
/* output to screen should go throught this function */
/* text gets checked for actions                     */
/*****************************************************/
void tintin_puts(char *cptr, struct session *ses)
{
    char line[BUFFER_SIZE];
    strcpy(line,cptr);
    if (ses)
    {
        _=line;
        check_all_actions(line, ses);
        _=0;
    }
    tintin_printf(ses,line);
}

/*****************************************************/
/* output to screen should go throught this function */
/* text gets checked for substitutes and actions     */
/*****************************************************/
void tintin_puts1(char *cptr, struct session *ses)
{
    char line[BUFFER_SIZE];

    strcpy(line,cptr);

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
    if (isnotblank(line,ses->blank))
        if (ses==activesession)
        {
            cptr=strchr(line,0);
            if (cptr-line>=BUFFER_SIZE-2)
                cptr=line+BUFFER_SIZE-2;
            cptr[0]='\n';
            cptr[1]=0;
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
        if (vsnprintf(buf, BUFFER_SIZE-1, format, ap)>BUFFER_SIZE-2)
            buf[BUFFER_SIZE-3]='>';
        va_end(ap);
        strcat(buf, "\n");
        user_textout(buf);
    }
}

void tintin_eprintf(struct session *ses, const char *format, ...)
{
    va_list ap;
    char buf[BUFFER_SIZE];

    /* note: the behavior on !ses is wrong */
    if ((ses == activesession || ses == nullsession || !ses) && (puts_echoing||!ses||ses->mesvar[11]))
    {
        va_start(ap, format);
        if (vsnprintf(buf, BUFFER_SIZE-1, format, ap)>BUFFER_SIZE-2)
            buf[BUFFER_SIZE-3]='>';
        va_end(ap);
        strcat(buf, "\n");
        user_textout(buf);
    }
}
