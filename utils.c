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
#include "ui.h"

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
char* mystrdup(char *s)
{
    char *dup;

    if ((dup = (char *)malloc(strlen(s) + 1)) == NULL)
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
