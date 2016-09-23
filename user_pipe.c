#include "tintin.h"
#include "protos/colors.h"
#include "protos/globals.h"
#include "protos/unicode.h"
#include "protos/user.h"
#include "protos/utils.h"


static mbstate_t outstate;
#define OUTSTATE &outstate
static char *i_pos;

static void userpipe_init(void)
{
    tty=isatty(1);
    color=lastcolor=7;
    i_pos=done_input;
    memset(&outstate, 0, sizeof(outstate));
}

static void userpipe_textout(const char *txt)
{
    char buf[BUFFER_SIZE], *b=buf;

    for (const char *a=txt; *a; )
        switch (*a)
        {
        case '~':
            if (getcolor(&a, &color, 1))
            {
                if (color==-1)
                    color=lastcolor;
                if (tty)
                    b=ansicolor(b, color);
            }
            else
                *b++='~';
            a++;
            break;
        case '\n':
            lastcolor=color;
        default:
            one_utf8_to_mb(&b, &a, &outstate);
        }
    write_stdout(buf, b-buf);
}

static bool userpipe_process_kbd(struct session *ses, WC ch)
{

    switch (ch)
    {
    case '\n':
        *i_pos=0;
        i_pos=done_input;
        return true;
    case 8:
        if (i_pos!=done_input)
            i_pos--;
        return false;
    default:
        if (i_pos-done_input>=BUFFER_SIZE-8)
            return false;
        i_pos+=wc_to_utf8(i_pos, &ch, 1, BUFFER_SIZE-(i_pos-done_input));
    case 0:
        ;
    }
    return false;
}

static void userpipe_beep(void)
{
    write_stdout("\007", 1);
    /* should it beep if we're redirected to a pipe? */
}

static void userpipe_passwd(bool x)
{
    // TODO: stty echo off?
}

static void userpipe_resize(void)
{
    need_resize=false;
}

static void user_illegal(void)
{
    syserr("DRIVER: operation not supported");
}

static void user_noop(void)
{
}

void userpipe_initdriver(void)
{
    ui_sep_input=false;
    ui_con_buffer=false;
    ui_keyboard=false;
    ui_own_output=false;
    ui_drafts=false;

    user_init           = userpipe_init;
    user_done           = user_noop;
    user_pause          = user_illegal;
    user_resume         = user_illegal;
    user_textout        = userpipe_textout;
    user_textout_draft  = (void (*)(const char*, bool))user_noop;
    user_process_kbd    = userpipe_process_kbd;
    user_beep           = userpipe_beep;
    user_keypad         = (void (*)(bool))user_illegal;
    user_retain         = user_illegal;
    user_passwd         = userpipe_passwd;
    user_condump        = (void (*)(FILE *))user_illegal;
    user_title          = (void (*)(const char*, ...))user_illegal;
    user_resize         = userpipe_resize;
    user_show_status    = user_illegal;
    user_mark_greeting  = user_noop;
}
