#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#ifdef HAVE_ASM_TERMIOS_H
#include <asm/termios.h>
#else
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#endif
#endif
#include <sys/stropts.h>
#include <stdlib.h>
#include "tintin.h"

extern char **environ;
extern int COLS,LINES;
extern struct termios old_tattr;
extern void syserr(char *msg);


int forkpty(int *amaster,struct termios *termp, struct winsize *wp)
{
    int master,slave;
    int pid;

#ifdef HAVE__GETPTY
    int filedes[2];
    char *line;

    line = _getpty(&filedes[0], O_RDWR|O_NDELAY, 0600, 0);
    if (0 == line)
        return -1;
    if (0 > (filedes[1] = open(line, O_RDWR)))
    {
        close(filedes[0]);
        return -1;
        master=filedes[0];
        slave=filedes[1];
    }
#else
#ifdef HAVE_PTSNAME
    char *name;
#else
    char name[80];
#endif

#ifdef HAVE_GETPT
    master=getpt();
#else
    master=open("/dev/ptmx", O_RDWR);
#endif

    if (master<0)
        return -1;

    if (grantpt(master)<0||unlockpt(master)<0)
        goto close_master;

#ifdef HAVE_PTSNAME
    if (!(name=(char*)ptsname(master)))
        goto close_master;
#else
    if (ptsname_r(master,name,80))
        goto close_master;
#endif

    slave=open(name,O_RDWR);
    if (slave==-1)
        goto close_master;

    if (isastream(slave))
        if (ioctl(slave, I_PUSH, "ptem")<0
                ||ioctl(slave, I_PUSH, "ldterm")<0)
            goto close_slave;

    goto ok;

close_slave:
    close (slave);

close_master:
    close (master);
    return -1;

ok:
#endif

    tcsetattr(slave, TCSANOW, termp);
    ioctl(slave,TIOCSWINSZ,wp);
    /* let's ignore errors on this ioctl silently */
    
    pid=fork();
    switch(pid)
    {
    case -1:
        close(master);
        close(slave);
        return -1;
    case 0:
        close(master);
        setsid();
        dup2(slave,0);
        dup2(slave,1);
        dup2(slave,2);
        close(slave);
        return 0;
    default:
        close(slave);
        *amaster=master;
        return pid;
    }
}

/********************/
/* the #run command */
/********************/
int run(char *command)
{

    int fd;

    struct termios ta;
    struct winsize ws;
    memcpy(&ta, &old_tattr, sizeof(ta));
    ta.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                    |INLCR|IGNCR|ICRNL|IXON);
    ta.c_oflag &= ~OPOST;
    ta.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    ta.c_cflag &= ~(CSIZE|PARENB);
    ta.c_cflag |= CS8;

    ta.c_lflag|=ISIG;    /* allow C-c, C-\ and C-z */
    ta.c_cc[VMIN]=1;
    ta.c_cc[VTIME]=0;

    ws.ws_row=LINES-1;
    ws.ws_col=COLS;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;

    switch(forkpty(&fd,&ta,&ws))
    {
    case -1:
        return(0);
    case 0:
        {
            char *argv[4];
            argv[0]="sh";
            argv[1]="-c";
            argv[2]=command;
            argv[3]=0;
            putenv("TERM=" TERM); /* TERM=KBtin.  Or should we lie? */
            execve("/bin/sh",argv,environ);
            fprintf(stderr,"#ERROR: Couldn't exec `%s'\n",command);
            exit(127);
        }
    default:
        return(fd);
    }

    return 0;
}


FILE *mypopen(char *command)
{
    int p[2];
    
    if (pipe(p))
        return 0;
    switch(fork())
    {
    case -1:
        close(p[0]);
        close(p[1]);
        return 0;
    case 0:
        {
            char *argv[4];

            close(p[0]);
            close(0);
            open("/dev/null",O_RDONLY);
            dup2(p[1],1);
            dup2(p[1],2);
            argv[0]="sh";
            argv[1]="-c";
            argv[2]=command;
            argv[3]=0;
            execve("/bin/sh",argv,environ);
            fprintf(stderr,"#ERROR: Couldn't exec `%s'\n",command);
            exit(127);
        }
    default:
        close(p[1]);
        return fdopen(p[0], "r");
    }
}
