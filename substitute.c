/* $Id: substitute.c,v 2.2 1998/10/11 18:36:36 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: substitute.c - functions related to the substitute command  */
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

extern char *get_arg_in_braces();
extern struct listnode *search_node_with_wild();
extern struct listnode *searchnode_list();

extern struct listnode *common_subs;
extern char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */
extern int subnum;
extern int mesvar[7];

/***************************/
/* the #substitute command */
/***************************/
void parse_sub(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *mysubs, *ln;

  mysubs = (ses) ? ses->subs : common_subs;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);

  if (!*left) {
    tintin_puts2("#THESE SUBSTITUTES HAVE BEEN DEFINED:", ses);
    show_list(mysubs);
    prompt(ses);
  } else if (*left && !*right) {
    if ((ln = search_node_with_wild(mysubs, left)) != NULL) {
      while ((mysubs = search_node_with_wild(mysubs, left)) != NULL) {
	shownode_list(mysubs);
      }
      prompt(ses);
    } else if (mesvar[2])
      tintin_puts2("#THAT SUBSTITUTE IS NOT DEFINED.", ses);
  } else {
    if ((ln = searchnode_list(mysubs, left)) != NULL)
      deletenode_list(mysubs, ln);
    insertnode_list(mysubs, left, right, "0", ALPHA);
    subnum++;
    if (strcmp(right, ".") != 0)
      sprintf(result, "#Ok. {%s} now replaces {%s}.", right, left);
    else
      sprintf(result, "#Ok. {%s} is now gagged.", left);
    if (mesvar[2])
      tintin_puts2(result, ses);
  }
}


/*****************************/
/* the #unsubstitute command */
/*****************************/

void unsubstitute_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *mysubs, *ln, *temp;
  int flag;

  flag = FALSE;
  mysubs = (ses) ? ses->subs : common_subs;
  temp = mysubs;
  arg = get_arg_in_braces(arg, left, 1);
  while ((ln = search_node_with_wild(temp, left)) != NULL) {
    if (mesvar[2]) {
      if (*(ln->right) == '.' && !*(ln->right + 1))
	sprintf(result, "#Ok. {%s} is no longer gagged.", ln->left);
      else
	sprintf(result, "#Ok. {%s} is no longer substituted.", ln->left);
      tintin_puts2(result, ses);
    }
    deletenode_list(mysubs, ln);
    flag = TRUE;
    /*  temp=ln; */
  }
  if (!flag && mesvar[2])
    tintin_puts2("#THAT SUBSTITUTE IS NOT DEFINED.", ses);
}


void do_all_sub(line, ses)
     char *line;
     struct session *ses;
{
  struct listnode *ln;

  ln = ses->subs;

  while ((ln = ln->next))
    if (check_one_action(line, ln->left, ses))
	prepare_actionalias(ln->right, line, ses);
}
