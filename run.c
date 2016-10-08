#define _XOPEN_SOURCE 500
#define __EXTENSIONS__
#include "tintin.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#include "protos/print.h"
#include "protos/pty.h"
#include "protos/utils.h"

extern char **environ;


#ifdef TERM_DEBUG
static void print_stty(int fd)
{
    struct termios ta;
    struct winsize ws;
    char buf[BUFFER_SIZE], *bptr;
#define battr(c, a, b) bptr+=sprintf(bptr, " %s%s~7~", (ta.c_##c&a)?"~9~":"~2~-~4~", b);

    memset(&ta, 0, sizeof(ta));
    memset(&ws, 0, sizeof(ws));
    tintin_printf(0, "~7~pty attributes (fd=%d):", fd);
    if (tcgetattr(fd, &ta))
        tintin_printf(0, " attrs: unknown");
    else
    {
        tintin_printf(0, " attrs: cflag=~3~%x~7~, iflag=~3~%x~7~, oflag=~3~%x~7~, lflag=~3~%x~7~",
            ta.c_cflag, ta.c_iflag, ta.c_oflag, ta.c_lflag);
        bptr=buf+sprintf(buf, " ~3~[%x]~7~:", ta.c_cflag);
        battr(cflag, PARENB, "parenb");
        battr(cflag, PARODD, "parodd");
        battr(cflag, CS8, "cs8");
        battr(cflag, HUPCL, "hupcl");
        battr(cflag, CSTOPB, "cstopb");
        battr(cflag, CREAD, "cread");
        battr(cflag, CLOCAL, "clocal");
#ifdef CRTSCTS
        battr(cflag, CRTSCTS, "crtscts");
#endif
        tintin_printf(0, "%s", buf);
        bptr=buf+sprintf(buf, " ~3~[%x]~7~:", ta.c_iflag);
        battr(iflag, IGNBRK, "ignbrk");
        battr(iflag, BRKINT, "brkint");
        battr(iflag, IGNPAR, "ignpar");
        battr(iflag, PARMRK, "parmrk");
        battr(iflag, INPCK, "inpck");
        battr(iflag, ISTRIP, "istrip");
        battr(iflag, INLCR, "inlcr");
        battr(iflag, IGNCR, "igncr");
        battr(iflag, ICRNL, "icrnl");
        battr(iflag, IXON, "ixon");
        battr(iflag, IXOFF, "ixoff");
#ifdef IUCLC
        battr(iflag, IUCLC, "iuclc");
#endif
#ifdef IXANY
        battr(iflag, IXANY, "ixany");
#endif
#ifdef IMAXBEL
        battr(iflag, IMAXBEL, "imaxbel");
#endif
        tintin_printf(0, "%s", buf);
        bptr=buf+sprintf(buf, " ~3~[%x]~7~:", ta.c_oflag);
        battr(oflag, OPOST, "opost");
        battr(oflag, OLCUC, "olcuc");
        battr(oflag, OCRNL, "ocrnl");
        battr(oflag, ONLCR, "onlcr");
        battr(oflag, ONOCR, "onocr");
        battr(oflag, ONLRET, "onlret");
#ifdef OFILL
        battr(oflag, OFILL, "ofill");
#endif
#ifdef OFDEL
        battr(oflag, OFDEL, "ofdel");
#endif
/*
        battr(oflag, NL0, "nl0");
        battr(oflag, CR0, "cr0");
        battr(oflag, TAB0, "tab0");
        battr(oflag, BS0, "bs0");
        battr(oflag, VT0, "vt0");
        battr(oflag, FF0, "ff0");
*/
        tintin_printf(0, "%s", buf);
        bptr=buf+sprintf(buf, " ~3~[%x]~7~:", ta.c_lflag);
        battr(lflag, ISIG, "isig");
        battr(lflag, ICANON, "icanon");
        battr(lflag, IEXTEN, "iexten");
        battr(lflag, ECHO, "echo");
        battr(lflag, ECHOE, "echoe");
        battr(lflag, ECHOK, "echok");
        battr(lflag, ECHONL, "echonl");
        battr(lflag, NOFLSH, "noflsh");
#ifdef XCASE
        battr(lflag, XCASE, "xcase");
#endif
        battr(lflag, TOSTOP, "tostop");
#ifdef ECHOPRT
        battr(lflag, ECHOPRT, "echoprt");
#endif
#ifdef ECHOCTL
        battr(lflag, ECHOCTL, "echoctl");
#endif
#ifdef ECHOKE
        battr(lflag, ECHOKE, "echoke");
#endif
        tintin_printf(0, "%s", buf);
    }
    if (ioctl(fd, TIOCGWINSZ, &ws))
        tintin_printf(0, " window size: unknown");
    else
        tintin_printf(0, " window size: %dx%d",
            ws.ws_col, ws.ws_row);
}

void termdebug_command(const char *arg, struct session *ses)
{
    print_stty(ses->socket);
}
#endif


void pty_resize(int fd, int sx, int sy)
{
    struct winsize ws;

    if (sx>0 && sy>0)
    {
        ws.ws_row=sy;
        ws.ws_col=sx;
        ws.ws_xpixel=0;
        ws.ws_ypixel=0;
        ioctl(fd, TIOCSWINSZ, &ws);
    }
}

int run(const char *command, int sx, int sy, const char *term)
{
    int fd, res;

#if defined(__FreeBSD_kernel__) && defined(__GLIBC__)
    /* Work around a kfreebsd 9.x bug.  If the handler for SIGCHLD is anything
       but SIG_DFL, grantpt() and thus forkpty() doesn't work. */
    struct sigaction act, oldact;
    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_RESTART;
    act.sa_handler=SIG_DFL;
    sigaction(SIGCHLD, &act, &oldact);
#endif

#ifndef PTY_ECHO_HACK
    struct winsize ws;

    ws.ws_row=sy;
    ws.ws_col=sx;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;
    res = forkpty(&fd, 0, 0, (sy>0 && sx>0)?&ws:0);
#else
    res = forkpty(&fd, 0, 0, 0);
#endif

#if defined(__FreeBSD_kernel__) && defined(__GLIBC__)
    int err = errno;
    sigaction(SIGCHLD, &oldact, 0);
    while (waitpid(-1, 0, WNOHANG)>0);
    errno = err;
#endif

    switch (res)
    {
    case -1:
        return -1;
    case 0:
        {
            struct termios ta;
            const char *argv[4];
            char cmd[BUFFER_SIZE+5];

            tcgetattr(1, &ta);
            cfmakeraw(&ta);
            tcsetattr(1, TCSANOW, &ta);

            sprintf(cmd, "exec %s", command);
            argv[0]="sh";
            argv[1]="-c";
            argv[2]=cmd;
            argv[3]=0;
            if (term)
                setenv("TERM", term, 1); /* TERM=KBtin.  Or should we lie? */
            execve("/bin/sh", (char*const*)argv, environ);
            fprintf(stderr, "#ERROR: Couldn't exec `%s'\n", command);
            exit(127);
        }
    default:
        return fd;
    }
}


FILE* mypopen(const char *command, bool wr, int ofd)
{
    int p[2];

    if (pipe(p))
    {
        close(ofd);
        return 0;
    }
    switch (fork())
    {
    case -1:
        close(p[0]);
        close(p[1]);
        close(ofd);
        return 0;
    case 0:
        {
            const char *argv[4];
            char cmd[BUFFER_SIZE+5];

            if (!wr)
            {
                close(p[0]);
                if (ofd==-1)
                    ofd=open("/dev/null", O_RDONLY);
                if (ofd!=-1)
                    dup2(ofd, 0);
                dup2(p[1], 1);
                dup2(p[1], 2);
                close(p[1]);
            }
            else
            {
                close(p[1]);
                if (ofd==-1)
                    ofd=open("/dev/null", O_WRONLY);
                if (ofd!=-1)
                    dup2(ofd, 1), dup2(ofd, 2);
                dup2(p[0], 0);
                close(p[0]);
                signal(SIGINT, SIG_IGN);
                signal(SIGHUP, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
            }
            if (ofd>2)
                close(ofd);
            sprintf(cmd, "exec %s", command);
            argv[0]="sh";
            argv[1]="-c";
            argv[2]=cmd;
            argv[3]=0;
            execve("/bin/sh", (char*const*)argv, environ);
            fprintf(stderr, "#ERROR: Couldn't exec `%s'\n", command);
            exit(127);
        }
    default:
        close(ofd);
        close(p[!wr]);
        return fdopen(p[wr], wr?"w":"r");
    }
}

void pty_write_line(const char *line, int pty)
{
    char out[4*BUFFER_SIZE+1];
    int len;
#ifdef PTY_ECHO_HACK
    struct termios ta, oldta;

    tcgetattr(pty, &oldta);
    memcpy(&ta, &oldta, sizeof(ta));
    cfmakeraw(&ta);
    ta.c_cc[VMIN]=MAX_INPUT;
    tcsetattr(pty, TCSANOW, &ta);
#else
# ifdef RESET_RAW
    struct termios ta;

    tcgetattr(pty, &ta);
    cfmakeraw(&ta);
    tcsetattr(pty, TCSANOW, &ta);
# endif
#endif

    len=sprintf(out, "%s\n", line);
    if (write(pty, out, len) == -1)
        syserr("write in pty_write_line()");

#ifdef PTY_ECHO_HACK
    /* FIXME: if write() blocks, they'll act in raw mode */
    tcsetattr(pty, TCSANOW, &oldta);
    sleep(0);   /* let'em act */
#endif
}
