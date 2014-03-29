/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: utils.c - some utility-functions                            */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "ui.h"

void syserr(char *msg, ...);

/*********************************************/
/* return: TRUE if s1 is an abrevation of s2 */
/*********************************************/
int is_abrev(char *s1, char *s2)
{
    return !strncmp(s2, s1, strlen(s1));
}

/********************************/
/* strdup - duplicates a string */
/* return: address of duplicate */
/********************************/
char* mystrdup(char *s)
{
    char *dup;

    if (!s)
        return 0;
    if ((dup = MALLOC(strlen(s) + 1)) == NULL)
        syserr("Not enough memory for strdup.");
    strcpy(dup, s);
    return dup;
}


/*************************************************/
/* print system call error message and terminate */
/*************************************************/
void syserr(char *msg, ...)
{
    va_list ap;

    if (ui_own_output)
        user_done();

    if (errno)
        fprintf(stderr, "ERROR (%s):  ", strerror(errno));
    else
        fprintf(stderr, "ERROR:  ");
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

/* Is any compiler _that_ old still alive? */
#ifndef HAVE_SNPRINTF
/* not for protos.h */ int snprintf(char *str, int len, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(str, fmt, ap);
    va_end(ap);
}
#endif

#ifndef HAVE_VSNPRINTF
/* not for protos.h */ int vsnprintf(char *str, int len, char *fmt, va_list ap);
{
    vsprintf(str, fmt, ap);
}
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t n)
{
    if (!n)
        return strlen(src);

    const char *s = src;

    while (--n > 0)
        if (!(*dst++ = *s++))
            break;

    if (!n)
    {
        *dst++ = 0;
        while (*s++)
            ;
    }

    return s - src - 1;
}
#endif
