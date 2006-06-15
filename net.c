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


void do_telnet_protecol();
void alarm_func(int);

extern int sessionsstarted;
extern struct listnode *common_aliases, *common_actions, *common_subs;
extern struct session *sessionlist, *activesession;
extern int errno;

/**************************************************/
/* try connect to the mud specified by the args   */
/* return fd on success / 0 on failure            */
/**************************************************/
int connect_mud(host, port, ses)
     char *host;
     char *port;
     struct session *ses;
{
  int sock, connectresult;
  struct sockaddr_in sockaddr;

  if (isdigit(*host))		/* interprete host part */
    sockaddr.sin_addr.s_addr = inet_addr(host);
  else {
    struct hostent *hp;

    if ((hp = gethostbyname(host)) == NULL) {
      tintin_puts("#ERROR - UNKNOWN HOST.", ses);
      prompt(NULL);
      return 0;
    }
    memcpy((char *)&sockaddr.sin_addr, hp->h_addr, sizeof(sockaddr.sin_addr));
  }

  if (isdigit(*port))
    sockaddr.sin_port = htons(atoi(port));	/* inteprete port part */
  else {
    tintin_puts("#THE PORT SHOULD BE A NUMBER.", ses);
    prompt(NULL);
    return 0;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    syserr("socket");

  sockaddr.sin_family = AF_INET;


  tintin_puts("#Trying to connect..", ses);

  if (signal(SIGALRM, alarm_func) == BADSIG)
    syserr("signal SIGALRM");

  alarm(15);			/* We'll allow connect to hang in 15seconds! NO MORE! */
  connectresult = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
  alarm(0);

  if (connectresult) {
    close(sock);
    switch (errno) {
    case EINTR:
      tintin_puts("#CONNECTION TIMED OUT.", ses);
      break;
    case ECONNREFUSED:
      tintin_puts("#ERROR - CONNECTION REFUSED.", ses);
      break;
    case ENETUNREACH:
      tintin_puts("#ERROR - THE NETWORK IS NOT REACHABLE FROM THIS HOST.", ses);
      break;
    default:
      tintin_puts("#Couldn't connect", ses);
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
void write_line_mud(line, ses)
     char *line;
     struct session *ses;
{
  char outtext[BUFFER_SIZE + 2];

  strcpy(outtext, line);
  strcat(outtext, "\n");

  if (write(ses->socket, outtext, strlen(outtext)) == -1)
    syserr("write in write_to_mud");
}


/*******************************************************************/
/* read at most BUFFER_SIZE chars from mud - parse protocol stuff  */
/*******************************************************************/
int read_buffer_mud(buffer, ses)
     char *buffer;
     struct session *ses;
{
  int i, didget;
  char tmpbuf[BUFFER_SIZE], *cpsource, *cpdest;

  didget = read(ses->socket, tmpbuf, 512);
  ses->old_more_coming=ses->more_coming;
  if (didget == 512)
    ses->more_coming = 1;
  else
    ses->more_coming = 0;
  if (didget < 0)
    return 0;			/*syserr("read from socket");  we do this here instead - dunno quite 
				   why, but i got some mysterious connection read by peer on some hps */

  else if (didget == 0)
    return 0;

  else {
    tmpbuf[didget]=0;
    cpsource = tmpbuf;
    cpdest = buffer;
    i = didget;
    while (i > 0) {
      if (*(unsigned char *)cpsource == 255) {
	do_telnet_protecol(*cpsource, *(cpsource + 1), *(cpsource + 2), ses);
	i -= 3;
	cpsource += 3;
      } else {
	*cpdest++ = *cpsource++;
	i--;
      }
    }
  }
  *cpdest = '\0';
  return didget;
}


/*****************************************************************/
/* respond according to the telnet protecol - weeeeelllllll..... */
/*****************************************************************/
void do_telnet_protecol(dat0, dat1, dat2, ses)
     int dat0;
     int dat1;
     int dat2;
     struct session *ses;
{
/* we don't do anything here.. why should we? add the stuff yourself if
   you feel like being nice..... */
}
