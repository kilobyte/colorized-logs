#include "tintin.h"
#include "config.h"
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

extern int colors[];
extern int yes_no(char *txt);
extern struct session *nullsession;
extern char *mystrdup(char *s);
extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern void tintin_printf(struct session *ses,char *format,...);


int mudcolors=3;    /* 0=disabled, 1=on, 2=null, 3=null+warning */
char *MUDcolors[16]={};

inline int getcolor(char **ptr,int *color,const int flag)
{
    int fg,bg,blink;
    char *txt=*ptr;

    if (*(txt++)!='~')
        return 0;
    if (flag&&(*txt=='-')&&(*(txt+1)=='1')&&(*(txt+2)=='~'))
    {
        *color=-1;
        *ptr+=3;
        return 1;
    };
    if (isdigit(*txt))
    {
        fg=strtol(txt,&txt,10);
        if (fg>255)
            return 0;
    }
    else
    if (*txt==':')
        fg=(*color==-1)? 7 : ((*color)&15);
    else
        return 0;
    if (*txt=='~')
    {
        *color=fg;
        *ptr=txt;
        return 1;
    };
    if (*txt!=':')
        return 0;
    if (isdigit(*++txt))
    {
        bg=strtol(txt,&txt,10);
        if (bg>7)
            return 0;
    }
    else
        bg=(*color==-1)? 0 : ((*color&0x70)>>4);
    if (*txt=='~')
    {
        *color=bg<<4|fg;
        *ptr=txt;
        return 1;
    };
    if (*txt!=':')
        return 0;
    if (isdigit(*++txt))
    {
        blink=strtol(txt,&txt,10);
        if (blink>1)
            return 0;
    }
    else
        blink=(*color==-1)? 0 : (*color>>7);
    if (*txt!='~')
        return 0;
    *color=blink<<7|bg<<4|fg;
    *ptr=txt;
    return 1;
}

inline int setcolor(char *txt,int c)
{
    if (c==-1)
        return sprintf(txt,"~-1~");
    if (c<16)
        return sprintf(txt,"~%d~",c);
    return sprintf(txt,"~%d:%d%s~",c&15,(c&0x70)>>4,(c>>7)? ":1" : "");
}

void do_in_MUD_colors(char *txt)
{
    static int ccolor=7;
    char OUT[BUFFER_SIZE],*out,*back,*TXT=txt;
    int colorchanged=0,oldccolor;

    for (out=OUT;*txt;txt++)
        switch(*txt)
        {
        case 12:
            out+=sprintf(out,"~112~<FormFeed>~%d~",ccolor);
        case 27:
            if (*(txt+1)=='[')
                switch(txt[strspn(txt+2,"0123456789;")+2])
                {
                case 'm':
                    {
                        back=txt++;
                        oldccolor=ccolor;
again:
                        switch (*++txt)
                        {
                        case 0:
                            goto error;
                        case ';':
                            goto again;
                        case 'm':
                            if (*(txt-1)=='[')
                                ccolor=7;   /* ESC[m */
                            out+=setcolor(out,ccolor);
                            colorchanged=1;
                            break;
                        case '0':
                            ccolor=7;
                            goto again;
                        case '1':
                            ccolor|=8;
                            goto again;
                        case '2':
                            ccolor=(ccolor&0xf0)|8;
                            goto again;
                        case '3':
                            if (!*++txt)
                                goto error;
                            if ((*txt>='0')&&(*txt<'8'))
                            {
                                ccolor=(ccolor&0xf8)|colors[*txt-'0'];
                                goto again;
                            };
                            goto error;
                        case '4':
                            if (!*++txt)
                                goto error;
                            if ((*txt>='0')&&(*txt<'8'))
                            {
                                /* background */
                                ccolor=(ccolor&0x8f)|(colors[*txt-'0']<<4);
                                goto again;
                            };
                            goto error;
                        case '5':
                            ccolor=ccolor|=128;
                            goto again;
                        case '7':
                            ccolor=(ccolor&0x88)|(ccolor&0x70>>4)|(ccolor&7);
                            /* inverse should propagate... oh well */
                            goto again;
                        default:
error:
                            txt=back;
                        };
                    };
                    break;
                case 'C':
                    back=txt;
                    oldccolor=0;
                    txt+=2;
                    while (isdigit(*txt))
                        oldccolor=oldccolor*10+*txt++-'0';
                    if (oldccolor>132)
                        oldccolor=0; /* something is wrong? */
                    if (*txt=='C')
                        while (oldccolor--)
                            *out++=' ';
                    else
                        txt=back;
                    break;
                case 'D': /* this interpretation is not really valid... */
                case 'K':
                    txt+=2;
                    while (!isalpha(*txt++));
                    txt--;
                    out=OUT;
                    if (colorchanged)
                        out+=setcolor(out,ccolor);
                };
                break;
        case '~':
            back=txt;
            if (getcolor(&txt,&oldccolor,1))
            {
                *out++='`';
                back++;
                while (back<txt)
                    *out++=*back++;
                *out++='`';
                break;
            };
        default:
            *out++=*txt;
        };
    *out=0;
    strcpy(TXT,OUT);
}



void do_out_MUD_colors(char *line)
{
    char buf[BUFFER_SIZE];
    char *txt=buf,*pos=line;
    int c=7;

    if (!mudcolors)
        return;
    for (;*pos;pos++)
    {
        if (*pos=='~')
            if (getcolor(&pos,&c,0))
                goto color;
        *txt++=*pos;
        continue;
color:
        switch(mudcolors)
        {
        case 3:
            tintin_printf(0,"#Warning: no color codes set, use #mudcolors");
            mudcolors=2;
        case 2:
            break;
        case 1:
            strcpy(txt,MUDcolors[c&15]);
            txt+=strlen(MUDcolors[c&15]);
        }
    };
    *txt=0;
    strcpy(line,buf);
}

/**************************/
/* the #mudcolors command */
/**************************/
void mudcolors_command(char *arg,struct session *ses)
{
    char cc[BUFFER_SIZE][16],buf[BUFFER_SIZE];
    int nc;

    if (!*arg)
    {
error_msg:
        tintin_printf(ses,"#Syntax: #mudcolors OFF, #mudcolors {} or #mudcolors {c0} {c1} ... {c15}");
        return;
    }
    if (!yes_no(arg))
    {
        mudcolors=0;
        tintin_printf(ses,"#outgoing color codes (~n~) are now sent verbatim.");
        return;
    }
    if (!*get_arg_in_braces(arg,buf,0))
    {
        arg=buf;
        if (!*arg)
            goto null_codes;
    }
    /* we allow BOTH {a} {b} {c} _and_ {{a} {b} {c}} - inconstency, but it's ok */
    for (nc=0;nc<16;nc++)
    {
        if (!*arg)
        {
            if ((nc==1)&&!*cc[0])
            {
null_codes:
                mudcolors=2;
                tintin_printf(ses,"#outgoing color codes are now ignored.");
                return;
            }
            else
                goto error_msg;
        }
        arg=get_arg_in_braces(arg,cc[nc],0);
    };
    if (*arg)
        goto error_msg;
    mudcolors=1;
    for (nc=0;nc<16;nc++)
    {
        free(MUDcolors[nc]);
        MUDcolors[nc]=mystrdup(cc[nc]);
    };
    tintin_printf(ses,"#outgoing color codes table initialized");
}
