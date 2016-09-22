/*********************************************************************/
/* file: utils.c - some utility-functions                            */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "tintin.h"
#include "protos/user.h"

void syserr(const char *msg, ...);

/*********************************************/
/* return: true if s1 is an abrevation of s2 */
/*********************************************/
bool is_abrev(const char *s1, const char *s2)
{
    return !strncmp(s2, s1, strlen(s1));
}

/********************************/
/* strdup - duplicates a string */
/* return: address of duplicate */
/********************************/
char* mystrdup(const char *s)
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
void syserr(const char *msg, ...)
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

#ifndef HAVE_STRLCPY
/* not for protos.h */ size_t strlcpy(char *dst, const char *src, size_t n)
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
