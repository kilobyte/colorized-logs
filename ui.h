#include "unicode.h"

#define COLORCODE(c) "\033[0%s;3%d;4%d%sm",((c)&8)?";1":"",colors[(c)&7],colors[((c)>>4)&7],attribs[(c)>>7]

typedef void (voidfunc)(void);
typedef void (voidboolfunc)(bool x);
typedef void (voidcharpfunc)(const char *txt);
typedef void (voidcharpboolfunc)(const char *txt, bool x);
typedef bool (processkbdfunc)(struct session *ses, WC ch);
typedef void (voidFILEpfunc)(FILE *f);
typedef void (printffunc)(const char *fmt, ...);

#include "protos/user.h"
