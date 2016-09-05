#include "tintin.h"
#include "protos/user_pipe.h"
#include "protos/user_tty.h"
#include "protos/utils.h"
#include "ui.h"

bool tty;

bool ui_sep_input=false;
bool ui_con_buffer=false;
bool ui_keyboard=false;
bool ui_own_output=false;
bool ui_drafts=false;
char done_input[BUFFER_SIZE];
int color, lastcolor;

voidfunc *user_init;
voidfunc *user_pause;
voidfunc *user_resume;
voidcharpfunc *user_textout;
voidcharpboolfunc *user_textout_draft;
processkbdfunc *user_process_kbd;
voidfunc *user_beep;
voidfunc *user_done;
voidboolfunc *user_keypad;
voidfunc *user_retain;
voidboolfunc *user_passwd;
voidFILEpfunc *user_condump;
printffunc *user_title;
voidfunc *user_resize;
voidfunc *user_show_status;
voidfunc *user_mark_greeting;


void user_setdriver(int dr)
{
    switch (dr)
    {
    case 0:
        userpipe_initdriver();
        break;
    case 1:
        usertty_initdriver();
        break;
    default:
        syserr("No such driver: %d", dr);
    }
}
