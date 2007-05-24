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

#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifndef HAVE_MEMCPY
# define memcpy(d, s, n) bcopy ((s), (d), (n))
# define memmove(d, s, n) bcopy ((s), (d), (n))
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
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
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
extern void convert(struct charset_conv *conv, char *outbuf, char *inbuf, int dir);
#ifdef PROFILING
extern char *prof_area;
#endif
#ifdef HAVE_LIBZ
int init_mccp(struct session *ses, int cplen, char *cpsrc);
#endif
extern void debuglog(struct session *ses, const char *format, ...);

#ifndef SOL_IP
int SOL_IP;
int SOL_TCP;
#endif

static int abort_connect;

#ifdef HAVE_GETADDRINFO
static char* afstr(int af)
{
    static char msg[19];

    switch(af)
    {
        case AF_INET:
            return "IPv4";
        case AF_INET6:
            return "IPv6";
        default:
            /* No symbolic names for AF_UNIX, AF_IPX and the like.
             * We would need separate autoconf checks just for them,
             * and they don't support TCP anyway.  We probably should
             * allow all SOCK_STREAM-capable transports, but I'm a bit
             * unsure.  */  
            snprintf(msg, 19, "AF=%d", af);
            return msg;
    }
}


/**************************************************/
/* try connect to the mud specified by the args   */
/* return fd on success / 0 on failure            */
/**************************************************/
int connect_mud(char *host, char *port, struct session *ses)
{
    int err, val;
    struct addrinfo *ai, hints, *addr;
    int sock;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    hints.ai_flags=AI_ADDRCONFIG;
    
    if ((err=getaddrinfo(host, port, &hints, &ai)))
    {
        if (err==-2)
            tintin_eprintf(ses, "#Unknown host: {%s}", host);
        else
            tintin_eprintf(ses, "#ERROR: %s", gai_strerror(err));
        return 0;
    }
    
    if (signal(SIGALRM, alarm_func) == BADSIG)
        syserr("signal SIGALRM");
    
    for (addr=ai; addr; addr=addr->ai_next)
    {
#ifdef UTF8
        tintin_printf(ses, "#Trying to connect... (%s) (charset=%s)",
            afstr(addr->ai_family), ses->charset);
#else
        tintin_printf(ses, "#Trying to connect... (%s)",
            afstr(addr->ai_family));
#endif
        
        if ((sock=socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol))==-1)
        {
            tintin_eprintf(ses, "#ERROR: %s", strerror(errno));
            continue;
        }
        
        val=IPTOS_LOWDELAY;
        if (setsockopt(sock, SOL_IP, IP_TOS, &val, sizeof(val)))
            /*tintin_eprintf(ses, "#setsockopt: %s", strerror(errno))*/;
            /* FIXME: BSD doesn't like this on IPv6 */
        
        abort_connect=0;
        alarm(15);
    intr:
        if ((connect(sock, addr->ai_addr, addr->ai_addrlen)))
        {
            switch(errno)
            {
            case EINTR:
                if (abort_connect)
                {
                    tintin_eprintf(ses, "#CONNECTION TIMED OUT");
                    continue;
                }
                else
                    goto intr;
            default:
                alarm(0);
                tintin_eprintf(ses, "#%s", strerror(errno));
                continue;
            }
        }
        
        alarm(0);
        freeaddrinfo(ai);
        return sock;
    }
    
    if (!ai)
        tintin_eprintf(ses, "#No valid addresses for {%s}", host);
    freeaddrinfo(ai);
    return 0;
}
#else
int connect_mud(char *host, char *port, struct session *ses)
{
    int sock, val;
    struct sockaddr_in sockaddr;

    if (isdigit(*host))		/* interpret host part */
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
        sockaddr.sin_port = htons(atoi(port));	/* intepret port part */
    else
    {
        tintin_eprintf(ses, "#THE PORT SHOULD BE A NUMBER (got {%s}).", port);
        prompt(NULL);
        return 0;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        syserr("socket");

    sockaddr.sin_family = AF_INET;

    val=IPTOS_LOWDELAY;
    setsockopt(sock, SOL_IP, IP_TOS, &val, sizeof(val));

    tintin_printf(ses, "#Trying to connect...");

    if (signal(SIGALRM, alarm_func) == BADSIG)
        syserr("signal SIGALRM");

    alarm(15);			/* We'll allow connect to hang in 15seconds! NO MORE! */
    val = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    alarm(0);

    if (val)
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
#endif

/*****************/
/* alarm handler */
/*****************/
void alarm_func(int k)
{
    abort_connect=1;
}

/********************************************************************/
/* write line to the mud ses is connected to - add \n or \r\n first */
/********************************************************************/
void write_line_mud(char *line, struct session *ses)
{
    char rstr[BUFFER_SIZE];

    if (*line)
        ses->idle_since=time(0);
    if (ses->issocket)
    {
        if (!ses->nagle)
        {
            setsockopt(ses->socket, SOL_TCP, TCP_NODELAY, &ses->nagle,
                sizeof(ses->nagle));
            ses->nagle=1;
        }
#ifdef UTF8
        PROFPUSH("conv: utf8->remote");
        convert(&ses->c_io, rstr, line, 1);
        PROFPOP;
        telnet_write_line(rstr, ses);
#else
        telnet_write_line(line, ses);
#endif
    }
    else if (ses==nullsession)
        tintin_eprintf(ses, "#spurious output: %s", line);  /* CHANGE ME */
    else
    {
#ifdef UTF8
        PROFPUSH("conv: utf8->remote");
        convert(&ses->c_io, rstr, line, 1);
        PROFPOP;
        pty_write_line(rstr, ses);
#else
        pty_write_line(line, ses);
#endif
    }
    do_hook(ses, HOOK_SEND, line, 1);
}

void flush_socket(struct session *ses)
{
    setsockopt(ses->socket, SOL_TCP, TCP_NODELAY, &ses->nagle,
        sizeof(ses->nagle));
    ses->nagle=0;
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

#ifdef HAVE_LIBZ    
    if (ses->mccp)
    {
        if (!ses->mccp_more)
        {
            didget = read(ses->socket, ses->mccp_buf, INPUT_CHUNK);
            if (didget<=0)
            {
                ses->mccp_more=0;
                return -1;
            }
            ses->mccp->next_in = (Bytef*)ses->mccp_buf;
            ses->mccp->avail_in = didget;
        }
        ses->mccp->next_out = (Bytef*)(tmpbuf+len);
        ses->mccp->avail_out = INPUT_CHUNK-len;
        switch(i=inflate(ses->mccp, Z_SYNC_FLUSH))
        {
        case Z_OK:
            didget=INPUT_CHUNK-len-ses->mccp->avail_out;
            break;
        case Z_STREAM_END:
            tintin_printf(ses, "#COMPRESSION END, DISABLING MCCP.");
            didget=INPUT_CHUNK-len-ses->mccp->avail_out;
            inflateEnd(ses->mccp);
            free(ses->mccp);
            ses->mccp=0;
            break;
        case Z_BUF_ERROR:
            didget=0;
            ses->mccp_more=0;
            break;
        default:
            if (ses->debuglogfile)
                debuglog(ses, "COMPRESSION ERROR: %d", i);
            tintin_eprintf(ses, "#COMPRESSION ERROR");
            ses->mccp_more=0;
            return -1;
        }
        ses->mccp_more = !ses->mccp->avail_out;
    }
    else
#endif
    {
        didget = read(ses->socket, tmpbuf+len, INPUT_CHUNK-len);
        
        if (didget < 0)
            return -1;
        
        else if (didget == 0)
            return -1;
    }

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
#ifdef HAVE_LIBZ
            case -4:
                didget-=i;
                i-=5;
                cpsource+=5;
                if (init_mccp(ses, i, cpsource)<0)
                    return -1;
                *cpdest = 0;
                return didget;
                break;
#endif
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

void init_net()
{
#ifndef SOL_IP
    struct protoent *pent;
    pent = getprotobyname ("ip");
    SOL_IP = (pent != NULL) ? pent->p_proto : 0;
    pent = getprotobyname ("tcp");
    SOL_TCP = (pent != NULL) ? pent->p_proto : 0;
#endif
}

#ifdef HAVE_LIBZ
int init_mccp(struct session *ses, int cplen, char *cpsrc)
{
    if (ses->mccp)
        return 0;

    ses->mccp = malloc(sizeof(z_stream));

    ses->mccp->data_type = Z_ASCII;
    ses->mccp->zalloc    = 0;
    ses->mccp->zfree     = 0;
    ses->mccp->opaque    = NULL;

    if (inflateInit(ses->mccp) != Z_OK)
    {
        tintin_eprintf(ses, "#FAILED TO INITIALIZE MCCP2.");
        /* Unrecoverable */
        free(ses->mccp);
        ses->mccp = NULL;
        return -1;
    }
#ifdef TELNET_DEBUG
    else
        tintin_printf(ses, "#MCCP2 INITIALIZED.");
#endif
    memcpy(ses->mccp_buf, cpsrc, cplen);
    ses->mccp->next_in = (Bytef*)ses->mccp_buf;
    ses->mccp->avail_in = cplen;
    ses->mccp_more = cplen;
    return 1;
}
#endif
