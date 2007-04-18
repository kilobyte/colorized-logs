#include <string.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <errno.h>
#include "config.h"
#include "tintin.h"
#include "translit.h"

#ifdef UTF8

#ifdef PROFILING
extern char *prof_area;
#endif
extern void syserr(char *msg, ...);

int user_charset;
char *user_charset_name;


/* get the length of an UTF-8 string */
int utf8_len(char *s)
{
    int l=0;
    
    while(*s)
        if ((*s++&0xc0)!=0x80)
            l++;
    return l;
}

/* copy at most n UTF-8 characters, using no more than maxb bytes */
/* return # of wide chars copied */
int utf8_ncpy(char *d, char *s, int n, int maxb)
{
    int nc=0;

    if (--maxb<0)
        return 0;
    while(*s && maxb)
    {
        if ((*s&0xc0)!=0x80)
            if (nc++>=n)
            {
                nc--;
                break;
            }
        *d++=*s++;
    }
    *d=0;
    return nc;
}

/* skip first n UTF-8 characters */
char* utf8_seek(char *s, int n)
{
    if (n<=0)
        return s;
    for(; *s; s++)
        if ((*s&0xc0)!=0x80)
            if (!n--)
                return s;
    return s;
}

/* copy up to n (no check for 0, -1 = inf) wide chars, return n of bytes consumed */
/* d must be able to hold at least n+1 WChars */
int utf8_to_wc(wchar_t *d, char *s, int n)
{
    char *s0;
    unsigned char ic;
    int tc,c,cnt,surrogate;
    
    
#define OUTC(x,y) \
        {		\
            *d++=(x);	\
            if (!--n)	\
            {		\
                y;	\
                break;	\
            }		\
        }

    s0=s;
    cnt=surrogate=tc=0;
    for(;(ic=*s);s++)
    {
        if (ic>0x7f)
            if (cnt>0 && (ic&0xc0)==0x80)
            {
                tc=(tc<<6) | (ic&0x3f);
                if (!--cnt)
                    c=tc;
                else
                    continue;
                
                if (c<0xA0)	/* illegal */
                    c=0xFFFD;
                if (c==0xFFEF)  /* BOM */
                    continue;

                /* The following code deals with malformed UTF-16 surrogates
                 * encoded in UTF-8 text.  While the standard explicitely
                 * forbids this, some (usually Windows and/or Java) programs
                 * generate them, and thus we'll better support such
                 * encapsulation anyway.  We don't go out of our way to detect
                 * unpaired surrogates, though.
                 */
                if (c>=0xD800 && c<=0xDFFF)     /* UTF-16 surrogates */
                {
                    if (c<0xDC00)       /* lead surrogate */
                    {
                        surrogate=c;
                        continue;
                    }
                    else                /* trailing surrogate */
                    {
                        c=(surrogate<<10)+c+
                            (0x10000 - (0xD800 << 10) - 0xDC00);
                        surrogate=0;
                        if (c<0x10000)  /* malformed pair */
                            c=0xFFFD;
                    }
                }
            }
            else
            {
                if (cnt>0)
                    OUTC(0xFFFD,); /* previous sequence was incomplete */
                if ((ic&0xe0)==0xc0)
                    cnt=1, tc=ic&0x1f;
                else if ((ic&0xf0)==0xe0)
                    cnt=2, tc=ic&0x0f;
                else if ((ic&0xf8)==0xf0)
                    cnt=3, tc=ic&0x07;
                else if ((ic&0xfc)==0xf8)
                    cnt=4, tc=ic&0x03;
                else if ((ic&0xfe)==0xfc)
                    cnt=5, tc=ic&0x01;
                else
                {
                    OUTC(0xFFFD,);
                    cnt=0;
                }
                continue;
            }
        else
        {
            if (cnt>0)
                OUTC(0xFFFD,); /* previous sequence was incomplete */
            cnt=0, c=ic;
        }
        OUTC(c, s++);
    }
    if (cnt && n)
        *d++=0xFFFD;
    *d=0;
    return s-s0;
}
#undef OUTC

/* returns # of bytes stored (not counting \0) */
int wc_to_utf8(char *d, const wchar_t *s, int n, int maxb)
{
    char *maxd, *d0;
    
    d0=d;
    maxd=d+maxb-8;
#define uv ((unsigned int)(*s))
#define vb d
    for(;n-- && *s && d<maxd;s++)
    {
        if (uv<0x80)
        {
            *vb++=uv;
            continue;
        }
        if (uv < 0x800) {
            *vb++ = ( uv >>  6)         | 0xc0;
            *vb++ = ( uv        & 0x3f) | 0x80;
            continue;
        }
#ifdef WCHAR_IS_UCS4
        if (uv < 0x10000) {
#endif
            *vb++ = ( uv >> 12)         | 0xe0;
            *vb++ = ((uv >>  6) & 0x3f) | 0x80;
            *vb++ = ( uv        & 0x3f) | 0x80;
#ifdef WCHAR_IS_UCS4
            continue;
        }
        if (uv < 0x110000) {
            *vb++ = ( uv >> 18)         | 0xf0;
            *vb++ = ((uv >> 12) & 0x3f) | 0x80;
            *vb++ = ((uv >>  6) & 0x3f) | 0x80;
            *vb++ = ( uv        & 0x3f) | 0x80;
            continue;
        }
        *vb++=0xef;
        *vb++=0xbf;
        *vb++=0xbd;
#endif
    }
#undef uv
#undef vb
    *d=0;
    return d-d0;
}

char translit(wchar_t ch)
{
    if (ch<TRANSLIT_MIN || ch>TRANSLIT_MAX)
        return BAD_CHAR;
    return tlits[ch-TRANSLIT_MIN];
}

int one_utf8_to_mb(char **d, char **s, mbstate_t *cs)
{
    wchar_t u[2];
    int len,len2;
    
    len=utf8_to_wc(u, *s, 1);
    if (!len)
        return 0;
    
    *s+=len;
    len2=wcrtomb(*d, u[0], cs);
    if (len2!=-1)
        *d+=len2;
    else
        *(*d)++=translit(u[0]);
    return len;
}

void utf8_to_mb(char **d, char *s, mbstate_t *cs)
{
    while(*s && one_utf8_to_mb(d, &s, cs));
}

int wc_to_mb(char *d, wchar_t *s, int n, mbstate_t *cs)
{
    int res, len=0;
    
    while(*s && n--)
    {
        res=wcrtomb(d, *s++, cs);

        if (res!=-1)
            d+=res, len+=res;
        else
            *d++=translit(s[-1]), len++;
    }
    return len;
}


/* do an entire buffer at once */
void utf8_to_local(char *d, char *s)
{
    mbstate_t cs;
    
    PROFPUSH("conv: utf8->local");
    memset(&cs, 0, sizeof(cs));
    utf8_to_mb(&d, s, &cs);
    *d=0;
    PROFPOP;
}

void local_to_utf8(char *d, char *s, int maxb, mbstate_t *cs)
{
    mbstate_t cs0;
    int len,n;
    wchar_t c;
    
    PROFPUSH("conv: local->utf8");
    if (!cs)
    {
        memset(&cs0, 0, sizeof(cs0));
        cs=&cs0;
    }
    len=strlen(s);
    while(len && maxb>10)
    {
        switch(n=mbrtowc(&c, s, len, cs))
        {
        case -2: /* truncated last character */
            *d++=BAD_CHAR;
        case 0:
            goto out;
        case -1:
            *d++=BAD_CHAR;
            s++;
            maxb--;
            len--;
            break;
        default:
            s+=n;
            len-=n;
            maxb-=n;
            d+=wc_to_utf8(d, &c, 1, BUFFER_SIZE);
        }
    }
out:
    *d=0;
    PROFPOP;
}



void init_locale()
{
    setlocale(LC_CTYPE, "");
    user_charset_name=nl_langinfo(CODESET);
    if (!strcmp(user_charset_name, "UTF-8"))
        user_charset=0;
}

int new_conv(struct charset_conv *conv, char *name, int dir)
{
    memset(conv, 0, sizeof(struct charset_conv));
    conv->name=name;
    conv->dir=dir;
    if (!strcasecmp(name, "UTF-8") || !strcasecmp(name, "UTF8"))
        conv->mode=1;
    else if (!strcasecmp(name, "ANSI_X3.4-1968")
          || !strcasecmp(name, "ISO-8859-1")
          || !strcasecmp(name, "ISO8859-1"))
        conv->mode=0;
    else if (!strcasecmp(name, "ASCII"))
        conv->mode=3;
    else
    {
        if ((dir<=0 && (conv->i_in=iconv_open("UTF-8", name))==(iconv_t)-1) ||
            (dir>=0 && (conv->i_out=iconv_open(name, "UTF-8"))==(iconv_t)-1))
            return 0;
        conv->mode=2;
    }
    return 1;
}

void nullify_conv(struct charset_conv *conv)
{
    memset(conv, 0, sizeof(struct charset_conv));
}

void cleanup_conv(struct charset_conv *conv)
{
    if (conv->mode==2)
    {
        if (conv->dir<=0)
            iconv_close(conv->i_in);
        if (conv->dir>=0)
            iconv_close(conv->i_out);
    }
    conv->mode=-1;
}

void convert(struct charset_conv *conv, char *outbuf, char *inbuf, int dir)
{
    wchar_t wbuf[BUFFER_SIZE], *wptr;
    size_t il,ol;
    int cl;

#ifndef NDEBUG
    if (dir==-1)
    {
        if (conv->dir>0)
            syserr("trying to do input on an output-only conversion");
    }
    else if (dir==1)
    {
        if (conv->dir<0)
            syserr("trying to do output on an input-only conversion");
    }
    else
        syserr("invalid conversion direction");
#endif
    
    switch(conv->mode)
    {
    case 3:		/* ASCII => UTF-8 */
        if (dir<0)
        {
            while(*inbuf)
                if ((unsigned char)*inbuf>=127)
                    *outbuf++=BAD_CHAR, inbuf++;
                else
                    *outbuf++=*inbuf++;
            *outbuf=0;
            return;
        }
        else		/* UTF-8 => ASCII */
        {
            while(*inbuf)
                if ((unsigned char)*inbuf>=127)
                    *outbuf++=translit((unsigned char)*inbuf++);
                else
                    *outbuf++=*inbuf++;
            *outbuf=0;
            return;
        }
    case 0:
        if (dir<0)	/* ISO-8859-1 => UTF-8 */
        {
            wptr=wbuf;
            while(*inbuf)
                if ((unsigned char)*inbuf>=127 && (unsigned char)*inbuf<0xA0)
                    *wptr++=0xFFFD, inbuf++;
                else
                    *wptr++=(unsigned char)*inbuf++;
            outbuf+=wc_to_utf8(outbuf, wbuf, wptr-wbuf, BUFFER_SIZE);
            return;
        }
        else		/* UTF-8 => ISO-8859-1 */
        {
            utf8_to_wc(wbuf, inbuf, BUFFER_SIZE-1);
            wptr=wbuf;
            while(*wptr)
            {
                if (*wptr>0xFF)
                    *outbuf++=translit(*wptr++);
                else
                    *outbuf++=*wptr++;
            }
            *outbuf=0;
            return;
        }
    case 1:	/* UTF-8 => UTF-8 */
        if (dir<0)	/* input: sanitize it */
        {
            utf8_to_wc(wbuf, inbuf, BUFFER_SIZE-1);
            outbuf+=wc_to_utf8(outbuf, wbuf, -1, BUFFER_SIZE);
            return;
        }
        else		/* output: trust ourself */
        {
            while(*inbuf)
                *outbuf++=*inbuf++;
            *outbuf++=0;
        }
        return;
    case 2:
        il=strlen(inbuf);
        ol=BUFFER_SIZE-1;
        while(il>0)
        {
            if (iconv((dir<0) ? conv->i_in : conv->i_out,
                &inbuf, &il, &outbuf, &ol))
            {
                if (errno==E2BIG)
                    break;
                if (dir<0)
                {
                    *outbuf++=translit((unsigned char)*inbuf++);
                    --il;
                    --ol;
                }
                else
                {
                    cl=utf8_to_wc(wbuf, inbuf, 1);
                    inbuf+=cl;
                    *outbuf++=translit(wbuf[0]);
                    il-=cl;
                    ol++;
                }
            }
        }
        *outbuf=0;
        return;
    default:
        syserr("unknown conversion mode");
    }
}
#endif
