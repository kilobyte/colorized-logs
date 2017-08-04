extern int openpty(int *amaster, int *aslave, char *dummy, struct termios *termp, struct winsize *wp);
extern int forkpty(int *amaster, char *dummy, struct termios *termp, struct winsize *wp);
extern void cfmakeraw(struct termios *ta);
