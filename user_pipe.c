#include "tintin.h"
#include "ui.h"
#include "protos/colors.h"
#include "protos/unicode.h"
#include "protos/utils.h"


mbstate_t outstate;
#define OUTSTATE &outstate
extern void user_illegal();
extern void user_noop();
extern int need_resize;


static char *i_pos;


static void userpipe_init(void)
{
    tty=isatty(1);
    color=lastcolor=7;
    i_pos=done_input;
    memset(&outstate, 0, sizeof(outstate));
}

static void userpipe_textout(char *txt)
{
    char buf[BUFFER_SIZE],*a,*b;

    for (a=txt,b=buf; *a; )
        switch (*a)
        {
        case '~':
            if (getcolor(&a,&color,1))
            {
                if (color==-1)
                    color=lastcolor;
                if (tty)
                    b+=sprintf(b,COLORCODE(color));
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
    write_stdout(buf,b-buf);
}

static int userpipe_process_kbd(struct session *ses, WC ch)
{

    switch (ch)
    {
    case '\n':
        *i_pos=0;
        i_pos=done_input;
        return 1;
    case 8:
        if (i_pos!=done_input)
            i_pos--;
        return 0;
    default:
        if (i_pos-done_input>=BUFFER_SIZE-8)
            return 0;
        i_pos+=wc_to_utf8(i_pos, &ch, 1, BUFFER_SIZE-(i_pos-done_input));
    case 0:
        ;
    }
    return 0;
}

static void userpipe_beep(void)
{
    write_stdout("\007",1);
    /* should it beep if we're redirected to a pipe? */
}

static void userpipe_resize(void)
{
    need_resize=0;
}

void userpipe_initdriver()
{
    ui_sep_input=0;
    ui_con_buffer=0;
    ui_keyboard=0;
    ui_own_output=0;
    ui_tty=1;
    ui_drafts=0;

    user_init           = userpipe_init;
    user_done           = user_noop;
    user_pause          = user_illegal;
    user_resume         = user_illegal;
    user_textout        = userpipe_textout;
    user_textout_draft  = user_noop;
    user_process_kbd    = userpipe_process_kbd;
    user_beep           = userpipe_beep;
    user_keypad         = user_illegal;
    user_retain         = user_illegal;
    user_passwd         = user_noop;
    user_condump        = user_illegal;
    user_title          = (printffunc*)user_illegal;
    user_resize         = userpipe_resize;
    user_show_status    = user_illegal;
    user_mark_greeting  = user_noop;
}
