#include "unicode.h"

bool ui_sep_input;
bool ui_con_buffer;
bool ui_keyboard;
bool ui_own_output;
bool ui_drafts;

int LINES, COLS;
bool tty;
char done_input[BUFFER_SIZE];
int color, lastcolor;

#define COLORCODE(c) "\033[0%s;3%d;4%d%sm",((c)&8)?";1":"",colors[(c)&7],colors[((c)>>4)&7],attribs[(c)>>7]

typedef void (voidfunc)(void);
typedef void (voidboolfunc)(bool x);
typedef void (voidcharpfunc)(const char *txt);
typedef void (voidcharpboolfunc)(const char *txt, bool x);
typedef bool (processkbdfunc)(struct session *ses, WC ch);
typedef void (voidFILEpfunc)(FILE *f);
typedef void (printffunc)(const char *fmt, ...);

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
