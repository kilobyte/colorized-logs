#include "config.h"
#include "tintin.h"
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

extern int yes_no(char *txt);
extern struct session *nullsession;
extern char *mystrdup(char *s);
extern char *get_arg_in_braces(char *s,char *arg,int flag);
extern void tintin_printf(struct session *ses,char *format,...);
extern void tintin_eprintf(struct session *ses,char *format,...);

const int colors[8]={0,4,2,6,1,5,3,7};
int mudcolors=3;    /* 0=disabled, 1=on, 2=null, 3=null+warning */
char *MUDcolors[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int getcolor(char **ptr,int *color,const int flag)
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
        if (fg>1023)
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
        if (blink>7)
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

int setcolor(char *txt,int c)
{
    if (c==-1)
        return sprintf(txt,"~-1~");
    if (c<16)
        return sprintf(txt,"~%d~",c);
    if (c<128)
        return sprintf(txt,"~%d:%d~",c&15,(c&0x70)>>4);
    return sprintf(txt,"~%d:%d:%d~",c&15,(c&0x70)>>4,c>>7);
}

#define MAXTOK 10

void do_in_MUD_colors(char *txt,int quotetype)
{
    static int ccolor=7;
    /* worst case: buffer full of FormFeeds, with color=1023 */
    char OUT[BUFFER_SIZE*20],*out,*back,*TXT=txt;
    int tok[MAXTOK],nt,i;

    for (out=OUT;*txt;txt++)
        switch(*txt)
        {
        case 12:
            out+=sprintf(out,"~112~<FormFeed>~%d~",ccolor);
        case 27:
            if (*(txt+1)=='[')
            {
                back=txt++;
                tok[0]=nt=0;
again:
                switch (*++txt)
                {
                case 0:
                    goto error;
                case ';':
                    if (++nt==MAXTOK)
                        goto error;
                    tok[nt]=0;
                    goto again;
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    tok[nt]=tok[nt]*10+*txt-'0';
                    goto again;
                case 'm':
                    if (*(txt-1)!='[')
                        nt++;
                    else
                        ccolor=7;
                    for(i=0;i<nt;i++)
                        switch(tok[i])
                        {
                        case 0:
                            ccolor=7;
                            break;
                        case 1:
                            ccolor|=8;
                            break;
                        case 2:
                            ccolor=(ccolor&~0x0f)|8;
                            break;
                        case 3:
                            ccolor|=256;
                            break;
                        case 4:
                            ccolor|=512;
                            break;
                        case 5:
                            ccolor|=128;
                            break;
                        case 7:
                            ccolor=(ccolor&0x388)|(ccolor&0x70>>4)|(ccolor&7);
                            /* inverse should propagate... oh well */
                            break;
                        case 21:
                            ccolor&=~8;
                            break;
                        case 22:
                            ccolor&=~8;
                            if (!(ccolor&15))
                                ccolor|=7;
                            break;
                        case 23:
                            ccolor&=~256;
                            break;
                        case 24:
                            ccolor&=~512;
                            break;
                        case 25:
                            ccolor&=~128;
                            break;
                        case 39:
                            ccolor&=~0xf;
                            ccolor|=7;
                            break;
                        case 49:
                            ccolor&=~0x70;
                            break;
                        default:
                            if (tok[i]>=30 && tok[i]<38)
                                ccolor=(ccolor&0x3f8)|colors[tok[i]-30];
                            else if (tok[i]>=40 && tok[i]<48)
                                ccolor=(ccolor&0x38f)|(colors[tok[i]-40]<<4);
                            /* ignore unknown attributes */
                        }
                        out+=setcolor(out,ccolor);
                    break;
                case 'C':
                    if (tok[0]<0)     /* sanity check */
                        break;
                    if (out-OUT+tok[0]>INPUT_CHUNK*2)
                        break;       /* something fishy is going on */
                    for(i=0;i<tok[0];i++)
                        *out++=' ';
                    break;
                case 'D': /* this interpretation is badly invalid... */
                case 'K':
                case 'J':
                    out=OUT;
                    out+=setcolor(out,ccolor);
                    break;
                default:
error:
                    txt=back;
                }
            }
            break;
        case '~':
            back=txt;
            if (getcolor(&txt,&i,1))
            {
                if (quotetype)
                {
                    *out++='~';
                    *out++='~';
                    *out++=':';
                    *out++='~';
                }
                else
                    *out++='`';
                back++;
                while (back<txt)
                    *out++=*back++;
                *out++=quotetype?'~':'`';
                break;
            };
        default:
            *out++=*txt;
        };
    if (out-OUT>=BUFFER_SIZE) /* can happen only if there's a lot of FFs */
        out=OUT+BUFFER_SIZE-1;
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
        tintin_eprintf(ses,"#ERROR: valid syntax is: #mudcolors OFF, #mudcolors {} or #mudcolors {c0} {c1} ... {c15}");
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
