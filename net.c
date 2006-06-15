/* $Id: net.c,v 2.4 1998/11/28 20:22:54 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: net.c - do all the net stuff                                */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include <ctype.h>

#ifdef STDC_HEADERS
#include <string.h>
#else
#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy ((s), (d), (n))
#define memmove(d, s, n) bcopy ((s), (d), (n))
#endif
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include "tintin.h"

#ifndef BADSIG
#define BADSIG (void (*)())-1
#endif

/* NOTE!  Some systems might require a #include <net/errno.h>,
 * try adding this if you are really stuck and net.c won't compile.
 * Thanks to Brian Ebersole [Harm@GrimneMUD] for this suggestion.
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#include <arpa/inet.h>
#endif


extern int do_telnet_protocol(char *data,int nb,struct session *ses);
void alarm_func(int);

extern struct session *sessionlist, *activesession, *nullsession;
extern int errno;
extern void syserr(char *msg, ...);
extern void tintin_printf(struct session *ses, char *format, ...);
extern void tintin_eprintf(struct session *ses, char *format, ...);
extern void prompt(struct session *ses);
extern void telnet_write_line(char *line, struct session *ses);
extern void pty_write_line(char *line, struct session *ses);
extern struct session* do_hook(struct session *ses, int t, char *data, int blockzap);

/**************************************************/
/* try connect to the mud specified by the args   */
/* return fd on success / 0 on failure            */
/**************************************************/
int connect_mud(char *host, char *port, struct session *ses)
{
    int sock, connectresult;
    struct sockaddr_in sockaddr;

    if (isdigit(*host))		/* interprete host part */
        sockaddr.sin_addr.s_addr = inet_addr(host);
    else
    {
        struct hostent *hp;

        if ((hp = gethostbyname(host)) == NULL)
        {
            tintin_eprintf(ses, "#ERROR - UNKNOWN HOST: {%s}", host);
            prompt(NULL);
            return 0;
        }
        memcpy((char *)&sockaddr.sin_addr, hp->h_addr, sizeof(sockaddr.sin_addr));
    }

    if (isdigit(*port))
        sockaddr.sin_port = htons(atoi(port));	/* inteprete port part */
    else
    {
        tintin_eprintf(ses, "#THE PORT SHOULD BE A NUMBER (got {%s}).", port);
        prompt(NULL);
        return 0;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        syserr("socket");

    sockaddr.sin_family = AF_INET;


    tintin_printf(ses, "#Trying to connect...");

    if (signal(SIGALRM, alarm_func) == BADSIG)
        syserr("signal SIGALRM");

    alarm(15);			/* We'll allow connect to hang in 15seconds! NO MORE! */
    connectresult = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    alarm(0);

    if (connectresult)
    {
        close(sock);
        switch (errno)
        {
        case EINTR:
            tintin_eprintf(ses, "#CONNECTION TIMED OUT.");
            break;
        case ECONNREFUSED:
            tintin_eprintf(ses, "#ERROR - Connection refused.");
            break;
        case ENETUNREACH:
            tintin_eprintf(ses, "#ERROR - Network unreachable.");
            break;
        default:
            tintin_eprintf(ses, "#Couldn't connect to %s:%s",host,port);
        }
        prompt(NULL);
        return 0;
    }
    return sock;
}

/*****************/
/* alarm handler */
/*****************/
void alarm_func(int k)
{
    /* nothing happens :) */
}

/************************************************************/
/* write line to the mud ses is connected to - add \n first */
/************************************************************/
void write_line_mud(char *line, struct session *ses)
{
    if (*line)
        ses->idle_since=time(0);
    if (ses->issocket)
        telnet_write_line(line, ses);
    else if (ses==nullsession)
        tintin_eprintf(ses, "#spurious output: %s", line);  /* CHANGE ME */
    else
        pty_write_line(line, ses);
    do_hook(ses, HOOK_SEND, line, 1);
}


/*******************************************************************/
/* read at most BUFFER_SIZE chars from mud - parse protocol stuff  */
/*******************************************************************/
int read_buffer_mud(char *buffer, struct session *ses)
{
    int i, didget, b;
    char *cpsource, *cpdest;
#define tmpbuf ses->telnet_buf
#define len ses->telnet_buflen

    if (!ses->issocket)
    {
        didget=read(ses->socket, buffer, INPUT_CHUNK);
        if (didget<=0)
            return -1;
        ses->more_coming=(didget==INPUT_CHUNK);
        buffer[didget]=0;
        return didget;
    }
    
    didget = read(ses->socket, tmpbuf+len, INPUT_CHUNK-len);

    if (didget < 0)
        return -1;

    else if (didget == 0)
        return -1;

    *(tmpbuf+len+didget)=0;
#if 0
    tintin_printf(ses,"~8~text:[%s]~-1~",tmpbuf);
#endif
    
    if ((didget+=len) == INPUT_CHUNK)
        ses->more_coming = 1;
    else
        ses->more_coming = 0;
    len=0;
    ses->ga=0;

    tmpbuf[didget]=0;
    cpsource = tmpbuf;
    cpdest = buffer;
    i = didget;
    while (i > 0)
    {
        switch(*(unsigned char *)cpsource)
        {
        case 0:
            i--;
            didget--;
            cpsource++;
            break;
        case 255:
            b=do_telnet_protocol(cpsource, i, ses);
            switch(b)
            {
            case -1:
                len=i;
                memmove(tmpbuf, cpsource, i);
                *cpdest=0;
                return didget-len;
            case -2:
            	i-=2;
            	didget-=2;
            	cpsource+=2;
            	if (!i)
            		ses->ga=1;
            	break;
            case -3:
                i -= 2;
                didget-=1;
                *cpdest++=255;
                cpsource+=2;
                break;
            default:
                i -= b;
                didget-=b;
                cpsource += b;
            }
            break;
        default:
            *cpdest++ = *cpsource++;
            i--;
        }
    }
    *cpdest = '\0';
    return didget;
}
