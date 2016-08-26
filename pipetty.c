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
#include "protos/pty.h"

#define PN "pipetty"

void syserr(char *msg, ...)
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

    char buf[16384];
    int r;
    while ((r=read(master,buf,sizeof(buf)))>0)
    {
        if (write(1,buf,r)!=r)
            syserr("error writing to stdout");
    }

    int ret;
    if (waitpid(pid,&ret,0)==-1)
        syserr("waitpid failed");
    return WIFEXITED(ret)?WEXITSTATUS(ret):WTERMSIG(ret)+128;
}
