#include "tintin.h"

bool term_echoing = true;
bool keypad= DEFAULT_KEYPAD;
bool retain= DEFAULT_RETAIN;
int alnum = 0;
int acnum = 0;
int subnum = 0;
int varnum = 0;
int hinum = 0;
int routnum = 0;
int pdnum = 0;
int antisubnum = 0;
int bindnum = 0;
int hooknum = 0;
int gotpassword=0;
bool got_more_kludge=false;
int hist_num;
bool need_resize=false;
char *tintin_exec;
struct session *lastdraft;
bool aborting=false;
bool any_closed=false;
int in_read=0;
char *_; /* incoming line being processed */
bool real_quiet=false; /* if set, #verbose 0 will be really quiet */
char *history[HISTORY_SIZE];
struct session *sessionlist, *activesession, *nullsession;
pvars_t *pvars; /* the %0, %1, %2,....%9 variables */
char tintin_char = DEFAULT_TINTIN_CHAR;
char verbatim_char = DEFAULT_VERBATIM_CHAR;
char status[BUFFER_SIZE];
int LINES, COLS;
bool isstatus;
