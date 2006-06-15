/* $Id: help.c,v 1.4 1998/11/25 17:14:00 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*****************************************************************/
/* functions for the #help command                               */
/* Some small patches by David Hedbor (neotron@lysator.liu.se)   */
/* to make it work better.                                       */
/*****************************************************************/
#include "config.h"
#include <stdlib.h>
#include <ctype.h>
#include "tintin.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

extern char tintin_char;
extern int is_abrev(char *s1, char *s2);
extern void check_all_promptactions(char *line, struct session *ses);
extern void prompt(struct session *ses);
extern char *tintin_exec;
extern void tintin_printf(struct session *ses, const char *format, ...);
extern void tintin_eprintf(struct session *ses, const char *format, ...);
extern FILE *mypopen(char *command);

FILE *check_file(char *filestring)
{
#if COMPRESSED_HELP
    char sysfile[BUFFER_SIZE];
    FILE *f;

    sprintf(sysfile, "%s%s", filestring, DEFAULT_COMPRESSION_EXT);
    if ((f=fopen(sysfile, "r")))
        fclose(f);
    else
        return 0;
    sprintf(sysfile, "%s %s%s", DEFAULT_EXPANSION_STR, filestring,
        DEFAULT_COMPRESSION_EXT);
    return mypopen(sysfile);
#else
    return (FILE *) fopen(filestring, "r");
#endif
}

void help_command(char *arg,struct session *ses)
{
    FILE *myfile=NULL;
    char text[BUFFER_SIZE], line[BUFFER_SIZE], filestring[BUFFER_SIZE];
    int flag;

    flag = TRUE;
    if (strcmp(DEFAULT_FILE_DIR, "HOME"))
    {
        sprintf(filestring, "%s/KBtin_help", DEFAULT_FILE_DIR);
        myfile = check_file(filestring);
    }
#ifdef DATA_PATH
    if (myfile == NULL)
    {
        sprintf(filestring, "%s/KBtin_help", DATA_PATH);
        myfile = check_file(filestring);
    }
#endif
    if (myfile == NULL)
    {
        sprintf(filestring, "%s_help", tintin_exec);
        myfile = check_file(filestring);
    }
    if (myfile == NULL)
    {
        sprintf(filestring, "%s/KBtin_help", getenv("HOME"));
        myfile = check_file(filestring);
    }
    if (myfile == NULL)
    {
        tintin_eprintf(0, "#Help file not found - no help available.");
        tintin_eprintf(0, "#Locations checked:");
        if (strcmp(DEFAULT_FILE_DIR, "HOME"))
            tintin_eprintf(0, "#      %s/KBtin_help%s", DEFAULT_FILE_DIR,
                DEFAULT_COMPRESSION_EXT);
#ifdef DATA_PATH
        tintin_eprintf(0, "#      %s/KBtin_help%s", DATA_PATH,
            DEFAULT_COMPRESSION_EXT);
#endif
        tintin_eprintf(0, "#      %s_help%s",tintin_exec,
            DEFAULT_COMPRESSION_EXT);
        tintin_eprintf(0, "#      %s/KBtin_help%s", getenv("HOME"),
            DEFAULT_COMPRESSION_EXT);
        prompt(NULL);
        return;
    }
    if (*arg==tintin_char)
        arg++;
    if (*arg)
    {
        sprintf(text, "~%s", arg);
        while (flag)
        {
            fgets(line, sizeof(line), myfile);
            if (*line == '~')
            {
                if (*(line + 1) == '*')
                {
                    tintin_printf(0,"#Sorry, no help on that word.");
                    flag = FALSE;
                }
                else if (is_abrev(text, line))
                {
                    while (flag)
                    {
                        fgets(line, sizeof(line), myfile);
                        if ((*line == '~')&&(*(line+1)=='~'))
                            flag = FALSE;
                        else
                        {
                            *(line + strlen(line) - 1) = '\0';
                            if (*line!='~')
                                tintin_printf(0,"%s",line);
                        }
                    }
                }
            }
        }
    }
    else
    {
        while (flag)
        {
            fgets(line, sizeof(line), myfile);
            if ((*line == '~')&&(*(line+1)=='~'))
                flag = FALSE;
            else
            {
                *(line + strlen(line) - 1) = '\0';
                if (*line!='~')
                    tintin_printf(0,"%s",line);
            }
        }
    }
    prompt(NULL);
    fclose(myfile);
}
