#define _XOPEN_SOURCE
#include "config.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
#if GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif
#ifdef HAVE_GRANTPT
# ifdef HAVE_STROPTS_H
#  include <stropts.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <stdlib.h>
#include <signal.h>
#include "tintin.h"
#include "ui.h"

extern char **environ;
extern void syserr(char *msg, ...);


#ifndef HAVE_FORKPTY
# if !(defined(HAVE__GETPTY) || defined(HAVE_GRANTPT) && (defined(HAVE_GETPT) || defined(HAVE_DEV_PTMX)))
/*
 * if no PTYRANGE[01] is in the config file, we pick a default
 */
#  ifndef PTYRANGE0
#   define PTYRANGE0 "qpr"
#  endif
#  ifndef PTYRANGE1
#   define PTYRANGE1 "0123456789abcdef"
#  endif
#  ifdef M_UNIX
static char PtyProto[] = "/dev/ptypXY";
static char TtyProto[] = "/dev/ttypXY";
#  else
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
#  endif
# endif


#ifdef TERM_DEBUG
void print_stty(int fd)
{
    struct termios ta;
    struct winsize ws;
    char buf[BUFFER_SIZE],*bptr;
#define battr(c,a,b) bptr+=sprintf(bptr," %s%s~7~",(ta.c_##c&a)?"~9~":"~2~-~4~",b,a);

    memset(&ta,0,sizeof(ta));
    memset(&ws,0,sizeof(ws));
    tintin_printf(0, "~7~pty attributes (fd=%d):",fd);
    if (tcgetattr(fd, &ta))
        tintin_printf(0," attrs: unknown");
    else
    {
        tintin_printf(0," attrs: cflag=~3~%x~7~, iflag=~3~%x~7~, oflag=~3~%x~7~, lflag=~3~%x~7~",
            ta.c_cflag, ta.c_iflag, ta.c_oflag, ta.c_lflag);
        bptr=buf+sprintf(buf," ~3~[%x]~7~:",ta.c_cflag);
        battr(cflag,PARENB,"parenb");
        battr(cflag,PARODD,"parodd");
        battr(cflag,CS8,"cs8");
        battr(cflag,HUPCL,"hupcl");
        battr(cflag,CSTOPB,"cstopb");
        battr(cflag,CREAD,"cread");
        battr(cflag,CLOCAL,"clocal");
        battr(cflag,CRTSCTS,"crtscts");
        tintin_printf(0,"%s",buf);
        bptr=buf+sprintf(buf," ~3~[%x]~7~:",ta.c_iflag);
        battr(iflag,IGNBRK,"ignbrk");
        battr(iflag,BRKINT,"brkint");
        battr(iflag,IGNPAR,"ignpar");
        battr(iflag,PARMRK,"parmrk");
        battr(iflag,INPCK,"inpck");
        battr(iflag,ISTRIP,"istrip");
        battr(iflag,INLCR,"inlcr");
        battr(iflag,IGNCR,"igncr");
        battr(iflag,ICRNL,"icrnl");
        battr(iflag,IXON,"ixon");
        battr(iflag,IXOFF,"ixoff");
        battr(iflag,IUCLC,"iuclc");
        battr(iflag,IXANY,"ixany");
        battr(iflag,IMAXBEL,"imaxbel");
        tintin_printf(0,"%s",buf);
        bptr=buf+sprintf(buf," ~3~[%x]~7~:",ta.c_oflag);
        battr(oflag,OPOST,"opost");
        battr(oflag,OLCUC,"olcuc");
        battr(oflag,OCRNL,"ocrnl");
        battr(oflag,ONLCR,"onlcr");
        battr(oflag,ONOCR,"onocr");
        battr(oflag,ONLRET,"onlret");
        battr(oflag,OFILL,"ofill");
        battr(oflag,OFDEL,"ofdel");
/*
        battr(oflag,NL0,"nl0");
        battr(oflag,CR0,"cr0");
        battr(oflag,TAB0,"tab0");
        battr(oflag,BS0,"bs0");
        battr(oflag,VT0,"vt0");
        battr(oflag,FF0,"ff0");
*/
        tintin_printf(0,"%s",buf);
        bptr=buf+sprintf(buf," ~3~[%x]~7~:",ta.c_lflag);
        battr(lflag,ISIG,"isig");
        battr(lflag,ICANON,"icanon");
        battr(lflag,IEXTEN,"iexten");
        battr(lflag,ECHO,"echo");
        battr(lflag,ECHOE,"echoe");
        battr(lflag,ECHOK,"echok");
        battr(lflag,ECHONL,"echonl");
        battr(lflag,NOFLSH,"noflsh");
        battr(lflag,XCASE,"xcase");
        battr(lflag,TOSTOP,"tostop");
        battr(lflag,ECHOPRT,"echoprt");
        battr(lflag,ECHOCTL,"echoctl");
        battr(lflag,ECHOKE,"echoke");
        tintin_printf(0,"%s",buf);
    }
    if (ioctl(fd,TIOCGWINSZ,&ws))
        tintin_printf(0," window size: unknown");
    else
        tintin_printf(0," window size: %dx%d",
            ws.ws_col,ws.ws_row);
}

void termdebug_command(char *arg, struct session *ses)
{
    print_stty(ses->socket);
}
#endif


int forkpty(int *amaster,char *dummy,struct termios *termp, struct winsize *wp)
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
    }
    master=filedes[0];
    slave=filedes[1];
#else
#if defined(HAVE_GRANTPT) && (defined(HAVE_GETPT) || defined(HAVE_DEV_PTMX))
# ifdef HAVE_PTSNAME
    char *name;
# else
    char name[80];
# endif

# ifdef HAVE_GETPT
    master=getpt();
# else
    master=open("/dev/ptmx", O_RDWR);
# endif

    if (master<0)
        return -1;

    if (grantpt(master)<0||unlockpt(master)<0)
        goto close_master;

# ifdef HAVE_PTSNAME
    if (!(name=ptsname(master)))
        goto close_master;
# else
    if (ptsname_r(master,name,80))
        goto close_master;
# endif

    slave=open(name,O_RDWR);
    if (slave==-1)
        goto close_master;

# ifdef HAVE_STROPTS_H
    if (isastream(slave))
        if (ioctl(slave, I_PUSH, "ptem")<0
                ||ioctl(slave, I_PUSH, "ldterm")<0)
            goto close_slave;
# endif

    goto ok;

close_slave:
    close (slave);

close_master:
    close (master);
    return -1;

ok:
#else
  char *p, *q, *l, *d;
  char PtyName[32], TtyName[32];

  strcpy(PtyName, PtyProto);
  strcpy(TtyName, TtyProto);
  for (p = PtyName; *p != 'X'; p++)
    ;
  for (q = TtyName; *q != 'X'; q++)
    ;
  for (l = PTYRANGE0; (*p = *l) != '\0'; l++)
    {
      for (d = PTYRANGE1; (p[1] = *d) != '\0'; d++)
        {
/*        tintin_printf(0,"OpenPTY tries '%s'", PtyName);*/
          if ((master = open(PtyName, O_RDWR | O_NOCTTY)) == -1)
            continue;
          q[0] = *l;
          q[1] = *d;
          if (access(TtyName, R_OK | W_OK))
            {
              close(master);
              continue;
            }
          if((slave=open(TtyName, O_RDWR|O_NOCTTY))==-1)
	  {
	  	close(master);
	  	continue;
	  }
          goto ok;
        }
    }
  return -1;
  ok:
#endif
#endif

    if (termp)
        tcsetattr(master, TCSANOW, termp);
    if (wp)
        ioctl(master,TIOCSWINSZ,wp);
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
#endif

void pty_resize(int fd,int sx,int sy)
{
    struct winsize ws;
    
    if (LINES>1 && COLS>0)
    {
        ws.ws_row=LINES-1;
        ws.ws_col=COLS;
        ws.ws_xpixel=0;
        ws.ws_ypixel=0;
        ioctl(fd,TIOCSWINSZ,&ws);
    }
}

inline void pty_makeraw(struct termios *ta)
{
    memset(ta, 0, sizeof(*ta));
    ta->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                    |INLCR|IGNCR|ICRNL|IXON);
    ta->c_oflag &= ~OPOST;
    ta->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    ta->c_cflag &= ~(CSIZE|PARENB);
    ta->c_cflag |= CS8;

    ta->c_cc[VMIN]=1;
    ta->c_cc[VTIME]=0;
}

int run(char *command)
{
    int fd;

#ifndef PTY_ECHO_HACK
    struct termios ta;
    struct winsize ws;

    pty_makeraw(&ta);

    ws.ws_row=LINES-1;
    ws.ws_col=COLS;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;
    switch(forkpty(&fd,0,&ta,(LINES>1 && COLS>0)?&ws:0))
#else
    switch(forkpty(&fd,0,0,0))
#endif
    {
    case -1:
        return(0);
    case 0:
        {
            char *argv[4];
            char cmd[BUFFER_SIZE+5];
            
            sprintf(cmd, "exec %s", command);
            argv[0]="sh";
            argv[1]="-c";
            argv[2]=cmd;
            argv[3]=0;
            putenv("TERM=" TERM); /* TERM=KBtin.  Or should we lie? */
            execve("/bin/sh",argv,environ);
            fprintf(stderr,"#ERROR: Couldn't exec `%s'\n",command);
            exit(127);
        }
    default:
        return(fd);
    }
}


FILE* mypopen(char *command, int wr)
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
            char *argv[4], cmd[BUFFER_SIZE+5];

            if(!wr)
            {
                close(p[0]);
                close(0);
                open("/dev/null",O_RDONLY);
                dup2(p[1],1);
                dup2(p[1],2);
                close(p[1]);
            }
            else
            {
                close(p[1]);
                close(1);
                close(2);
                open("/dev/null",O_WRONLY);
                dup2(1,2);
                dup2(p[0],0);
                close(p[0]);
                signal(SIGINT, SIG_IGN);
                signal(SIGHUP, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
            }
            sprintf(cmd, "exec %s", command);
            argv[0]="sh";
            argv[1]="-c";
            argv[2]=cmd;
            argv[3]=0;
            execve("/bin/sh",argv,environ);
            fprintf(stderr,"#ERROR: Couldn't exec `%s'\n",command);
            exit(127);
        }
    default:
        close(p[!wr]);
        return fdopen(p[wr], wr?"w":"r");
    }
}

void pty_write_line(char *line, struct session *ses)
{
    char out[4*BUFFER_SIZE+1];
    int len;
#ifdef PTY_ECHO_HACK
    struct termios ta, oldta;

    tcgetattr(ses->socket, &oldta);
    memcpy(&ta, &oldta, sizeof(ta));
    pty_makeraw(&ta);
    ta.c_cc[VMIN]=0x7fffffff;
    tcsetattr(ses->socket, TCSANOW, &ta);
#else
# ifdef RESET_RAW
    struct termios ta;
    
    memset(&ta, 0, sizeof(ta));
    pty_makeraw(&ta);
    tcsetattr(ses->socket, TCSANOW, &ta);
# endif
#endif
    
    len=sprintf(out, "%s\n", line);
    if (write(ses->socket, out, len) == -1)
        syserr("write in pty_write_line()");
    
#ifdef PTY_ECHO_HACK
    /* FIXME: if write() blocks, they'll act in raw mode */
    tcsetattr(ses->socket, TCSANOW, &oldta);
    sleep(0);   /* let'em act */
#endif
}
