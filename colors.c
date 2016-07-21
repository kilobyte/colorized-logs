#include "tintin.h"
#include "protos/print.h"
#include "protos/misc.h"
#include "protos/parse.h"
#include "protos/utils.h"
#include "protos/hooks.h"

extern struct session *nullsession;

const int colors[8]={0,4,2,6,1,5,3,7};
static int mudcolors=3;    /* 0=disabled, 1=on, 2=null, 3=null+warning */
static char *MUDcolors[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

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
        if (fg>0x7ff)
            return 0;
    }
    else
    if (*txt==':')
        fg=(*color==-1)? 7 : ((*color)&0xf);
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
        if (blink>15)
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
    if (c<0x80)
        return sprintf(txt,"~%d:%d~",c&0xf,(c&0x70)>>4);
    return sprintf(txt,"~%d:%d:%d~",c&0xf,(c&0x70)>>4,c>>7);
}

typedef unsigned char u8;
struct rgb { u8 r; u8 g; u8 b; };

static struct rgb rgb_from_256(int i)
{
   struct rgb c;
   if (i < 8)
   {   /* Standard colours. */
       c.r = i&1 ? 0xaa : 0x00;
       c.g = i&2 ? 0xaa : 0x00;
       c.b = i&4 ? 0xaa : 0x00;
   }
   else if (i < 16)
   {
       c.r = i&1 ? 0xff : 0x55;
       c.g = i&2 ? 0xff : 0x55;
       c.b = i&4 ? 0xff : 0x55;
   }
   else if (i < 232)
   {   /* 6x6x6 colour cube. */
       c.r = (i - 16) / 36 * 85 / 2;
       c.g = (i - 16) / 6 % 6 * 85 / 2;
       c.b = (i - 16) % 6 * 85 / 2;
   }
   else/* Grayscale ramp. */
      c.r = c.g = c.b = i * 10 - 2312;
   return c;
}

static int rgb_foreground(struct rgb c)
{
   u8 fg, max = c.r;
   if (c.g > max)
       max = c.g;
   if (c.b > max)
       max = c.b;
   fg = (c.r > max/2 ? 4 : 0)
      | (c.g > max/2 ? 2 : 0)
      | (c.b > max/2 ? 1 : 0);
   if (fg == 7 && max <= 0x55)
       return 8;
   else if (max > 0xaa)
       return fg+8;
   else
       return fg;
}

static int rgb_background(struct rgb c)
{
    /* For backgrounds, err on the dark side. */
    return ((c.r&0x80) >> 5 | (c.g&0x80) >> 6 | (c.b&0x80) >> 7) << 4;
}

#define MAXTOK 10

void do_in_MUD_colors(char *txt,int quotetype,struct session *ses)
{
    static int ccolor=7;
    /* worst case: buffer full of FormFeeds, with color=1023 */
    /* TODO: not anymore, it's much shorter now */
    char OUT[BUFFER_SIZE*20],*out,*back,*TXT=txt;
    int tok[MAXTOK],nt,i;

    for (out=OUT;*txt;txt++)
        switch (*txt)
        {
        case 27:
            if (*(txt+1)=='[')
            {
                back=txt++;
                if (*(txt+1)=='?')
                {
                    txt++;
                    txt++;
                    while (*txt==';'||(*txt>='0'&&*txt<='9'))
                        txt++;
                    break;
                }

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
                    for (i=0;i<nt;i++)
                        switch (tok[i])
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
                            ccolor|=0x100;
                            break;
                        case 4:
                            ccolor|=0x200;
                            break;
                        case 5:
                            ccolor|=0x80;
                            break;
                        case 7:
                            ccolor=(ccolor&~0x77)|(ccolor&0x70>>4)|(ccolor&7);
                            /* inverse should propagate... oh well */
                            break;
                        case 9:
                            ccolor|=0x400;
                            break;
                        case 21:
                            ccolor&=~8;
                            break;
                        case 22:
                            ccolor&=~8;
                            if (!(ccolor&0xf))
                                ccolor|=7;
                            break;
                        case 23:
                            ccolor&=~0x100;
                            break;
                        case 24:
                            ccolor&=~0x200;
                            break;
                        case 25:
                            ccolor&=~0x80;
                            break;
                        case 29:
                            ccolor&=~0x400;
                            break;
                        case 38:
                            i++;
                            if (i>=nt)
                                break;
                            if (tok[i]==5 && i+1<nt)
                            {   /* 256 colours */
                                i++;
                                ccolor=(ccolor&~0xf)|rgb_foreground(rgb_from_256(tok[i]));
                            }
                            else if (tok[i]==2 && i+3<nt)
                            {   /* 24 bit */
                                struct rgb c =
                                {
                                    .r = tok[i+1],
                                    .g = tok[i+2],
                                    .b = tok[i+3],
                                };
                                ccolor=(ccolor&~0xf)|rgb_foreground(c);
                                i+=3;
                            }
                            /* Subcommands 3 (CMY) and 4 (CMYK) are so insane
                             * there's no point in supporting them.
                             */
                            break;
                        case 48:
                            i++;
                            if (i>=nt)
                                break;
                            if (tok[i]==5 && i+1<nt)
                            {   /* 256 colours */
                                i++;
                                ccolor=(ccolor&~0x70)|rgb_background(rgb_from_256(tok[i]));
                            }
                            else if (tok[i]==2 && i+3<nt)
                            {   /* 24 bit */
                                struct rgb c =
                                {
                                    .r = tok[i+1],
                                    .g = tok[i+2],
                                    .b = tok[i+3],
                                };
                                ccolor=(ccolor&~0x70)|rgb_background(c);
                                i+=3;
                            }
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
                                ccolor=(ccolor&~0x07)|colors[tok[i]-30];
                            else if (tok[i]>=40 && tok[i]<48)
                                ccolor=(ccolor&~0x70)|(colors[tok[i]-40]<<4);
                            else if (tok[i]>=90 && tok[i]<98)
                                ccolor=(ccolor&~0x07)|8|colors[tok[i]-90];
                            else if (tok[i]>=100 && tok[i]<108) /* not bright */
                                ccolor=(ccolor&~0x70)|(colors[tok[i]-100]<<4);
                            /* ignore unknown attributes */
                        }
                    out+=setcolor(out,ccolor);
                    break;
                case 'C':
                    if (tok[0]<0)     /* sanity check */
                        break;
                    if (out-OUT+tok[0]>INPUT_CHUNK*2)
                        break;       /* something fishy is going on */
                    for (i=0;i<tok[0];i++)
                        *out++=' ';
                    break;
                case 'D': /* this interpretation is badly invalid... */
                case 'K':
                    out=OUT;
                    out+=setcolor(out,ccolor);
                    break;
                case 'J':
                    if (tok[0])
                        *out++=12; /* Form Feed */
                    break;
                default:
error:
                    txt=back;
                }
            }
            else if (*(txt+1)=='%' && *(txt+2)=='G')
                txt+=2;
            else if (*(txt+1)==']')
            {
                txt+=2;
                if (!isadigit(*txt)) /* malformed */
                    break;
                nt=0;
                while (isadigit(*txt))
                    nt=nt*10+*txt++-'0';
                if (*txt!=';')
                    break;
                back=++txt;
                while (*txt && *txt!=7 && *txt!=27)
                    txt++;
                if (*txt==27)
                    *txt++=0;
                else if (*txt==7)
                    *txt=0;
                else
                    break;
                switch (nt)
                {
                case 0: /* set window title */
                    if (!ses)
                        break;
                    if (back-TXT<=ses->lastintitle)
                        break;
                    ses->lastintitle=back-TXT;
                    do_hook(ses, HOOK_TITLE, back, 1);
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
        switch (mudcolors)
        {
        case 3:
            tintin_printf(0,"#Warning: no color codes set, use #mudcolors");
            mudcolors=2;
        case 2:
            break;
        case 1:
            strcpy(txt,MUDcolors[c&0xf]);
            txt+=strlen(MUDcolors[c&0xf]);
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
        SFREE(MUDcolors[nc]);
        MUDcolors[nc]=mystrdup(cc[nc]);
    };
    tintin_printf(ses,"#outgoing color codes table initialized");
}
