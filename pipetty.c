#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "config.h"
#ifdef HAVE_PTY_H
# include <pty.h>
#endif
#ifdef HAVE_LIBUTIL_H
# include <libutil.h>
#endif
#ifdef HAVE_UTIL_H
# include <util.h>
#endif

extern void sigobit(int ret);

#define PN "pipetty"

void syserr(const char *msg, ...)
{
    int err=errno;
    va_list ap;

    fprintf(stderr, PN ": ");
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    if (err)
        fprintf(stderr, ": %s", strerror(err));
    fprintf(stderr, "\n");
    exit(1);
}

static int basename_is(const char *proc, const char *name)
{
    int plen = strlen(proc);
    int nlen = strlen(name);

    if (plen==nlen && !memcmp(proc, name, nlen))
        return 1;
    if (plen>nlen && proc[plen-nlen-1]=='/' && !memcmp(proc+plen-nlen, name, nlen))
        return 1;
    return 0;
}

int main(int argc, const char **argv)
{
    if (argc<2)
        errno=0, syserr("missing command");

    int master, slave;
    if (openpty(&master, &slave, 0, 0/*termios*/, 0/*winsize*/))
        syserr("can't allocate a pseudo-terminal");
    if (master>31)
        errno=0, syserr("bad fd from openpty(): %d", master);

    struct termios ti;
    if (!tcgetattr(slave, &ti))
    {
        cfmakeraw(&ti);
        tcsetattr(slave, TCSANOW, &ti);
    }

    int pid=fork();
    if (pid==-1)
        syserr("fork failed");
    if (!pid)
    {
        putenv("PAGER=cat");
        close(master);
        setsid();
        dup2(slave, 1);
        dup2(slave, 2);
        close(slave);
        int zero=0;
        ioctl(1, TIOCSCTTY, &zero);
        execvp(argv[1], (char*const*)argv+1);
        syserr("%s", argv[1]);
        return 127;
    }
    close(slave);

    int less = basename_is(argv[0], "lesstty");
    if (less)
    {
        int p[2];
        if (pipe(p))
            syserr("pipe failed");

        less=fork();
        if (less==-1)
            syserr("fork failed");
        if (!less)
        {
            close(p[1]);
            dup2(p[0], 0);
            close(p[0]);

            execlp("less", "less", "-R", "-", NULL);
            syserr("can't run less");
            return 127;
        }

        close(p[0]);
        dup2(p[1], 1);
        close(p[1]);
    }

    char buf[16384];
    int r;
    while ((r=read(master, buf, sizeof(buf)))>0)
    {
        if (write(1, buf, r)!=r)
            syserr("error writing to stdout");
    }
    close(master);

    int ret;
    if (waitpid(pid, &ret, 0)==-1)
        syserr("waitpid failed");
    if (WIFSIGNALED(ret))
        sigobit(ret);
    if (less)
    {
        close(1);
        waitpid(less, 0, 0);
    }
    return WIFEXITED(ret)?WEXITSTATUS(ret):WTERMSIG(ret)+128;
}
