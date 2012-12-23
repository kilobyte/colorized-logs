#include "unicode.h"

int ui_sep_input;
int ui_con_buffer;
int ui_keyboard;
int ui_own_output;
int ui_tty;
int ui_drafts;

int LINES,COLS,tty;
char done_input[BUFFER_SIZE];
int color,lastcolor;

extern int colors[];
extern const char *attribs[8];
#ifdef GRAY2
extern const char *fcolors[16];
extern const char *bcolors[8];
#define COLORCODE(c) "\033[0%s%s%sm",fcolors[(c)&15],bcolors[((c)>>4)&7],attribs[(c))>>7]
#else
#define COLORCODE(c) "\033[0%s;3%d;4%d%sm",((c)&8)?";1":"",colors[(c)&7],colors[((c)>>4)&7],attribs[(c)>>7]
#endif

typedef void (voidfunc)(void);
typedef void (voidintfunc)(int x);
typedef void (voidcharpfunc)(char *txt);
typedef void (voidcharpintfunc)(char *txt, int i);
typedef int (processkbdfunc)(struct session *ses, WC ch);
typedef void (voidFILEpfunc)(FILE *f);
typedef void (printffunc)(char *fmt,...);

voidfunc *user_init;
voidfunc *user_pause;
voidfunc *user_resume;
voidcharpfunc *user_textout;
voidcharpintfunc *user_textout_draft;
processkbdfunc *user_process_kbd;
voidfunc *user_beep;
voidfunc *user_done;
voidintfunc *user_keypad;
voidfunc *user_retain;
voidintfunc *user_passwd;
voidFILEpfunc *user_condump;
printffunc *user_title;
voidfunc *user_resize;
voidfunc *user_show_status;
voidfunc *user_mark_greeting;
