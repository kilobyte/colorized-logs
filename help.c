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

FILE *check_file(char *filestring)
{
#if COMPRESSED_HELP
  char sysfile[BUFFER_SIZE];

  sprintf(sysfile, "%s %s", DEFAULT_EXPANSION_STR, filestring);
  if (fopen(filestring, "r"))
    return (FILE *) popen(sysfile, "r");
  return NULL;
#else
  return (FILE *) fopen(filestring, "r");
#endif
}

void help_command(arg)
     char *arg;
{
  FILE *myfile=NULL;
  char *cptr, text[80], line[80], filestring[500];
  int flag, counter;

  flag = TRUE;
  if (strcmp(DEFAULT_FILE_DIR, "HOME")) {
    sprintf(filestring, "%s/.tt_help.txt", DEFAULT_FILE_DIR);
#if COMPRESSED_HELP
    strcat(filestring, DEFAULT_COMPRESSION_EXT);
#endif
    myfile = check_file(filestring);
  }
  if (myfile == NULL) {
    sprintf(filestring, "%s/.tt_help.txt", getenv("HOME"));
#if COMPRESSED_HELP
    strcat(filestring, DEFAULT_COMPRESSION_EXT);
#endif
    if ((myfile = check_file(filestring)) == NULL) {
      char err[1000];

      sprintf(err, "#Help file '%s' not found - no help available.", filestring);
      tintin_puts2(err, NULL);
      prompt(NULL);
      return;
    }
  }
  if (*arg) {
    sprintf(text, "~%s", arg);
    cptr = text;

    while (*++cptr) {
      *cptr = toupper(*cptr);
    }
    while (flag) {
      fgets(line, sizeof(line), myfile);
      if (*line == '~') {
	if (*(line + 1) == '*') {
	  tintin_puts2("#Sorry, no help on that word.", (struct session *)NULL);
	  flag = FALSE;
	} else if (is_abrev(text, line)) {
	  counter = 0;
	  while (flag) {
	    fgets(line, sizeof(line), myfile);
	    if (*line == '~')
	      flag = FALSE;
	    else {
	      *(line + strlen(line) - 1) = '\0';
	      tintin_puts2(line, (struct session *)NULL);
	    }
	    if (flag && (counter++ > 23)) {
	      tintin_puts2("[ -- Press a key to continue -- ]",
			   (struct session *)NULL);
	      getchar();
	      counter = 0;
	    }
	  }
	}
      }
    }
  } else {
    while (flag) {
      fgets(line, sizeof(line), myfile);
      if (*line == '~')
	flag = FALSE;
      else {
	*(line + strlen(line) - 1) = '\0';
	tintin_puts2(line, (struct session *)NULL);
      }
    }
  }
  prompt(NULL);
#if COMPRESSED_HELP
  pclose(myfile);
#else
  fclose(myfile);
#endif
}
