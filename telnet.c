/* Do all the telnet protocol stuff */

#include <unistd.h>
#include "config.h"
#include "tintin.h"

extern void tintin_printf(struct session *ses, char *format, ...);
extern int LINES,COLS,isstatus;
extern struct session *sessionlist;

#define SE  240     /* subnegotiation end */
#define NOP 241     /* no operation */
#define DM  242     /* Data Mark */
#define BRK 243     /* Break */
#define IP  244     /* interrupt process */
#define AO  245     /* abort output */
#define AYT 246     /* Are you there? */
#define EC  247     /* erase character */
#define EL  248     /* erase line */
#define GA  249     /* go ahead */
#define SB  250     /* subnegotiations */
#define WILL    251
#define WONT    252
#define DO      253
#define DONT    254
#define IAC 255     /* interpret as command */

#define ECHO                1
#define SUPPRESS_GO_AHEAD   3
#define STATUS              5
#define TERMINAL_TYPE       24
#define NAWS                31

#define IS      0
#define SEND    1

#ifdef TELNET_DEBUG
char *will_names[4]={"WILL", "WONT", "DO", "DONT"};
char *option_names[]=
    {
        "Binary Transmission",
        "Echo",
        "Reconnection",
        "Suppress Go Ahead",
        "Approx Message Size Negotiation",
        "Status",
        "Timing Mark",
        "Remote Controlled Trans and Echo",
        "Output Line Width",
        "Output Page Size",
        "Output Carriage-Return Disposition",
        "Output Horizontal Tab Stops",
        "Output Horizontal Tab Disposition",
        "Output Formfeed Disposition",
        "Output Vertical Tabstops",
        "Output Vertical Tab Disposition",
        "Output Linefeed Disposition",
        "Extended ASCII",
        "Logout",
        "Byte Macro",
        "Data Entry Terminal",
        "SUPDUP",
        "SUPDUP Output",
        "Send Location",
        "Terminal Type",
        "End of Record",
        "TACACS User Identification",
        "Output Marking",
        "Terminal Location Number",
        "Telnet 3270 Regime",
        "X.3 PAD",
        "Negotiate About Window Size",
        "Terminal Speed",
        "Remote Flow Control",
        "Linemode",
        "X Display Location",
        "Environment Option",
        "Authentication Option",
        "Encryption Option",
        "New Environment Option",
    };
#endif

void telnet_send_naws(struct session *ses)
{
    unsigned char nego[128],*np;

#define PUTBYTE(b)    if ((b)==255) *np++=255;   *np++=(b);
    if (!ses->issocket || !ses->naws)
        return;
    np=nego;
    *np++=IAC;
    *np++=SB;
    *np++=NAWS;
    PUTBYTE(COLS/256);
    PUTBYTE(COLS%256);
    PUTBYTE((LINES-1-!!isstatus)/256);
    PUTBYTE((LINES-1-!!isstatus)%256);
    *np++=IAC;
    *np++=SE;
    write(ses->socket, nego, np-nego);
#ifdef TELNET_DEBUG
    {
        char buf[BUFFER_SIZE],*b=buf;
        int neb=np-nego-2;
        np=nego+3;
        b=buf+sprintf(buf, "IAC SB NAWS ");
        while (np-nego<neb)
            b+=sprintf(b, "<%u> ", *np++);
        b+=sprintf(b, "IAC SE");
        tintin_printf(ses, "~8~[telnet] sent: %s~-1~", buf);
    }
#endif
}

void telnet_resize_all(void)
{
    struct session *sp;

    for (sp=sessionlist; sp; sp=sp->next)
        telnet_send_naws(sp);
}

int do_telnet_protecol(unsigned char *data,int nb,struct session *ses)
{
    unsigned char *cp = data+1;
    unsigned char wt;
    unsigned char answer[3];
    unsigned char nego[128],*np;
    unsigned int neb;

#define NEXTCH  cp++;               \
                if (cp-data>=nb)    \
                    return -1;

    if (nb<2)
        return -1;
    switch(*cp)
    {
    case WILL:
    case WONT:
    case DO:
    case DONT:
        if (nb<3)
            return -1;
        wt=*(cp++);
#ifdef TELNET_DEBUG
        tintin_printf(ses, "~8~[telnet] received: IAC %s <%u> (%s)~-1~",
                      will_names[wt-251], *cp, option_names[*cp]);
#endif
        answer[0]=IAC;
        answer[2]=*cp;
        switch(*cp)
        {
        case ECHO:
            switch(wt)
            {
            case WILL:  answer[1]=DONT; ses->server_echo=1; break;
            case DO:    answer[1]=WILL; break;
            case WONT:  answer[1]=DONT; ses->server_echo=2; break;
            case DONT:  answer[1]=WONT; break;
            };
            break;
        case TERMINAL_TYPE:
            switch(wt)
            {
            case WILL:  answer[1]=DONT; break;
            case DO:    answer[1]=WILL; break;
            case WONT:  answer[1]=DONT; break;
            case DONT:  answer[1]=WONT; break;
            };
            break;
        case NAWS:
            switch(wt)
            {
            case WILL:  answer[1]=DO;   ses->naws=1; break;
            case DO:    answer[1]=WILL; ses->naws=1; break;
            case WONT:  answer[1]=DONT; ses->naws=0; break;
            case DONT:  answer[1]=WONT; ses->naws=0; break;
            };
            break;
        default:
            switch(wt)
            {
            case WILL:  answer[1]=DONT; break;
            case DO:    answer[1]=WONT; break;
            case WONT:  answer[1]=DONT; break;
            case DONT:  answer[1]=WONT; break;
            };
        }
        write(ses->socket, answer, 3);
#ifdef TELNET_DEBUG
        tintin_printf(ses, "~8~[telnet] sent: IAC %s <%u> (%s)~-1~",
                      will_names[answer[1]-251], *cp, option_names[*cp]);
#endif
        if (*cp==NAWS)
            telnet_send_naws(ses);
        return 3;
    case SB:
        np=nego;
sbloop:
        NEXTCH;
        while (*cp!=IAC)
        {
            *np++=*cp;
            NEXTCH;
        }
        NEXTCH;
        if (*cp==IAC)
        {
            *np++=IAC;
            goto sbloop;
        }
        if (*cp!=SE)
        {
            *np++=IAC;
            *np++=*cp;
            goto sbloop;
        }
        nb=cp-data;
        neb=np-nego;
#ifdef TELNET_DEBUG
        {
            char buf[BUFFER_SIZE],*b=buf;
            np=nego;
            b=buf+sprintf(buf, "IAC SB ");
            switch(*np)
            {
            case TERMINAL_TYPE:
                b+=sprintf(b, "TERMINAL-TYPE ");
                if (*++np==SEND)
                {
                    b+=sprintf(b, "SEND ");
                    ++np;
                }
                break;
            }
            while (np-nego<neb)
                b+=sprintf(b, "<%u> ", *np++);
            b+=sprintf(b, "IAC SE");
            tintin_printf(ses, "~8~[telnet] received: %s~-1~", buf);
        }
#endif
        switch(*(np=nego))
        {
        case TERMINAL_TYPE:
            if (*(np+1)==SEND)
            {
                write(ses->socket, nego,
                      sprintf(nego, "%c%c%c%c%s%c%c", IAC, SB,
                              TERMINAL_TYPE, IS, TERM, IAC, SE));
#ifdef TELNET_DEBUG
                tintin_printf(ses, "~8~[telnet] sent: IAC SB TERMINAL-TYPE IS \"%s\" IAC SE~-1~", TERM);
#endif
            }
            break;
        default:
        }
        return nb+4;
    case GA:
#ifdef TELNET_DEBUG
        tintin_printf(ses, "~8~[telnet] received: IAC GA~-1~");
#endif
        ses->gas=1;
        return -2;
    case IAC:       /* IAC IAC is the escape for literal 255 byte */
        return 2;	    /* ... but we ignore it */  /* FIXME */
    default:
        /* other 2-byte command, ignore */
#ifdef TELNET_DEBUG
        tintin_printf(ses, "~8~[telnet] received: IAC <%u>~-1~", *cp&255);
#endif
        return 2;
    }
    return (cp-data);
}
