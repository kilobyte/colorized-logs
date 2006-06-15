/* $Id: text.c,v 2.2 1998/10/11 18:36:36 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: text.c  - funtions for logfile and reading/writing comfiles */
/*                             TINTIN + +                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*                    New code by Joann Ellsworth                    */
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

extern struct session *parse_input();
extern char *get_arg_in_braces();
extern int puts_echoing;
extern int verbose;
extern char tintin_char;
extern int verbatim;
extern void verbatim_command();

/**********************************/
/* load a file for input to mud.  */
/**********************************/
void read_file(arg, ses)
     char *arg;
     struct session *ses;
{
  FILE *myfile;
  char buffer[BUFFER_SIZE], *cptr;

  get_arg_in_braces(arg, arg, 1);
  if (ses == NULL) {
    tintin_puts("You can't read any text in without a session being active.", NULL);
    prompt(NULL);
    return;
  }
  if ((myfile = fopen(arg, "r")) == NULL) {
    tintin_puts("ERROR: No file exists under that name.\n", (struct session *)NULL);
    prompt(NULL);
    return;
  }
  while (fgets(buffer, sizeof(buffer), myfile)) {
    for (cptr = buffer; *cptr && *cptr != '\n'; cptr++) ;
    *cptr = '\0';
    write_line_mud(buffer, ses);
  }
  fclose(myfile);
  tintin_puts("File read - Success.\n", (struct session *)NULL);
  prompt(NULL);
  tintin_puts("\n", (struct session *)NULL);

}
