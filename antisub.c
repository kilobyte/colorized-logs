/* $Id: antisub.c,v 1.3 1998/10/11 18:36:34 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: antisub.c - functions related to the substitute command     */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include "tintin.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

char *get_arg_in_braces();
struct listnode *searchnode_list();
struct listnode *search_node_with_wild();

extern struct listnode *common_antisubs;
extern char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */
extern int antisubnum;
extern int mesvar[7];

/***************************/
/* the #substitute command */
/***************************/
void parse_antisub(arg, ses)
     char *arg;
     struct session *ses;
{
  /* char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE]; */
  char left[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *myantisubs, *ln;

  myantisubs = (ses) ? ses->antisubs : common_antisubs;
  arg = get_arg_in_braces(arg, left, 1);

  if (!*left) {
    tintin_puts("#THESE ANTISUBSTITUTES HAS BEEN DEFINED:", ses);
    show_list(myantisubs);
    prompt(ses);
  } else {

    if ((ln = searchnode_list(myantisubs, left)) != NULL)
      deletenode_list(myantisubs, ln);
    insertnode_list(myantisubs, left, left, "0", ALPHA);
    antisubnum++;
    if (mesvar[3]) {
      sprintf(result, "Ok. Any line with {%s} will not be subbed.", left);
      tintin_puts2(result, ses);
    }
  }
}


void unantisubstitute_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *myantisubs, *ln, *temp;
  int flag;

  flag = FALSE;
  myantisubs = (ses) ? ses->antisubs : common_antisubs;
  temp = myantisubs;
  arg = get_arg_in_braces(arg, left, 1);
  while ((ln = search_node_with_wild(temp, left)) != NULL) {
    if (mesvar[3]) {
      sprintf(result,"#Ok. Lines with {%s} will now be subbed.", ln->left);
      tintin_puts2(result, ses);
    }
    deletenode_list(myantisubs, ln);
    flag = TRUE;
    /* temp=ln; */
  }
  if (!flag && mesvar[3])
    tintin_puts2("#THAT ANTISUBSTITUTE IS NOT DEFINED.", ses);
}




int do_one_antisub(line, ses)
     char *line;
     struct session *ses;
{
  struct listnode *ln;

  ln = ses->antisubs;

  while ((ln = ln->next))
    if (check_one_action(line, ln->left, ses))
      return TRUE;
  return FALSE;
}
