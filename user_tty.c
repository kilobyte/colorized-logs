#include "config.h"
#include "tintin.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <wctype.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <stdarg.h>
#include <wchar.h>
#include "unicode.h"
#include "ui.h"


extern int wc_to_utf8(char *d, const wchar_t *s, int n, int maxb);
extern int getcolor(char **ptr,int *color,const int flag);
extern int one_utf8_to_mb(char **d, char **s, mbstate_t *cs);
extern void utf8_to_local(char *d, char *s);
extern char translit(WC ch);
mbstate_t outstate;
#define OUTSTATE &outstate

#if HAVE_TERMIOS_H
# include <termios.h>
#endif
#if GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

#define B_LENGTH CONSOLE_LENGTH

extern char status[BUFFER_SIZE];
extern int find_bind(char *key,int msg,struct session *ses);
extern int keypad,retain;
extern int setcolor(char *txt,int c);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern void syserr(char *msg, ...);
extern struct session *activesession, *lastdraft;
extern void telnet_resize_all(void);
extern struct session *zap_command(char *arg, struct session *ses);
extern int wc_to_mb(char *d, wchar_t *s, int n, mbstate_t *cs);
extern int utf8_to_wc(wchar_t *d, char *s, int n);
extern void utf8_to_mb(char **d, char *s, mbstate_t *cs);

static char out_line[BUFFER_SIZE],b_draft[BUFFER_SIZE];
static WC k_input[BUFFER_SIZE],kh_input[BUFFER_SIZE],tk_input[BUFFER_SIZE];
static WC yank_buffer[BUFFER_SIZE];
static int k_len,k_pos,k_scrl,tk_len,tk_pos,tk_scrl;
static int o_len,o_pos,o_oldcolor,o_prevcolor,o_draftlen,o_lastprevcolor;
#define o_color color
#define o_lastcolor lastcolor
static int b_first,b_current,b_last,b_bottom,b_screenb,o_strongdraft;
static char *b_output[B_LENGTH];
static int scr_len,scr_curs;
extern int isstatus;
extern int hist_num;
extern char *history[HISTORY_SIZE];
static int in_getpassword;
extern int margins,marginl,marginr;
static struct termios old_tattr;
static int retaining;
#ifdef XTERM_TITLE
static int xterm;
#endif
static int putty;
extern int need_resize;

static char term_buf[BUFFER_SIZE*8],*tbuf;

static void term_commit(void)
{
    write(1,term_buf,tbuf-term_buf);
    tbuf=term_buf;
}

static int term_init(void)
{
    struct termios tattr;

    if (!(tty=isatty(0)))
    {
        fprintf(stderr,"Warning! isatty() reports stdin is not a terminal.\n");
        return 0;
    }

    tcgetattr(0,&old_tattr);
    tattr=old_tattr;
    /* cfmakeraw(&tattr); */
    tattr.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                       |INLCR|IGNCR|ICRNL|IXON);
    tattr.c_oflag &= ~OPOST;
    tattr.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    tattr.c_cflag &= ~(CSIZE|PARENB);
    tattr.c_cflag |= CS8;

#ifndef IGNORE_INT
    tattr.c_lflag|=ISIG;        /* allow C-c, C-\ and C-z */
#endif
    tattr.c_cc[VMIN]=1;
    tattr.c_cc[VTIME]=0;
    tcsetattr(0,TCSANOW,&tattr);

    return 1;
}

static void term_restore(void)
{
    tcdrain(0);
    tcsetattr(0,TCSADRAIN,&old_tattr);
}

static void term_getsize(void)
{
    struct winsize ts;

    if (ioctl(1,TIOCGWINSZ,&ts))
/*        syserr("ioctl(TIOCGWINSZ)");*/
    {       /* not a terminal, let's quietly assume 80x25 */
        LINES=25;
        COLS=80;
        return;
    }
    LINES=ts.ws_row;
    COLS=ts.ws_col;
}

#ifdef USER_DEBUG
static void debug_info(void)
{
    char txt[BUFFER_SIZE];
    sprintf(txt,"b_first=%d, b_current=%d, b_last=%d, b_bottom=%d, b_screenb=%d",
            b_first, b_current, b_last, b_bottom, b_screenb);
    tbuf+=sprintf(tbuf,"\033[1;%df\033[41;33;1m[\033[41;35m%s\033[41;33m]\033[37;40;0m",COLS-strlen(txt)-1,txt);
    sprintf(txt,"k_len=%d, strlen(k_input)=%d, k_pos=%d, k_scrl=%d",
            k_len, WClen(k_input), k_pos, k_scrl);
    tbuf+=sprintf(tbuf,"\033[2;%df\033[41;33;1m[\033[41;35m%s\033[41;33m]\033[37;40;0m",COLS-strlen(txt)-1,txt);
}
#endif


static void redraw_cursor(void)
{
    tbuf+=sprintf(tbuf,"\033[%d;%df",scr_len+1,scr_curs+1);
}

static void countpos(void)
{
    k_pos=k_len=WClen(k_input);
    k_scrl=0;
}

/**********************/
/* rewrite input line */
/**********************/
static void redraw_in(void)
{
    int i,l,r,k;

    tbuf+=sprintf(tbuf,"\033[%d;1f\033[0;37;4%dm",scr_len+1,INPUT_COLOR);
    if (k_pos<k_scrl)
        k_scrl=k_pos;
    if (k_pos-k_scrl+(k_scrl!=0)>=COLS)
        k_scrl=k_pos+2-COLS;
    if (k_scrl)
        tbuf+=sprintf(tbuf,"\033[1m<\033[0;4%dm",INPUT_COLOR);
    if (retaining)
        tbuf+=sprintf(tbuf,"\033[30;1m");
    if (in_getpassword)
    {
        l=WClen(&(k_input[k_scrl]));
        l=(l<COLS-2+(k_scrl==0))?l:COLS-2+(k_scrl==0);
        for (i=0;i<l;i++)
            *tbuf++='*';
    }
    else
    {
        l=COLS-2+(k_scrl==0);
        if (margins)
        {
            if (k_scrl+l>k_len)
                for (r=k_len-k_scrl;r<l;r++)
                    (k_input+k_scrl)[r]=' ';
            r=k_scrl;
            k=marginl-k_scrl-1;
            if (k>0)
            {
                tbuf+=OUT_WC(tbuf, k_input+r, k);
                r+=k;
                l-=k;
                k=0;
            };
            tbuf+=sprintf(tbuf,"\033[4%dm",MARGIN_COLOR);
            k+=marginr-marginl+1;
            if (k>l)
                k=l;
            if (k>0)
            {
                tbuf+=OUT_WC(tbuf, k_input+r, k);
                r+=k;
                l-=k;
            };
            tbuf+=sprintf(tbuf,"\033[4%dm",INPUT_COLOR);
            if (l>0)
                tbuf+=OUT_WC(tbuf, k_input+r, l);
            k_input[k_len]=0;
        }
        else
        {
            if (k_scrl+l>k_len)
                l=k_len-k_scrl;
            tbuf+=OUT_WC(tbuf, k_input+k_scrl, l);
        }
    };
    if (k_scrl+COLS-2<k_len)
        tbuf+=sprintf(tbuf,"\033[1m>");
    else
        if (!putty)
            tbuf+=sprintf(tbuf,"\033[0K");
        else
            for(i=l;i<COLS-!!k_scrl;i++)
                *tbuf++=' ';
    scr_curs=(k_scrl!=0)+k_pos-k_scrl;
    redraw_cursor();
    term_commit();
}

/*************************/
/* write the status line */
/*************************/
static void redraw_status(void)
{
    char *pos;
    int c,color=7;

    if (!isstatus)
        return;
    tbuf+=sprintf(tbuf,"\033[%d;1f\033[0;3%d;4%dm\033[2K\r",LINES,
                  STATUS_COLOR==COLOR_BLACK?7:0,STATUS_COLOR);
    if (!*(pos=status))
        goto end;
    while(*pos)
    {
        if (getcolor(&pos,&color,0))
        {
            c=color;
            if (!(c&0x70))
                c=c|(STATUS_COLOR<<4);
            if ((c&15)==((c>>4)&7))
                c=(c&0xf0)|(c&7? 0:(STATUS_COLOR==COLOR_BLACK? 7:0));
            tbuf+=sprintf(tbuf,COLORCODE(c));
            pos++;
        }
        else
#ifdef UTF8
            one_utf8_to_mb(&tbuf, &pos, &outstate);
#else
            *tbuf++=*pos++;
#endif
    };
end:
    redraw_cursor();
    term_commit();
}

static int b_shorten()
{
    if (b_first>b_bottom)
        return(FALSE);
    free(b_output[b_first++%B_LENGTH]);
    return(TRUE);
}

/********************************************/
/* write a single line to the output window */
/********************************************/
static void draw_out(char *pos)
{
    int c=7;
    while(*pos)
    {
        if (getcolor(&pos,&c,0))
        {
            tbuf+=sprintf(tbuf,COLORCODE(c));
            pos++;
            continue;
        };
#ifdef UTF8
        one_utf8_to_mb(&tbuf, &pos, &outstate);
#else
        *tbuf++=*pos;
#endif
    };
}

/****************************/
/* scroll the output window */
/****************************/
static void b_scroll(int b_to)
{
    int y;
    if (b_screenb==(b_to=(b_to<b_first?b_first:(b_to>b_bottom?b_bottom:b_to))))
        return;
    tbuf+=sprintf(tbuf,"\033[%d;%df\033[0;37;40m\033[1J",scr_len,COLS);
    term_commit();
    for (y=b_to-LINES+(isstatus?3:2);y<=b_to;++y)
        if ((y>=1)&&(y>b_first))
        {
            tbuf+=sprintf(tbuf,"\033[%d;1f",scr_len+y-b_to);
            if (y<b_current)
                draw_out(b_output[y%B_LENGTH]);
            else
                if (y==b_current)
                    draw_out(out_line);
                else
                    tbuf+=sprintf(tbuf,"\033[2K");
            term_commit();
        };
    tbuf+=sprintf(tbuf,"\0337");

    if (b_screenb==b_bottom)
    {
        int x;
        tbuf+=sprintf(tbuf,"\033[%d;1f\033[0;1;37;4%dm\033[2K\033[?25l",
                scr_len+1,INPUT_COLOR);
        for (x=0;x<COLS;++x)
            *tbuf++='^';
    }
    else
        tbuf+=sprintf(tbuf,"\033[?25h");
#ifdef USER_DEBUG
    debug_info();
#endif
    if ((b_screenb=b_to)==b_bottom)
        redraw_in();
    term_commit();
}

static void b_addline(void)
{
    char *new;
    while (!(new=(char*)malloc(o_len+1)))
        if (!b_shorten())
            syserr("Out of memory");
    out_line[o_len]=0;
    strcpy(new,out_line);
    if (b_bottom==b_first+B_LENGTH)
        b_shorten();
    b_output[(unsigned int)b_current%(unsigned int)B_LENGTH]=new;
    o_pos=0;
    o_len=setcolor(out_line,o_oldcolor=o_color);
    if (b_bottom<++b_current)
    {
        b_bottom=b_current;
        if (b_current==b_screenb+1)
        {
            ++b_screenb;
            draw_out(out_line);
            term_commit();
        }
        else
            tbuf=term_buf;
    }
}

/****************************************************/
/* textout - write some text into the output window */
/****************************************************/
static inline void print_char(const WC ch)
{
    int clen;

    if (o_pos==COLS)
    {
        tbuf+=sprintf(tbuf,"\033[0;37;40m\r\n\033[2K");
        b_addline();
    }
    else    /* b_addline already updates the color */
        if (o_oldcolor!=o_color)
        {
            if ((o_color&15)==((o_color&0x70)>>4))
                o_color=(o_color&0xf0)|((o_color&7)?0:7);
            o_len+=setcolor(out_line+o_len,o_color);
            if (b_screenb==b_bottom)
                tbuf+=sprintf(tbuf,COLORCODE(o_color));
            o_oldcolor=o_color;
        };
#ifdef UTF8
    clen=wcrtomb(tbuf, ch, &outstate);
    if (clen!=-1)
        tbuf+=clen;
    else
        *tbuf++=translit(ch);
    o_len+=wc_to_utf8(out_line+o_len, &ch, 1, BUFFER_SIZE-8+term_buf-tbuf);
#else
    *tbuf++=ch;
    out_line[o_len++]=ch;
#endif
    ++o_pos;
}

static void b_textout(char *txt)
{
#ifdef UTF8
    wchar_t u[2];
#endif

    /* warning! terminal output can get discarded! */
    tbuf+=sprintf(tbuf,"\0338");
    for (;*txt;txt++)
        switch(*txt)
        {
        case 27:    /* we're not supposed to see escapes here */
            print_char('^');
            print_char('[');
            break;
        case 7:
            write(1,"\007",1);
            break;
        case 8:
        case 127:
        case '\r':
            break;
        case '\n':
            out_line[o_len]=0;
            tbuf+=sprintf(tbuf,"\033[0;37;40m\r\n\033[2K");
            b_addline();
            break;
        case 9:
            if (o_pos<COLS) /* tabs on the right side of screen are ignored */
                do print_char(' '); while (o_pos&7);
            break;
        case '~':
            if (getcolor(&txt,&o_color,1))
            {
                if (o_color==-1)
                    o_color=o_prevcolor;
                break;
            };
            /* fall through */
        default:
#ifdef UTF8
            txt+=utf8_to_wc(u, txt, 1)-1;
            print_char(u[0]);
#else
            print_char(*txt);
#endif
        };
    out_line[o_len]=0;
    tbuf+=sprintf(tbuf,"\0337");
#ifdef USER_DEBUG
    debug_info();
#endif
    redraw_cursor();
    if (b_screenb==b_bottom)
        term_commit();
    else
        tbuf=term_buf;
    o_prevcolor=o_color;
}

/************************ debugging function ******/
static void b_textout_test(char *txt)
{
    char bb[BUFFER_SIZE],*b,*t;

    for (b=bb;*txt;txt++)
        switch(*txt)
        {
        case 001:
            b+=sprintf(b,"~6~");
            break;
        case 002:
            b+=sprintf(b,"~7~");
            break;
        case '\n':
            b+=sprintf(b,"~2~\\n~7~");
            break;
        case '~':         /* broken... */
            t=txt+1;
            if (!iswdigit(*t))
                goto nope;
            while (iswdigit(*t))
                t++;
            if (*t=='~')
            {
                b+=sprintf(b,"~5~`~5~");
                for(txt++;txt<t;txt++)
                    *b++=*txt;
                b+=sprintf(b,"~5~`~7~");
                break;
            };
nope:
        default:
            *b++=*txt;
        };
    b[0]='\n';
    b[1]=0;

    b_textout(bb);
}

static void b_canceldraft(void)
{
    if (b_bottom==b_screenb)
    {
        tbuf+=(o_oldcolor&0x70)?
                    sprintf(tbuf,"\0338\033[0m\033[2K"):
                    sprintf(tbuf,"\0338\033[2K");
        while (b_current>b_last)
        {
            b_current--;
            tbuf+=sprintf(tbuf,"\033[A\033[2K");
        };
        tbuf+=sprintf(tbuf,"\r"COLORCODE(o_lastcolor));
        tbuf+=sprintf(tbuf,"\0337");
    }
    else
        b_current=b_last;
    o_oldcolor=o_color=o_lastcolor;
    o_prevcolor=o_lastprevcolor;
    o_len=setcolor(out_line,o_lastcolor);
    o_pos=0;
}

static void usertty_textout(char *txt)
{
    if (o_draftlen)
    {                  /* text from another source came, we need to either */
        if (o_strongdraft)
        {
            o_draftlen=0;                              /* accept the draft */
            if (lastdraft)
                lastdraft->last_line[0]=0;
            lastdraft=0;
        }
        else
            b_canceldraft();                  /* or temporarily discard it */
    }
    b_textout(txt);
    b_last=b_current;
    o_lastcolor=o_color;
    o_lastprevcolor=o_prevcolor;
    if (o_draftlen)
        b_textout(b_draft); /* restore the draft */
#ifdef USER_DEBUG
    debug_info();
    redraw_cursor();
    term_commit();
#endif
}

static void usertty_textout_draft(char *txt, int flag)
{
    if (o_draftlen)
        b_canceldraft();
    if (txt)
    {
        strcpy(b_draft,txt);
#ifdef USER_DEBUG
        strcat(b_draft,"\376");
#endif    
        if ((o_draftlen=strlen(b_draft)))
            b_textout(b_draft);
#ifdef USER_DEBUG
        debug_info();
        redraw_cursor();
        term_commit();
#endif
    }
    else
    {
        b_draft[0]=0;
        o_draftlen=0;
    }
    o_strongdraft=flag;
}

static int transpose_words()
{
    WC buf[BUFFER_SIZE];
    int a1,a2,b1,b2;
    
    a2=k_pos;
    while(a2<k_len && !iswalnum(k_input[a2]))
        a2++;
    if (a2==k_len)
        while(a2 && !iswalnum(k_input[a2-1]))
            a2--;
    while(a2 && iswalnum(k_input[a2-1]))
        a2--;
    if (!a2)
        return 1;
    b2=a2;
    while(iswalnum(k_input[b2+1]))
        b2++;
    b1=a2;
    do { if(--b1<0) return 1; } while(!iswalnum(k_input[b1]));
    a1=b1;
    while(a1>0 && iswalnum(k_input[a1-1]))
        a1--;
    memcpy(buf, k_input+a2, (b2+1-a2)*WCL);
    memcpy(buf+b2+1-a2, k_input+b1+1, (a2-b1-1)*WCL);
    memcpy(buf+b2-b1, k_input+a1, (b1+1-a1)*WCL);
    memcpy(k_input+a1, buf, (b2+1-a1)*WCL);
    k_pos=b2+1;
    return 0;
}

static int ret(int r)
{
    if (!retaining)
        return 0;
    if (!r)
    {
        k_len=k_pos=k_scrl=0;
        k_input[0]=0;
        scr_curs=0;
    }
    retaining=0;
    return 1;
}

static int state=0;
static int val=0;
static int usertty_process_kbd(struct session *ses, WC ch)
{
    char txt[16];
    int i;
    
    switch(state)
    {
#if 0
    case 5:
        state=0;
        goto insert_verbatim;
        break;
#endif
    case 4:			/* ESC O */
        state=0;
        switch(ch)
        {
        case 'A':
            goto prev_history;
        case 'B':
            goto next_history;
        case 'C':
            goto key_cursor_right;
        case 'D':
            goto key_cursor_left;
        case 'H':
            goto key_home;
        case 'F':
            goto key_end;
        }
        if (b_bottom!=b_screenb)
            b_scroll(b_bottom);
        {
            sprintf(txt, "ESCO"WCC, ch);
            find_bind(txt,1,ses);
        };
        break;
    case 3:			/* ESC [ [ */
        state=0;
        if (b_bottom!=b_screenb)
            b_scroll(b_bottom);
        {
            sprintf(txt, "ESC[["WCC, ch);
            find_bind(txt,1,ses);
        };
        break;
    case 2:			/* ESC [ */
        state=0;
        if (iswdigit(ch))
        {
            val=val*10+(ch-'0');
            state=2;
        }
        else
            if (iswupper(ch))
            {
                switch(ch)
                {

                prev_history:
                case 'A':	/* up arrow */
                    if (ret(0))
                        redraw_in();
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    if (!ses)
                        break;
                    if (hist_num==HISTORY_SIZE-1)
                        break;
                    if (!history[hist_num+1])
                        break;
                    if (hist_num==-1)
                        WCcpy(kh_input, k_input);
                    TO_WC(k_input, history[++hist_num]);
                    countpos();
                    redraw_in();
                    break;
                next_history:
                case 'B':	/* down arrow */
                    if (ret(0))
                        redraw_in();
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    if (!ses)
                        break;
                    if (hist_num==-1)
                        break;
                    do --hist_num;
                    while ((hist_num>0)&&!(history[hist_num]));
                    if (hist_num==-1)
                        WCcpy(k_input, kh_input);
                    else
                        TO_WC(k_input, history[hist_num]);
                    countpos();
                    redraw_in();
                    break;
                key_cursor_left:
                case 'D':	/* left arrow */
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    if (ret(1))
                       redraw_in();
                    if (k_pos==0)
                        break;
                    --k_pos;
                    if (k_pos>=k_scrl)
                    {
                        scr_curs=k_pos-k_scrl+(k_scrl!=0);
                        redraw_cursor();
                        term_commit();
                    }
                    else
                        redraw_in();
                    break;
                key_cursor_right:
                case 'C':	/* right arrow */
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    if (ret(1))
                       redraw_in();
                    if (k_pos==k_len)
                        break;
                    ++k_pos;
                    if (k_pos<=k_scrl+COLS-2)
                    {
                        scr_curs=k_pos-k_scrl+(k_scrl!=0);
                        redraw_cursor();
                        term_commit();
                    }
                    else
                        redraw_in();
                    break;
                case 'H':
                    goto key_home;
                case 'F':
                    goto key_end;
                default:
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    {
                        sprintf(txt, "ESC["WCC, ch);
                        find_bind(txt,1,ses);
                        break;
                    };
                }
            }
            else
                if (ch=='[')
                    state=3;
                else
                    if (ch=='~')
                        switch(val)
                        {
                        case 5:		/* [PgUp] */
                            if (b_screenb>b_first+LINES-(isstatus?3:2))
                                b_scroll(b_screenb+(isstatus?3:2)-LINES);
                            else
                                write(1,"\007",1);
                            break;
                        case 6:		/* [PgDn] */
                            if (b_screenb<b_bottom)
                                b_scroll(b_screenb+LINES-(isstatus?3:2));
                            else
                                write(1,"\007",1);
                            break;
                        key_home:
                        case 1:		/* [Home] */
                        case 7:
                            if (b_bottom!=b_screenb)
                                b_scroll(b_bottom);
                            if (ret(1))
                                redraw_in();
                            if (!k_pos)
                                break;
                            k_pos=0;
                            if (k_pos>=k_scrl)
                            {
                                scr_curs=k_pos-k_scrl+(k_scrl!=0);
                                redraw_cursor();
                                term_commit();
                            }
                            else
                                redraw_in();
                            break;
                        key_end:
                        case 4:		/* [End] */
                        case 8:
                            if (b_bottom!=b_screenb)
                                b_scroll(b_bottom);
                            if (ret(1))
                                redraw_in();
                            if (k_pos==k_len)
                                break;
                            k_pos=k_len;
                            if (k_pos<=k_scrl+COLS-2)
                            {
                                scr_curs=k_pos-k_scrl+(k_scrl!=0);
                                redraw_cursor();
                                term_commit();
                            }
                            else
                                redraw_in();
                            break;
                        key_del:
                        case 3:		/* [Del] */
                            ret(0);
                            if (b_bottom!=b_screenb)
                                b_scroll(b_bottom);
                            if (k_pos!=k_len)
                            {
                                for (i=k_pos;i<=k_len;++i)
                                k_input[i]=k_input[i+1];
                                --k_len;
                            }
                            redraw_in();
                            break;
                        default:
                            if (b_bottom!=b_screenb)
                                b_scroll(b_bottom);
                            {
                                sprintf(txt,"ESC[%i~",val);
                                find_bind(txt,1,ses);
                                break;
                            }
                        };
        break;
    case 1:			/* ESC */
        if (ch=='[')
        {
            state=2; val=0;
            break;
        };
        if (ch=='O')
        {
            state=4; val=0;
            break;
        };
#ifndef BARE_ESC
        state=0;
        if (ch==127)
            sprintf(txt,"Alt-Backspace");
        else if ((unsigned char)ch>32)
            sprintf(txt,"Alt-"WCC,ch);
        else if (ch==32)
            sprintf(txt,"Alt-Space");
        else if (ch==27)
            sprintf(txt,"Alt-Esc");
        else if (ch==13)
            sprintf(txt,"Alt-Enter");
        else if (ch==9)
            sprintf(txt,"Alt-Tab");
        else
            sprintf(txt,"Alt-^"WCC,ch+64);
        if (find_bind(txt,0,ses))
            break;
        switch(ch)
        {
        case 9:     /* Alt-Tab */
            goto key_alt_tab;
        case '<':	/* Alt-< */
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (ret(0))
                redraw_in();
            if (!ses)
                break;
            if (hist_num==HISTORY_SIZE-1 || !history[hist_num+1])
                break;
            do hist_num++;
                while(hist_num!=HISTORY_SIZE-1 && history[hist_num+1]);
            TO_WC(k_input, history[hist_num]);
            countpos();
            redraw_in();
            break;
        case '>':	/* Alt-> */
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (ret(0))
                redraw_in();
            if (!ses)
                break;
            if (hist_num==-1)
                break;
            hist_num=-1;
            WCcpy(k_input, kh_input);
            countpos();
            redraw_in();
            break;
        case 'f':   /* Alt-F */
        case 'F':
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (ret(1))
               redraw_in();
            if (k_pos==k_len)
                break;
            while(k_pos<k_len && !iswalnum(k_input[k_pos]))
                ++k_pos;
            while(k_pos<k_len && iswalnum(k_input[k_pos]))
                ++k_pos;
            if (k_pos<=k_scrl+COLS-2)
            {
                scr_curs=k_pos-k_scrl+(k_scrl!=0);
                redraw_cursor();
                term_commit();
            }
            else
                redraw_in();
            break;
        case 'b':   /* Alt-B */
        case 'B':
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (ret(1))
               redraw_in();
            if (!k_pos)
                break;
            while(k_pos && !iswalnum(k_input[k_pos-1]))
                --k_pos;
            while(k_pos && iswalnum(k_input[k_pos-1]))
                --k_pos;
            if (k_pos>=k_scrl)
            {
                scr_curs=k_pos-k_scrl+(k_scrl!=0);
                redraw_cursor();
                term_commit();
            }
            else
                redraw_in();
            break;
        case 'l':   /* Alt-L */
        case 'L':
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            ret(1);
            while(k_pos<k_len && !iswalnum(k_input[k_pos]))
                ++k_pos;
            while(k_pos<k_len && iswalnum(k_input[k_pos]))
            {
                k_input[k_pos]=towlower(k_input[k_pos]);
                ++k_pos;
            }
            redraw_in();
            break;
        case 'u':   /* Alt-U */
        case 'U':
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (ret(1))
               redraw_in();
            while(k_pos<k_len && !iswalnum(k_input[k_pos]))
                ++k_pos;
            while(k_pos<k_len && iswalnum(k_input[k_pos]))
            {
                k_input[k_pos]=towupper(k_input[k_pos]);
                ++k_pos;
            }
            redraw_in();
            break;
        case 'c':   /* Alt-C */
        case 'C':
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            ret(1);
            while(k_pos<k_len && !iswalnum(k_input[k_pos]))
                ++k_pos;
            if(k_pos<k_len && iswalnum(k_input[k_pos]))
            {
                k_input[k_pos]=towupper(k_input[k_pos]);
                ++k_pos;
            }
            while(k_pos<k_len && iswalnum(k_input[k_pos]))
            {
                k_input[k_pos]=towlower(k_input[k_pos]);
                ++k_pos;
            }
            redraw_in();
            break;
        case 't':   /* Alt-T */
        case 'T':
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            ret(1);
            transpose_words();
            redraw_in();
            break;
        case 'd':  /* Alt-D */
        case 'D':
            if (ret(0))
                redraw_in();
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos==k_len)
                break;
            i=k_pos;
            while(i<k_len && !iswalnum(k_input[i]))
                i++;
            while(iswalnum(k_input[i]))
                i++;
            i-=k_pos;
            memcpy(yank_buffer, k_input+k_pos, i*WCL);
            yank_buffer[i]=0;
            memmove(k_input+k_pos, k_input+k_pos+i, (k_len-k_pos-i)*WCL);
            k_len-=i;
            k_input[k_len]=0;
            redraw_in();
            break;
        case 127:  /* Alt-Backspace */
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            ret(0);
            if (k_pos!=0)
            {
                i=k_pos-1;
                while((i>=0)&&!iswalnum(k_input[i]))
                    i--;
                while((i>=0)&&iswalnum(k_input[i]))
                    i--;
                i=k_pos-i-1;
                memmove(yank_buffer, k_input+k_pos-i, i*WCL);
                yank_buffer[i]=0;
                memmove(k_input+k_pos-i, k_input+k_pos, (k_len-k_pos)*WCL);
                k_len-=i;
                k_input[k_len]=0;
                k_pos-=i;
            }
            redraw_in();
            break;
        default:
            find_bind(txt,1,ses); /* FIXME: we want just the message */
    }
    break;
#else
        /* [Esc] */
        state=0;
        ret(0);
        tbuf+=sprintf(tbuf,"\0335n");
        if (b_bottom!=b_screenb)
            b_scroll(b_bottom);
        k_pos=0;
        k_scrl=0;
        k_len=0;
        k_input[0]=0;
        redraw_in();
        /* fallthrough */
#endif
    case 0:
        switch(ch)
        {
        case '\n':
        case '\r':
            ret(1);
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (!activesession->server_echo)
                in_getpassword=0;
            WRAP_WC(done_input, k_input);
            if (retain)
            {
                k_pos=k_len;
                retaining=1;
            }
            else
            {
                k_len=k_pos=k_scrl=0;
                k_input[0]=0;
                scr_curs=0;
            }
            redraw_in();
#if 0
            tbuf+=sprintf(tbuf,"\033[%d;1f\033[0;37;4%dm\033[2K",scr_len+1,INPUT_COLOR);
            if (margins&&(marginl<=COLS))
            {
                tbuf+=sprintf(tbuf,"\033[%d;%df\033[0;37;4%dm",
                              scr_len+1,marginl,MARGIN_COLOR);
                if (marginr<=COLS)
                    i=marginr+1-marginl;
                else
                    i=COLS+1-marginl;
                while (i--)
                    *tbuf++=' ';
                tbuf+=sprintf(tbuf,"\033[1;37;4%dm\033[%d;1f",INPUT_COLOR,scr_len+1);
            };
            scr_curs=0;
            term_commit();
#endif
            return(1);
        case 1:			/* ^[A] */
            if (find_bind("^A",0,ses))
                break;
            goto key_home;
        case 2:			/* ^[B] */
            if (find_bind("^B",0,ses))
                break;
            goto key_cursor_left;
        case 4:			/* ^[D] */
            if (find_bind("^D",0,ses))
                break;
            if (k_pos||k_len)
                goto key_del;
            if (ret(0))
                redraw_in();
            *done_input=0;
            activesession=zap_command("",ses);
            return(0);
        case 5:			/* ^[E] */
            if (find_bind("^E",0,ses))
                break;
            goto key_end;
        case 6:			/* ^[F] */
            if (find_bind("^F",0,ses))
                break;
            goto key_cursor_right;
        case 8:			/* ^[H] */
            if (find_bind("^H",0,ses))
                break;
        case 127:       /* [backspace] */
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            ret(0);
            if (k_pos!=0)
            {
                for (i=--k_pos;i<=k_len;++i)
                    k_input[i]=k_input[i+1];
                --k_len;
            };
            redraw_in();
            break;
        case 9:			/* [Tab],^[I] */
            if (find_bind("Tab",0,ses)||find_bind("^I",0,ses))
                break;
            {
                WC buf[BUFFER_SIZE];
key_alt_tab:
                ret(0);
                WCcpy(buf, k_input);
                WCcpy(k_input, tk_input);
                WCcpy(tk_input, buf);
                i=k_pos; k_pos=tk_pos; tk_pos=i;
                i=k_scrl; k_scrl=tk_scrl; tk_scrl=i;
                i=k_len; k_len=tk_len; tk_len=i;
            };
            redraw_in();
            break;
        case 11:		/* ^[K] */
            if (find_bind("^K",0,ses))
                break;
            ret(0);
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos!=k_len)
            {
                memmove(yank_buffer, k_input+k_pos, (k_len-k_pos)*WCL);
                yank_buffer[k_len-k_pos]=0;
                k_input[k_pos]=0;
                k_len=k_pos;
            }
            redraw_in();
            break;
        case 14:		/* ^[N] */
            if (find_bind("^N",0,ses))
                break;
            goto next_history;
        case 16:		/* ^[P] */
            if (find_bind("^P",0,ses))
                break;
            goto prev_history;
#if 0
        case 17:		/* ^[Q] */
            if (find_bind("^Q",0,ses))
                break;
            state=5;
            break;
#endif
        case 20:		/* ^[T] */
            if (find_bind("^T",0,ses))
                break;
            ret(1);
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos&&k_len)
            {
                if (k_pos==k_len)
                {
                    ch=k_input[k_pos-2];
                    k_input[k_pos-2]=k_input[k_pos-1];
                    k_input[k_pos-1]=ch;
                }
                else
                {
                    ch=k_input[k_pos];
                    k_input[k_pos]=k_input[k_pos-1];
                    k_input[k_pos-1]=ch;
                    k_pos++;
                }
            }
            redraw_in();
            break;
        case 21:		/* ^[U] */
            if (find_bind("^U",0,ses))
                break;
            ret(0);
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos!=0)
            {
                memmove(yank_buffer, k_input, k_pos*WCL);
                yank_buffer[k_pos]=0;
                memmove(k_input, k_input+k_pos, (k_len-k_pos+1)*WCL);
                k_len-=k_pos;
                k_pos=k_scrl=0;
            }
            redraw_in();
            break;
#if 0
        case 22:		/* ^[V] */
            if (find_bind("^V",0,ses))
                break;
            state=5;
            break;
#endif
        case 23:		/* ^[W] */
            if (find_bind("^W",0,ses))
                break;
            ret(0);
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos!=0)
            {
                i=k_pos-1;
                while((i>=0)&&iswspace(k_input[i]))
                    i--;
                while((i>=0)&&!iswspace(k_input[i]))
                    i--;
                i=k_pos-i-1;
                memmove(yank_buffer, k_input+k_pos-i, i*WCL);
                yank_buffer[i]=0;
                memmove(k_input+k_pos-i, k_input+k_pos, (k_len-k_pos)*WCL);
                k_len-=i;
                k_input[k_len]=0;
                k_pos-=i;
            }
            redraw_in();
            break;
        case 25:		/* ^[Y] */
            if (find_bind("^Y",0,ses))
                break;
            ret(0);
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (!*yank_buffer)
            {
                redraw_in();
                write(1,"\007",1);
                break;
            }
            i=WClen(yank_buffer);
            if (i+k_len>=BUFFER_SIZE)
                i=BUFFER_SIZE-1-k_len;
            memmove(k_input+k_pos+i, k_input+k_pos, (k_len-k_pos)*WCL);
            memmove(k_input+k_pos, yank_buffer, i*WCL);
            k_len+=i;
            k_input[k_len]=0;
            k_pos+=i;
            redraw_in();
            break;
        case 27:        /* [Esc] or a control sequence */
            state=1;
            break;
        default:
            if (ret(0))
                redraw_in();
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_len==BUFFER_SIZE-1)
                write(1,"\007",1);
            else
            {
                if ((ch>0)&&(ch<32))
                {
                    sprintf(txt,"^"WCC,ch+64);
                    find_bind(txt,1,ses);
                    break;
                };
#if 0
            insert_verbatim:
#endif
                k_input[++k_len]=0;
                for (i=k_len-1;i>k_pos;--i)
                    k_input[i]=k_input[i-1];
                k_input[k_pos++]=ch;
                if ((k_len==k_pos)&&(k_len<COLS))
                {
                    scr_curs++;
                    tbuf+=sprintf(tbuf,"\033[0;37;4%dm",
                                  margins&&
                                  (k_len>=marginl)&&(k_len<=marginr)
                                  ? MARGIN_COLOR : INPUT_COLOR);
                    if (in_getpassword)
                        *tbuf++='*';
                    else
#ifdef UTF8
                        tbuf+=OUT_WC(tbuf, &ch, 1);
#else
                        *tbuf+=ch;
#endif
                    term_commit();
                }
                else
                    redraw_in();
            }
        }
    };
#ifdef USER_DEBUG
    debug_info();
    redraw_cursor();
    term_commit();
#endif
    return(0);
}

/******************************/
/* set up the window outlines */
/******************************/
static void usertty_drawscreen(void)
{
    int i;

    need_resize=0;
    scr_len=LINES-1-isstatus;
    tbuf+=sprintf(tbuf,"\033[0;37;40m\033[2J\033[0;37;40m\033[1;%dr\0337",scr_len);
    tbuf+=sprintf(tbuf,"\033[%d;1f\033[0;37;4%dm",scr_len+1,INPUT_COLOR);
    if (!putty)
        tbuf+=sprintf(tbuf,"\033[2K");
    else
        for(i=0;i<COLS;i++)
            *tbuf++=' ';
    if (isstatus)
        tbuf+=sprintf(tbuf,"\033[%d;f\033[37;4%dm\033[2K",LINES,STATUS_COLOR);
}

static void usertty_keypad(int k)
{
    if (k)
        tbuf+=sprintf(tbuf,"\033=\033[?1051l\033[?1052l\033[?1060l\e[?1061h");
    else
        tbuf+=sprintf(tbuf,"\033>\033[?1051l\033[?1052l\033[?1060l\e[?1061l");
    term_commit();
}

static void usertty_retain()
{
    ret(0);
    redraw_in();
}

/*********************/
/* resize the screen */
/*********************/
static void usertty_resize(void)
{
    term_commit();
    term_getsize();
    usertty_drawscreen();
    b_screenb=-666;		/* impossible value */
    b_scroll(b_bottom);
    if (isstatus)
        redraw_status();
    redraw_in();
    
    telnet_resize_all();
}

static void usertty_show_status(void)
{
    int st;
    st=!!strcmp(status,EMPTY_LINE);
    if (st!=isstatus)
    {
        isstatus=st;
        usertty_resize();
    }
    else
        if (st)
        {
            redraw_status();
            redraw_cursor();
            term_commit();
        }
}

static void usertty_init(void)
{
    char* term;

    memset(&outstate, 0, sizeof(outstate));
#ifdef XTERM_TITLE
    xterm=getenv("DISPLAY")&&(getenv("WINDOWID")||getenv("KONSOLE_DCOP_SESSION"));
#endif
    /* some versions of PuTTY and screen badly support bg colors */
    putty=(term=getenv("TERM"))&&(!strcasecmp(term,"xterm")||!strncasecmp(term,"screen",6));
    term_getsize();
    term_init();
    tbuf=term_buf+sprintf(term_buf,"\033[?7l");
    usertty_keypad(keypad);
    isstatus=0;
    retaining=0;
    usertty_drawscreen();

    margins=0;
    marginl=
        k_len=0;
    k_pos=0;
    k_scrl=0;
    k_input[0]=0;
    tk_len=0;
    tk_pos=0;
    tk_scrl=0;
    tk_input[0]=0;
    in_getpassword=0;

    b_first=0;
    b_current=-1;
    b_bottom=-1;
    b_screenb=-1;
    b_last=-1;
    b_output[0]=out_line;
    out_line[0]=0;
    o_len=0;
    o_pos=0;
    o_color=7;
    o_oldcolor=7;
    o_prevcolor=7;
    o_lastprevcolor=7;
    o_draftlen=0;
    o_strongdraft=0;
    o_lastcolor=7;
    
    tbuf+=sprintf(tbuf,"\033[1;1f\0337");
    
    sprintf(done_input,"~12~KB~3~tin ~7~%s by ~11~kilobyte@angband.pl~9~\n",VERSION);
    usertty_textout(done_input);
    {
        int i;
        for (i=0;i<COLS;++i)
            done_input[i]='-';
        sprintf(done_input+COLS,"~7~");
    };
    usertty_textout(done_input);
}

static void usertty_done(void)
{
    tbuf+=sprintf(tbuf,"\033[1;%dr\033[%d;1f\033[?25h\033[?7h\033[0;37;40m",LINES,LINES);
    usertty_keypad(0);
    term_commit();
    term_restore();
    write(1,"\n",1);
}

static void usertty_pause(void)
{
    usertty_keypad(0);
    tbuf+=sprintf(tbuf,"\033[1;%dr\033[%d;1f\033[?25h\033[?7h\033[0;37;40m",LINES,LINES);
    term_commit();
    term_restore();
}

static void usertty_resume(void)
{
    term_getsize();
    term_init();
    tbuf=term_buf+sprintf(term_buf,"\033[?7l");
    usertty_keypad(keypad);
    usertty_drawscreen();
    b_screenb=-666;     /* impossible value */
    b_scroll(b_bottom);
    if (isstatus)
        redraw_status();
    redraw_in();
}


static void fwrite_out(FILE *f,char *pos)
{
    int c=7;
    char lstr[BUFFER_SIZE], ustr[BUFFER_SIZE], *s=ustr;
    
    for (;*pos;pos++)
    {
        if (*pos=='~')
            if (getcolor(&pos,&c,0))
            {
                if ((c>>4)&7)
                    s+=sprintf(s,COLORCODE(c));
                else	/* a kludge to make a certain log archive happy */
                    s+=sprintf(s,"\033[0%s;3%d%sm",((c)&8)?";1":"",colors[(c)&7],attribs[(c)>>7]);
                continue;
            };
        if (*pos!='\n')
            *s++=*pos;
    }
    *s=0;
#ifdef UTF8
    utf8_to_local(lstr, ustr);
    fputs(lstr, f);
#else
    fputs(ustr, f);
#endif
}

static void usertty_condump(FILE *f)
{
    int i;
    for (i=b_first;i<b_current;i++)
    {
        fwrite_out(f,b_output[i%B_LENGTH]);
        fprintf(f,"\n");
    };
    fwrite_out(f,out_line);
}

static void usertty_passwd(int x)
{
    ret(0);
    in_getpassword=x;
    redraw_in();
}

static void usertty_title(char *fmt,...)
{
    va_list ap;
#ifdef HAVE_VSNPRINTF
    char buf[BUFFER_SIZE];
#else
    char buf[BUFFER_SIZE*4]; /* let's hope this will never overflow... */
#endif
#ifndef XTERM_TITLE
    if (!xterm)
#endif
        return;

    va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
    if (vsnprintf(buf, BUFFER_SIZE-1, fmt, ap)>BUFFER_SIZE-2)
        buf[BUFFER_SIZE-3]='>';
#else
    vsprintf(buf, fmt, ap);
#endif
    va_end(ap);

#ifdef UTF8
    tbuf+=sprintf(tbuf,"\033]0;");
    utf8_to_mb(&tbuf, buf, &outstate);
    *tbuf++='\007';
#else
    tbuf+=sprintf(tbuf,"\033]0;%s\007",buf);
#endif
    term_commit();
}

static void usertty_beep(void)
{
    write(1,"\007",1);
}


void usertty_initdriver()
{
    ui_sep_input=1;
    ui_con_buffer=1;
    ui_keyboard=1;
    ui_own_output=1;
    ui_tty=1;
    ui_drafts=1;
    
    user_init		= usertty_init;
    user_done		= usertty_done;
    user_pause		= usertty_pause;
    user_resume 	= usertty_resume;
    user_textout 	= usertty_textout;
    user_textout_draft 	= usertty_textout_draft;
    user_process_kbd	= usertty_process_kbd;
    user_beep		= usertty_beep;
    user_keypad		= usertty_keypad;
    user_retain		= usertty_retain;
    user_passwd		= usertty_passwd;
    user_condump	= usertty_condump;
    user_title		= usertty_title;
    user_resize		= usertty_resize;
    user_show_status	= usertty_show_status;
}
