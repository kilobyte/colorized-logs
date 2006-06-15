#include "tintin.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_CONFIG_H
# include "config.h"

# if HAVE_TERMIOS_H
#  include <termios.h>
# endif
# if GWINSZ_IN_SYS_IOCTL
#  include <sys/ioctl.h>
# endif

# ifdef HAVE_STRING_H
#  include <string.h>
# else
#  ifdef HAVE_STRINGS_H
#   include <strings.h>
#  endif
# endif
#endif

#define B_LENGTH CONSOLE_LENGTH

extern char status[BUFFER_SIZE];
extern int find_bind(char *key,int msg,struct session *ses);
extern int keypad;
extern inline int getcolor(char **ptr,int *color,const int flag);
extern inline int setcolor(char *txt,int c);
extern struct session *parse_input(char *input,int override_verbatim,struct session *ses);
extern void syserr(char *msg);
extern struct session *activesession, *lastdraft;
extern void telnet_resize_all(void);

char done_input[BUFFER_SIZE],out_line[BUFFER_SIZE],b_draft[BUFFER_SIZE];
char k_input[BUFFER_SIZE],kh_input[BUFFER_SIZE],tk_input[BUFFER_SIZE];
char yank_buffer[BUFFER_SIZE];
int k_len,k_pos,k_scrl,tk_len,tk_pos,tk_scrl;
int o_len,o_pos,o_color,o_oldcolor,o_prevcolor,o_draftlen,o_lastcolor,o_lastprevcolor;
int b_first,b_current,b_last,b_bottom,b_screenb,o_strongdraft;
char *b_output[B_LENGTH];
int LINES,COLS,scr_len,scr_curs;
int isstatus;
int hist_num;
int need_resize;
int user_getpassword;
int margins,marginl,marginr;
struct termios old_tattr;

char term_buf[BUFFER_SIZE],*tbuf;

const int colors[8]={0,4,2,6,1,5,3,7};

#ifdef GRAY2
const char *fcolors[16]={";30",  ";34",  ";32",  ";36",
                            ";31",  ";35",  ";33",  "",
                        ";2",   ";34;1",";32;1",";36;1",
                            ";31;1",";35;1",";33;1",";1"};
const char *bcolors[8]={"",    ";44",    ";42",   ";46",
                            ";41",  ";45",  ";43",  "47"};
#define COLORCODE(c) "\e[0%s%s%sm",fcolors[(c)&15],bcolors[((c)>>4)&7],(c)>>7? ";5":""
#else
#define COLORCODE(c) "\e[0%s;3%d;4%d%sm",(c&8)?";1":"",colors[c&7],colors[(c>>4)&7],c>>7? ";5":""
#endif

void term_commit(void)
{
    write(1,term_buf,tbuf-term_buf);
    tbuf=term_buf;
}

int term_init(void)
{
    struct termios tattr;

    if (!isatty(0))
    {
        fprintf(stderr,"Warning! isatty() reports stdin is not a terminal.\n");
        return 0;
    }

    tcgetattr(0,&old_tattr);
    tattr=old_tattr;
    /*	cfmakeraw(&tattr); */
    tattr.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                       |INLCR|IGNCR|ICRNL|IXON);
    tattr.c_oflag &= ~OPOST;
    tattr.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    tattr.c_cflag &= ~(CSIZE|PARENB);
    tattr.c_cflag |= CS8;

#ifndef IGNORE_INT
    tattr.c_lflag|=ISIG;	/* allow C-c, C-\ and C-z */
#endif
    tattr.c_cc[VMIN]=1;
    tattr.c_cc[VTIME]=0;
    tcsetattr(0,TCSANOW,&tattr);

    return 1;
}

void term_restore(void)
{
    tcsetattr(0,TCSADRAIN,&old_tattr);
}

void term_getsize(void)
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

#ifdef DEBUG
void debug_info(void)
{
    char txt[BUFFER_SIZE];
    sprintf(txt,"b_first=%d, b_current=%d, b_last=%d, b_bottom=%d, b_screenb=%d",
            b_first,b_current,b_last,b_bottom,b_screenb);
    tbuf+=sprintf(tbuf,"\e[1;%df\e[41;33;1m[\e[41;35m%s\e[41;33m]\e[37;40;0m",COLS-strlen(txt)-1,txt);
    sprintf(txt,"k_len=%d, strlen(k_input)=%d, k_pos=%d, k_scrl=%d",
            k_len,strlen(k_input),k_pos,k_scrl);
    tbuf+=sprintf(tbuf,"\e[2;%df\e[41;33;1m[\e[41;35m%s\e[41;33m]\e[37;40;0m",COLS-strlen(txt)-1,txt);
}
#endif


void redraw_cursor(void)
{
    tbuf+=sprintf(tbuf,"\e[%d;%df",scr_len+1,scr_curs+1);
}

void countpos(void)
{
    k_pos=k_len=strlen(k_input);
    k_scrl=0;
}

/**********************/
/* rewrite input line */
/**********************/
void redraw_in(void)
{
    tbuf+=sprintf(tbuf,"\e[%d;1f\e[0;37;4%dm",scr_len+1,INPUT_COLOR);
    if (k_pos<k_scrl)
        k_scrl=k_pos;
    if (k_pos-k_scrl+(k_scrl!=0)>=COLS)
        k_scrl=k_pos+2-COLS;
    if (k_scrl)
        tbuf+=sprintf(tbuf,"\e[1m<\e[0;4%dm",INPUT_COLOR);
    if (user_getpassword)
    {
        int i,l;
        l=strlen(&(k_input[k_scrl]));
        l=(l<COLS-2+(k_scrl==0))?l:COLS-2+(k_scrl==0);
        for (i=0;i<l;i++)
            *tbuf++='*';
    }
    else
    {
        int l,r,k;
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
                memcpy(tbuf,k_input+r,k);
                tbuf+=k;
                r+=k;
                l-=k;
                k=0;
            };
            tbuf+=sprintf(tbuf,"\e[4%dm",MARGIN_COLOR);
            k+=marginr-marginl+1;
            if (k>l)
                k=l;
            if (k>0)
            {
                memcpy(tbuf,k_input+r,k);
                tbuf+=k;
                r+=k;
                l-=k;
            };
            tbuf+=sprintf(tbuf,"\e[4%dm",INPUT_COLOR);
            if (l>0)
            {
                memcpy(tbuf,k_input+r,l);
                tbuf+=l;
            };
            k_input[k_len]=0;
        }
        else
        {
            if (k_scrl+l>k_len)
                l=k_len-k_scrl;
            memcpy(tbuf,k_input+k_scrl,l);
            tbuf+=l;
        }
    };
    if (k_scrl+COLS-2<k_len)
        tbuf+=sprintf(tbuf,"\e[1m>");
    else
        tbuf+=sprintf(tbuf,"\e[0K");
    scr_curs=(k_scrl!=0)+k_pos-k_scrl;
    redraw_cursor();
    term_commit();
}

/*************************/
/* write the status line */
/*************************/
void redraw_status(void)
{
    char *pos;
    int c,color=7;

    if (!isstatus)
        return;
    tbuf+=sprintf(tbuf,"\e[%d;1f\e[0;3%d;4%dm\e[2K\r",LINES,
                  STATUS_COLOR==COLOR_BLACK?7:0,STATUS_COLOR);
    if (!*(pos=status))
        goto end;
    for (;*pos;pos++)
    {
        if (getcolor(&pos,&color,0))
        {
            c=color;
            if (!(c&0x70))
                c=c|(STATUS_COLOR<<4);
            if ((c&15)==((c>>4)&7))
                c=(c&0xf0)|(c&7? 0:(STATUS_COLOR==COLOR_BLACK? 7:0));
            tbuf+=sprintf(tbuf,COLORCODE(c));
        }
        else
            *tbuf++=*pos;
    };
end:
    redraw_cursor();
    term_commit();
}

int b_shorten()
{
    if (b_first>b_bottom)
        return(FALSE);
    free(b_output[b_first++%B_LENGTH]);
    return(TRUE);
}

/********************************************/
/* write a single line to the output window */
/********************************************/
void draw_out(char *pos)
{
    int c=7;
    for (;*pos;pos++)
    {
        if (getcolor(&pos,&c,0))
        {
            tbuf+=sprintf(tbuf,COLORCODE(c));
            continue;
        };
        *tbuf++=*pos;
    };
}

/****************************/
/* scroll the output window */
/****************************/
void b_scroll(int b_to)
{
    int y;
    if (b_screenb==(b_to=(b_to<b_first?b_first:(b_to>b_bottom?b_bottom:b_to))))
        return;
    tbuf+=sprintf(tbuf,"\e[%d;%df\e[0;37;40m\e[1J",scr_len,COLS);
    term_commit();
    for (y=b_to-LINES+(isstatus?3:2);y<=b_to;++y)
        if ((y>=1)&&(y>b_first))
        {
            tbuf+=sprintf(tbuf,"\e[%d;1f",scr_len+y-b_to);
            if (y<b_current)
                draw_out(b_output[y%B_LENGTH]);
            else
                if (y==b_current)
                    draw_out(out_line);
                else
                    tbuf+=sprintf(tbuf,"\e[2K");
            term_commit();
        };
    tbuf+=sprintf(tbuf,"\e7");

    if (b_screenb==b_bottom)
    {
        int x;
        tbuf+=sprintf(tbuf,"\e[%d;1f\e[0;1;37;4%dm\e[2K\e[?25l",
                scr_len+1,INPUT_COLOR);
        for (x=0;x<COLS;++x)
            *tbuf++='^';
    }
    else
        tbuf+=sprintf(tbuf,"\e[?25h");
#ifdef DEBUG		
    debug_info();
#endif
    if ((b_screenb=b_to)==b_bottom)
        redraw_in();
    term_commit();
}

void b_addline(void)
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
static inline void print_char(const char ch)
{
    if (o_pos==COLS)
    {
        tbuf+=sprintf(tbuf,"\e[0;37;40m\r\n\e[2K");
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
    out_line[o_len++]=ch;
    ++o_pos;
    *tbuf++=ch;
}

void b_textout(char *txt)
{
    /* warning! terminal output can get discarded! */
    tbuf+=sprintf(tbuf,"\e8");
    for (;*txt;txt++)
        switch(*txt)
        {
        case 27:
            if (*(txt+1)=='[')
            {
                ++txt;
again:
                switch (*++txt)
                {
                case 0:
                    --txt;
                    break;
                case ';':
                    goto again;
                case 'm':
                    if (*(txt-1)=='[')
                        o_color=7;
                    break;
                case '0':
                    o_color=7;
                    goto again;
                case '1':
                    o_color|=8;
                    goto again;
                case '2':
                    o_color=(o_color&0xf0)|8;
                    goto again;
                case '3':
                    if (!*++txt)
                    {--txt;break;};
                    if ((*txt>='0')&&(*txt<'8'))
                    {
                        o_color=(o_color&0xf0)|colors[*txt-'0'];
                        goto again;
                    }
                    else
                        --txt;
                    break;
                case '4':
                    if (!*++txt)
                    {--txt;break;};
                    if ((*txt>='0')&&(*txt<'8'))
                    {
                        o_color=(o_color&0x8f)|(colors[*txt-'0']<<4);
                        goto again;
                    }
                    else
                        --txt;
                    break;
                case '5':
                    o_color=o_color|=128;
                    goto again;
                case '7':
                    o_color=(o_color&0x88)|(o_color&0x70>>4)|(o_color&7);
                    /* inverse should propagate... oh well */
                    goto again;
                default:
                    --txt;
                };
                break;
            };
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
            tbuf+=sprintf(tbuf,"\e[0;37;40m\r\n\e[2K");
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
            print_char(*txt);
        };
    out_line[o_len]=0;
    tbuf+=sprintf(tbuf,"\e7");
#ifdef DEBUG
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
void b_textout_test(char *txt)
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
            if (!isdigit(*t))
                goto nope;
            while (isdigit(*t))
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

void b_canceldraft(void)
{
    if (b_bottom==b_screenb)
    {
        tbuf+=(o_oldcolor&0x70)?
                    sprintf(tbuf,"\e8\e[0m\e[2K"):
                    sprintf(tbuf,"\e8\e[2K");
        while (b_current>b_last)
        {
            b_current--;
            tbuf+=sprintf(tbuf,"\e[A\e[2K");
        };
        tbuf+=sprintf(tbuf,"\r"COLORCODE(o_lastcolor));
        tbuf+=sprintf(tbuf,"\e7");
    }
    else
        b_current=b_last;
    o_oldcolor=o_color=o_lastcolor;
    o_prevcolor=o_lastprevcolor;
    o_len=setcolor(out_line,o_lastcolor);
    o_pos=0;
}

void textout(char *txt)
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
#ifdef DEBUG
    debug_info();
    redraw_cursor();
    term_commit();
#endif
}

void textout_draft(char *txt, int flag)
{
    if (o_draftlen)
        b_canceldraft();
    if (txt)
    {
        strcpy(b_draft,txt);
#ifdef DEBUG
        strcat(b_draft,"\376");
#endif    
        if ((o_draftlen=strlen(b_draft)))
            b_textout(b_draft);
#ifdef DEBUG
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

static int state=0;
static int val=0;
int process_kbd(struct session *ses)
{
    char ch;
    int i;
    
    if (read(0,&ch,1)!=1)
        return(0);
    switch(state)
    {
    case 4:			/* ESC O */
        state=0;
        if (b_bottom!=b_screenb)
            b_scroll(b_bottom);
                                                    
        {
            char txt[128];
            sprintf(txt,"ESCO%c",ch);
            find_bind(txt,1,ses);
        };
        break;
    case 3:			/* ESC [ [ */
        state=0;
        if (b_bottom!=b_screenb)
            b_scroll(b_bottom);
        {
            char txt[128];
            sprintf(txt,"ESC[[%c",ch);
            find_bind(txt,1,ses);
        };
        break;
    case 2:			/* ESC [ */
        state=0;
        if (isdigit(ch))
        {
            val=val*10+(ch-'0');
            state=2;
        }
        else
            if (isupper(ch))
            {
                switch(ch)
                {

                prev_history:
                case 'A':	/* up arrow */
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    if (!ses)
                        break;
                    if (hist_num==HISTORY_SIZE-1)
                        break;
                    if (!ses->history[hist_num+1])
                        break;
                    if (hist_num==-1)
                        strcpy(kh_input,k_input);
                    strcpy(k_input,ses->history[++hist_num]);
                    countpos();
                    redraw_in();
                    break;
                next_history:
                case 'B':	/* down arrow */
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    if (!ses)
                        break;
                    if (hist_num==-1)
                        break;
                    do --hist_num;
                    while (hist_num&&!(ses->history[hist_num]));
                    if (hist_num==-1)
                        strcpy(k_input,kh_input);
                    else
                        strcpy(k_input,ses->history[hist_num]);
                    countpos();
                    redraw_in();
                    break;
                key_cursor_left:
                case 'D':	/* left arrow */
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
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
                default:
                    if (b_bottom!=b_screenb)
                        b_scroll(b_bottom);
                    {
                        char txt[128];
                        sprintf(txt,"ESC[%c",ch);
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
                            if (b_bottom!=b_screenb)
                                b_scroll(b_bottom);
                            if (k_pos==k_len)
                                break;
                            for (i=k_pos;i<=k_len;++i)
                            k_input[i]=k_input[i+1];
                            --k_len;
                            redraw_in();
                            break;
                        default:
                            if (b_bottom!=b_screenb)
                                b_scroll(b_bottom);
                            {
                                char txt[128];
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
        /* [Esc] */
        state=0;
        tbuf+=sprintf(tbuf,"\e5n");
        if (b_bottom!=b_screenb)
            b_scroll(b_bottom);
        k_pos=0;
        k_scrl=0;
        k_len=0;
        k_input[0]=0;
        redraw_in();
        /* fallthrough */
    case 0:
        switch(ch)
        {
        case '\n':
        case '\r':
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (!activesession->server_echo)
                user_getpassword=0;
            strcpy(done_input,k_input);
            k_len=k_pos=k_scrl=0;
            k_input[0]=0;
            tbuf+=sprintf(tbuf,"\e[%d;1f\e[0;37;4%dm\e[2K",scr_len+1,INPUT_COLOR);
            if (margins&&(marginl<=COLS))
            {
                tbuf+=sprintf(tbuf,"\e[%d;%df\e[0;37;4%dm",
                              scr_len+1,marginl,MARGIN_COLOR);
                if (marginr<=COLS)
                    i=marginr+1-marginl;
                else
                    i=COLS+1-marginl;
                while (i--)
                    *tbuf++=' ';
                tbuf+=sprintf(tbuf,"\e[1;37;4%dm\e[%d;1f",INPUT_COLOR,scr_len+1);
            };
            scr_curs=0;
            term_commit();
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
            *done_input=0;
            parse_input("#zap",1,ses);
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
            if (k_pos==0)
                break;
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
                char buf[BUFFER_SIZE];
                strcpy(buf,k_input);
                strcpy(k_input,tk_input);
                strcpy(tk_input,buf);
                i=k_pos; k_pos=tk_pos; tk_pos=i;
                i=k_scrl; k_scrl=tk_scrl; tk_scrl=i;
                i=k_len; k_len=tk_len; tk_len=i;
            };
            redraw_in();
            break;
        case 11:		/* ^[K] */
            if (find_bind("^K",0,ses))
                break;
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos==k_len)
                break;
            memmove(yank_buffer,k_input+k_pos,k_len-k_pos);
            yank_buffer[k_len-k_pos]=0;
            k_input[k_pos]=0;
            k_len=k_pos;
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
        case 20:		/* ^[T] */
            if (find_bind("^T",0,ses))
                break;
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (!k_pos||!k_len)
                break;
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
            };
            redraw_in();
            break;
        case 21:		/* ^[U] */
            if (find_bind("^U",0,ses))
                break;
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos==0)
                break;
            memmove(yank_buffer,k_input,k_pos);
            yank_buffer[k_pos]=0;
            memmove(k_input,k_input+k_pos,k_len-k_pos+1);
            k_len-=k_pos;
            k_pos=k_scrl=0;
            redraw_in();
            break;
        case 23:		/* ^[W] */
            if (find_bind("^W",0,ses))
                break;
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_pos==0)
                break;
            i=k_pos-1;
            while((i>=0)&&isspace(k_input[i]))
                i--;
            while((i>=0)&&!isspace(k_input[i]))
                i--;
            i=k_pos-i-1;
            memmove(yank_buffer,k_input+k_pos-i,i);
            yank_buffer[i]=0;
            memmove(k_input+k_pos-i,k_input+k_pos,k_len-k_pos);
            k_len-=i;
            k_input[k_len]=0;
            k_pos-=i;
            redraw_in();
            break;
        case 25:		/* ^[Y] */
            if (find_bind("^Y",0,ses))
                break;
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (!yank_buffer[0])
            {
                write(1,"\007",1);
                break;
            }
            i=strlen(yank_buffer);
            if (i+k_len>=BUFFER_SIZE)
                i=BUFFER_SIZE-1-k_len;
            memmove(k_input+k_pos+i,k_input+k_pos,k_len-k_pos);
            memmove(k_input+k_pos,yank_buffer,i);
            k_len+=i;
            k_input[k_len]=0;
            k_pos+=i;
            redraw_in();
            break;
        case 27:        /* [Esc] or a control sequence */
            state=1;
            break;
        default:
            if (b_bottom!=b_screenb)
                b_scroll(b_bottom);
            if (k_len==BUFFER_SIZE-1)
                write(1,"\007",1);
            else
            {
                if ((ch>0)&&(ch<32))
                {
                    char txt[128];
                    sprintf(txt,"^%c",ch+64);
                    find_bind(txt,1,ses);
                    break;
                };
                k_input[++k_len]=0;
                {
                    int i;
                    for (i=k_len-1;i>k_pos;--i)
                        k_input[i]=k_input[i-1];
                };
                k_input[k_pos++]=ch;
                if ((k_len==k_pos)&&(k_len<COLS))
                {
                    scr_curs++;
                    tbuf+=sprintf(tbuf,"\e[0;37;4%dm%c",
                                  margins&&
                                  (k_len>=marginl)&&(k_len<=marginr)
                                  ? MARGIN_COLOR : INPUT_COLOR,
                                  user_getpassword?'*':ch);
                    term_commit();
                }
                else
                    redraw_in();
            }
        }
    };
#ifdef DEBUG
    debug_info();
    redraw_cursor();
    term_commit();
#endif
    return(0);
}

void user_beep(void)
{
    write(1,"\007",1);
}

/******************************/
/* set up the window outlines */
/******************************/
void user_drawscreen(void)
{
    need_resize=0;
    scr_len=LINES-1-isstatus;
    tbuf+=sprintf(tbuf,"\e[0;37;40m\e[2J\e[0;37;40m\e[1;%dr\e7",scr_len);
    tbuf+=sprintf(tbuf,"\e[%d;f\e[0;37;4%dm\e[2K",scr_len+1,INPUT_COLOR);
    if (isstatus)
        tbuf+=sprintf(tbuf,"\e[%d;f\e[37;4%dm\e[2K",LINES,STATUS_COLOR);
}

void user_keypad(int onoff)
{
    if (onoff)
        tbuf+=sprintf(tbuf,"\e=");
    else
        tbuf+=sprintf(tbuf,"\e>");
    term_commit();
}

/********************/
/* SIGWINCH handler */
/********************/
void sigwinch(void)
{
    need_resize=1;
}

/*********************/
/* resize the screen */
/*********************/
void user_resize(void)
{
    term_commit();
    term_getsize();
    user_drawscreen();
    b_screenb=-666;		/* impossible value */
    b_scroll(b_bottom);
    if (isstatus)
        redraw_status();
    redraw_in();
    
    telnet_resize_all();
}

void show_status(void)
{
    int st;
    st=!!strcmp(status,EMPTY_LINE);
    if (st!=isstatus)
    {
        isstatus=st;
        user_resize();
    }
    else
        if (st)
        {
            redraw_status();
            redraw_cursor();
            term_commit();
        }
}

void user_init(void)
{
    term_getsize();
    term_init();
    tbuf=term_buf+sprintf(term_buf,"\e[?7l");
    user_keypad(keypad);
    isstatus=0;
    user_drawscreen();

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
    user_getpassword=0;

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
    sprintf(done_input,"~12~KB~3~tin ~7~%s by ~11~kilobyte@mimuw.edu.pl~9~\n",VERSION_NUM);
    textout(done_input);
    {
        int i;
        for (i=0;i<COLS;++i)
            done_input[i]='-';
        sprintf(done_input+COLS,"~7~");
    };
    textout(done_input);
}

void user_done(void)
{
    tbuf+=sprintf(tbuf,"\e[1;%dr\e[%d;1f\e[?25h\e[?7h\e[0;37;40m",LINES,LINES);
    user_keypad(0);
    term_commit();
    term_restore();
    write(1,"\n",1);
}

void user_pause(void)
{
    user_keypad(0);
    tbuf+=sprintf(tbuf,"\e[1;%dr\e[%d;1f\e[?25h\e[?7h\e[0;37;40m",LINES,LINES);
    term_commit();
    term_restore();
}

void user_resume(void)
{
    term_getsize();
    term_init();
    tbuf=term_buf+sprintf(term_buf,"\e[?7l");
    user_keypad(keypad);
    user_drawscreen();
    b_screenb=-666;     /* impossible value */
    b_scroll(b_bottom);
    if (isstatus)
        redraw_status();
    redraw_in();
}


void fwrite_out(FILE *f,char *pos)
{
    int c=7;
    for (;*pos;pos++)
    {
        if (*pos=='~')
            if (getcolor(&pos,&c,0))
            {
                fprintf(f,COLORCODE(c));
                continue;
            };
        if (*pos!='\n')
            fprintf(f,"%c",*pos);
    }
}

void user_condump(FILE *f)
{
    int i;
    for (i=b_first;i<b_current;i++)
    {
        fwrite_out(f,b_output[i%B_LENGTH]);
        fprintf(f,"\n");
    };
    fwrite_out(f,out_line);
}

void user_passwd(int x)
{
    user_getpassword=x;
    redraw_in();
}
