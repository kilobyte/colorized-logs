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


extern int do_telnet_protecol(char *data,int nb,struct session *ses);
void alarm_func(int);

extern struct session *sessionlist, *activesession;
extern int errno;
extern void syserr(char *msg);
extern void tintin_printf(struct session *ses, char *format, ...);
extern void prompt(struct session *ses);

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
            tintin_printf(ses, "#ERROR - UNKNOWN HOST: {%s}", host);
            prompt(NULL);
            return 0;
        }
        memcpy((char *)&sockaddr.sin_addr, hp->h_addr, sizeof(sockaddr.sin_addr));
    }

    if (isdigit(*port))
        sockaddr.sin_port = htons(atoi(port));	/* inteprete port part */
    else
    {
        tintin_printf(ses, "#THE PORT SHOULD BE A NUMBER (got {%s}).", port);
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
            tintin_printf(ses, "#CONNECTION TIMED OUT.");
            break;
        case ECONNREFUSED:
            tintin_printf(ses, "#ERROR - Connection refused.");
            break;
        case ENETUNREACH:
            tintin_printf(ses, "#ERROR - Network unreachable.");
            break;
        default:
            tintin_printf(ses, "#Couldn't connect to %s:%s",host,port);
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
    char outtext[BUFFER_SIZE + 2];

    strcpy(outtext, line);
    if (ses->issocket)
        strcat(outtext, "\r\n");
    else
        strcat(outtext, "\n");

    if (write(ses->socket, outtext, strlen(outtext)) == -1)
        syserr("write in write_to_mud");
    ses->idle_since=time(0);
}


/*******************************************************************/
/* read at most BUFFER_SIZE chars from mud - parse protocol stuff  */
/*******************************************************************/
int read_buffer_mud(char *buffer, struct session *ses)
{
    int i, didget, b;
    char tmpbuf[BUFFER_SIZE], *cpsource, *cpdest;


    if (!ses->issocket)
    {
        didget=read(ses->socket, buffer, 512);
        if (didget<=0)
            return -1;
        ses->more_coming=(didget==512);
        buffer[didget]=0;
        return didget;
    }
    
    didget = read(ses->socket, tmpbuf+ses->telnet_buf, 512-ses->telnet_buf);

    if (didget < 0)
        return -1;

    else if (didget == 0)
        return -1;
    
    if ((didget+=ses->telnet_buf) == 512)
        ses->more_coming = 1;
    else
        ses->more_coming = 0;
    ses->telnet_buf=0;
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
            b=do_telnet_protecol(cpsource, i, ses);
            if (b==-1)
            {
                ses->telnet_buf=i;
                memmove(tmpbuf, cpsource, i);
                *cpdest=0;
                return didget-ses->telnet_buf;
            }
            if (b==-2)
            {
            	i-=2;
            	didget-=2;
            	cpsource+=2;
/*            	*cpdest++=164;*/
            	if (!i)
            		ses->ga=1;
            	break;
            }
            i -= b;
            didget-=b;
            cpsource += b;
            break;
        default:
            *cpdest++ = *cpsource++;
            i--;
        }
    }
    *cpdest = '\0';
    return didget;
}
