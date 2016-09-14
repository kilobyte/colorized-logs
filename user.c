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

void (*user_init)();
void (*user_pause)();
void (*user_resume)();
void (*user_textout)(const char*);
void (*user_textout_draft)(const char*, bool);
bool (*user_process_kbd)(struct session*, WC);
void (*user_beep)();
void (*user_done)();
void (*user_keypad)(bool);
void (*user_retain)();
void (*user_passwd)(bool);
void (*user_condump)(FILE*);
void (*user_title)(const char *, ...);
void (*user_resize)();
void (*user_show_status)();
void (*user_mark_greeting)();


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
