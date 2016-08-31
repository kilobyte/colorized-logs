/*
 * match -- returns 1 if `string' satisfised `regex' and 0 otherwise
 * stolen from Spencer Sun: only recognizes * and \ as special characters
 */

#include "tintin.h"
#include <assert.h>

int match(const char *regex, const char *string)
{
    const char *rp = regex, *sp = string, *save;
    char ch;

    while (*rp != '\0')
    {
        switch (ch = *rp++)
        {
        case '*':
            if ('\0' == *sp)           /* match empty string at end of `string' */
                return '\0' == *rp;    /* but only if we're done with the pattern */
            /* greedy algorithm: save starting location, then find end of string */
            save = sp;
            sp += strlen(sp);
            do
            {
                if (match(rp, sp))     /* return success if we can match here */
                    return 1;
            } while (--sp >= save);    /* otherwise back up and try again */
            /*
             * Backed up all the way to starting location (i.e. `*' matches
             * empty string) and we _still_ can't match here.  Give up.
             */
            return 0;
            /* break; not reached */
        case '\\':
            if ((ch = *rp++) != '\0')
            {
                /* if not end of pattern, match next char explicitly */
                if (ch != *sp++)
                    return 0;
                break;
            }
            /* else FALL THROUGH to match a backslash */
        default:                       /* normal character */
            if (ch != *sp++)
                return 0;
            break;
        }
    }
    /*
     * OK, we successfully matched the pattern if we got here.  Now return
     * a match if we also reached end of string, otherwise failure
     */
    return ('\0' == *sp);
}


int is_literal(const char *txt)
{
    return !strchr(txt, '*');
}


int find(const char *text, const char *pattern, int *from, int *to, const char *fastener)
{
    const char *txt;
    char *a, *b, *pat, m1[BUFFER_SIZE], m2[BUFFER_SIZE];
    int i;

    if (fastener)
    {
        txt=strstr(text,fastener);
        if (!txt)
            return 0;
        *from=txt-text;
        if (strchr(pattern, '*'))
            *to=strlen(text)-1;
        else
            *to=*from+strlen(fastener)-1;
        return 1;
    }

    txt=text;
    if (*pattern=='^')
    {
        for (pattern++;(*pattern)&&(*pattern!='*');)
            if (*(pattern++)!=*(txt++))
                return 0;
        if (!*pattern)
        {
            *from=0;
            *to=txt-text-1;
            return 1;
        };
        strcpy(m1,pattern);
        pat=m1;
        goto start;
    };
    if (!(b=strchr(pattern,'*')))
    {
        a=strstr(txt,pattern);
        if (a)
        {
            *from=a-text;
            *to=*from+strlen(pattern)-1;
            return 1;
        }
        else
            return 0;
    };
    i=b-pattern;
    strcpy(m1,pattern);
    m1[i]=0;
    pat=m1;
    txt=strstr(txt,pat);
    if (!txt)
    {
        return 0;
    };
    *from=txt-text;
    txt+=i;
    pat+=i+1;
    while (*pat=='*')
        pat++;
start:
    i=strlen(pat);
    if (!*pat)
    {
        *to=strlen(text)-1;
        return 1;
    };
    a=pat;
    b=pat+i-1;
    while (a<b)
    {
        char c=*a;
        *a++=*b;
        *b--=c;
    };
    i=strlen(txt);
    for (a=m2+i-1;*txt;)
        *a--=*txt++;
    m2[i]=0;
    txt=m2;
    *to=-1;
    do
    {
        b=strchr(pat,'*');
        if (b)
            *b=0;
        a=strstr(txt,pat);
        if (!a)
            return 0;
        if (*to==-1)
            *to=strlen(text)-(a-txt)-1;
        txt=a+strlen(pat);
        if (b)
            pat=b;
        else
            pat=strchr(pat,0);
    } while (*pat);
    return 1;
}


char* get_fastener(const char *txt, char *mbl)
{
    const char *m;

    if (*txt=='^')
        return 0;
    if (*txt=='*')
        return 0;
    m=txt;
    while (*m && *m!='*')
        m++;
    if (*m=='*')
    {
        if (*(m+1))
            return 0;
    }
    else
    {
        if (*m)
            return 0;
    }
    assert(m-txt<BUFFER_SIZE);
    memcpy(mbl, txt, m-txt);
    *(mbl+(m-txt))=0;
    return mbl;
}
