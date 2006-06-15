/* $Id: utils.c,v 2.1 1998/10/11 18:36:36 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: utils.c - some utility-functions                            */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include "tintin.h"
#include <stdarg.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

extern void user_done(void);
void syserr(char *msg, ...);

/*********************************************/
/* return: TRUE if s1 is an abrevation of s2 */
/*********************************************/
int is_abrev(char *s1, char *s2)
{
    return (!strncmp(s2, s1, strlen(s1)));
}

/********************************/
/* strdup - duplicates a string */
/* return: address of duplicate */
/********************************/
char *mystrdup(char *s)
{
    char *dup;

    if ((dup = (char *)malloc(strlen(s) + 1)) == NULL)
        syserr("Not enought memory for strdup.");
    strcpy(dup, s);
    return dup;
}


/*************************************************/
/* print system call error message and terminate */
/*************************************************/
void syserr(char *msg, ...)
{
    va_list ap;
#ifdef UI_FULLSCREEN
    user_done();
#endif
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
