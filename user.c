#include "tintin.h"
#include "protos/user_pipe.h"
#include "protos/user_tty.h"
#include "protos/utils.h"

char done_input[BUFFER_SIZE];
bool tty;

bool ui_sep_input=false;
bool ui_con_buffer=false;
bool ui_keyboard=false;
bool ui_own_output=false;
bool ui_drafts=false;

int LINES=0;
int COLS=0;

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
