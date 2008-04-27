#include "tintin.h"
#include "unicode.h"
#include "ui.h"
#include "protos.h"

extern int colors[];
const char *attribs[8]={"",";5",";3",";3;5",";4",";4;5",";4;3",";4;3;5"};
#ifdef GRAY2
const char *fcolors[16]={";30",  ";34",  ";32",  ";36",
                            ";31",  ";35",  ";33",  "",
                        ";2",   ";34;1",";32;1",";36;1",
                            ";31;1",";35;1",";33;1",";1"};
const char *bcolors[8]={"",    ";44",    ";42",   ";46",
                            ";41",  ";45",  ";43",  "47"};
#define COLORCODE(c) "\033[0%s%s%sm",fcolors[(c)&15],bcolors[((c)>>4)&7],attribs[(c))>>7]
#else
#define COLORCODE(c) "\033[0%s;3%d;4%d%sm",((c)&8)?";1":"",colors[(c)&7],colors[((c)>>4)&7],attribs[(c)>>7]
#endif

char done_input[BUFFER_SIZE];
int tty;
int isstatus;

int ui_sep_input=0;
int ui_con_buffer=0;
int ui_keyboard=0;
int ui_own_output=0;
int ui_tty=1;
int ui_drafts=0;

int LINES=0;
int COLS=0;

void user_setdriver(int dr)
{
    switch(dr)
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

void user_illegal()
{
    syserr("DRIVER: operation not supported");
}

void user_noop()
{
}
