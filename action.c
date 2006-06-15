/* $Id: action.c,v 1.4 1998/10/11 18:51:51 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: action.c - funtions related to the action command           */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "config.h"
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include "tintin.h"

extern struct session *activesession;
extern struct listnode *common_actions;
extern char vars[10][BUFFER_SIZE];	/* the &0, %1, %2,....%9 variables */
extern int term_echoing;
extern int Echo;
extern char tintin_char;
extern int acnum;
extern int mesvar[6];
extern char *get_arg_in_braces();
extern struct listnode *search_node_with_wild();
extern struct listnode *searchnode_list();
void substitute_vars();
int var_len[10];
char *var_ptr[10];

#define K_ACTION_MAGIC "#X~4~~2~~12~[This action is being deleted!]~7~X"
int inActions=0;
int deletedActions=0;

void kill_action(struct listnode *head,struct listnode *nptr)
{
	if (inActions)
	{
		free(nptr->left);
		nptr->left=(char*)malloc(strlen(K_ACTION_MAGIC)+1);
		strcpy(nptr->left,K_ACTION_MAGIC);
		deletedActions++;
	}
	else
	    deletenode_list(head,nptr);
}

/***********************/
/* the #action command */
/***********************/

/*  Priority code added by Joann Ellsworth 2/2/94 */

void action_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
  char pr[BUFFER_SIZE];
  struct listnode *myactions, *ln;

  myactions = (ses) ? ses->actions : common_actions;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  arg = get_arg_in_braces(arg, pr, 1);
  if (!*pr)
    strcpy(pr, "5");		/* defaults priority to 5 if no value given */
  if (!*left) {
    tintin_puts2("#Defined actions:", ses);
    show_list_action(myactions);
    prompt(ses);
  } else if (*left && !*right) {
    if ((ln = search_node_with_wild(myactions, left)) != NULL) {
      while ((myactions = search_node_with_wild(myactions, left)) != NULL) {
	shownode_list_action(myactions);
      }
      prompt(ses);
    } else if (mesvar[1])
      tintin_puts("#That action is not defined.", ses);
  } else {
    if ((ln = searchnode_list(myactions, left)) != NULL)
      kill_action(myactions, ln);
    insertnode_list(myactions, left, right, pr, PRIORITY);
    if (mesvar[1]) {
      sprintf(result,"#Ok. {%s} now triggers {%s} @ {%s}", left, right, pr);
      tintin_puts2(result, ses);
    }
    acnum++;
  }
}

/*************************/
/* the #unaction command */
/*************************/
void unaction_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *myactions, *ln, *temp;
  int flag;

  flag = FALSE;
  myactions = (ses) ? ses->actions : common_actions;
  temp = myactions;
  arg = get_arg_in_braces(arg, left, 1);
  while ((ln = search_node_with_wild(temp, left)) != NULL) {
    if (mesvar[1]) {
      sprintf(result,"#Ok. {%s} is no longer a trigger.", ln->left);
      tintin_puts2(result, ses);
    }
    kill_action(myactions, ln);
    flag = TRUE;
    /* temp=ln;*/
  }
  if (!flag && mesvar[1]) {
    sprintf(result, "#No match(es) found for {%s}", left);
    tintin_puts2(result, ses);
  }
}

/**************************************************************************/
/* run throught each of the commands on the right side of an alias/action */
/* expression, call substitute_text() for all commands but #alias/#action */
/**************************************************************************/
void prepare_actionalias(string, result, ses)
     char *string;
     char *result;
     struct session *ses;
{

  char arg[BUFFER_SIZE];

  *result = '\0';
  substitute_vars(string, arg);
  substitute_myvars(arg, result, ses);
}

/*************************************************************************/
/* copy the arg text into the result-space, but substitute the variables */
/* %0..%9 with the real variables                                        */
/*************************************************************************/
void substitute_vars(arg, result)
     char *arg;
     char *result;
{
  int nest = 0;
  int numands, n;
  char *ptr;

  while (*arg) {

    if (*arg == '%') {		/* substitute variable */
      numands = 0;
      while (*(arg + numands) == '%')
	numands++;
      if (isdigit(*(arg + numands)) && numands == (nest + 1)) {
	n = *(arg + numands) - '0';
	strcpy(result, vars[n]);
	arg = arg + numands + 1;
	result += strlen(vars[n]);
      } else {
	strncpy(result, arg, numands + 1);
	arg += numands + 1;
	result += numands + 1;
      }
    }
    if (*arg == '$') {		/* substitute variable */
      numands = 0;
      while (*(arg + numands) == '$')
	numands++;
      if (isdigit(*(arg + numands)) && numands == (nest + 1)) {
	n = *(arg + numands) - '0';
	ptr = vars[n];
	while (*ptr) {
	  if (*ptr == ';')
	    ptr++;
	  else
	    *result++ = *ptr++;
	}
	arg = arg + numands + 1;
      } else {
	strncpy(result, arg, numands);
	arg += numands;
	result += numands;
      }
    } else if (*arg == DEFAULT_OPEN) {
      nest++;
      *result++ = *arg++;
    } else if (*arg == DEFAULT_CLOSE) {
      nest--;
      *result++ = *arg++;
    } else if (*arg == '\\' && nest == 0) {
      while (*arg == '\\')
	*result++ = *arg++;
      if (*arg == '%') {
	result--;
	*result++ = *arg++;
	*result++ = *arg++;
      }
    } else
      *result++ = *arg++;
  }
  *result = '\0';
}



/**********************************************/
/* check actions from a sessions against line */
/**********************************************/
void check_all_actions(line, ses)
     char *line;
     struct session *ses;
{

  struct listnode *ln,*ol;
  static char temp[BUFFER_SIZE] = PROMPT_FOR_PW_TEXT;
  char strng[BUFFER_SIZE];

  if (check_one_action(line, temp, ses) && ses == activesession) {
//    term_noecho();	// KB:??
    term_echoing = FALSE;
  }
  ln = (ses) ? ses->actions : common_actions;

  inActions=1;

  while ((ln = ln->next)) {
    if (check_one_action(line, ln->left, ses)) {
      char buffer[BUFFER_SIZE];

      prepare_actionalias(ln->right, buffer, ses);
      if (mesvar[1] && activesession == ses) {
	sprintf(strng, "[ACTION: %s]", buffer);
	tintin_puts2(strng, activesession);
      }
      parse_input(buffer, ses);
/*      return;*/		/* KB: we want ALL actions to be done */
    }
  }
  ln =(ol=(ses)?ses->actions:common_actions)->next;
  while (deletedActions && ln)
      if (strcmp(ln->left,K_ACTION_MAGIC))
          ln=(ol=ln)->next;
      else
      {
          struct listnode *l;
	  l=ln;
	  ol->next=ln=ln->next;
	  free(l->left);
	  free(l->right);
	  free(l->pr);
	  free(l);
	  deletedActions--;
      }
      
  inActions=0;
}

int match_a_string(line, mask)
     char *line;
     char *mask;
{
  char *lptr, *mptr;

  lptr = line;
  mptr = mask;
  while (*lptr && *mptr && !(*mptr == '%' && isdigit(*(mptr + 1)))) {
    if (*lptr++ != *mptr++)
      return -1;
  }
  if (!*mptr || (*mptr == '%' && isdigit(*(mptr + 1)))) {
    return (int)(lptr - line);
  }
  return -1;
}

int check_one_action(line, action, ses)
     char *line;
     char *action;
     struct session *ses;
{
  int i;

  if (check_a_action(line, action, ses)) {
    for (i = 0; i < 10; i++) {
      if (var_len[i] != -1) {
	strncpy(vars[i], var_ptr[i], var_len[i]);
	*(vars[i] + var_len[i]) = '\0';
      }
    }
    return TRUE;
  } else
    return FALSE;
}
/******************************************************************/
/* check if a text triggers an action and fill into the variables */
/* return TRUE if triggered                                       */
/******************************************************************/
int check_a_action(line, action, ses)
     char *line;
     char *action;
     struct session *ses;
{
  char result[BUFFER_SIZE];
  char *temp2, *tptr, *lptr, *lptr2;
  int i, flag_anchor, len, flag;

  for (i = 0; i < 10; i++)
    var_len[i] = -1;
  flag_anchor = FALSE;
  lptr = line;
  substitute_myvars(action, result, ses);
  tptr = result;
  if (*tptr == '^') {
    tptr++;
    flag_anchor = TRUE;
    /* CHANGED to fix a bug with #action {^%0 foo}
     * Thanks to Spencer Sun for the bug report (AND fix!)
     if (*tptr!=*line)
     return FALSE;
     */
  }
  if (flag_anchor) {
    if ((len = match_a_string(lptr, tptr)) == -1)
      return FALSE;
    lptr += len;
    tptr += len;
  } else {
    flag = TRUE;
    len = -1;
    while (*lptr && flag) {
      if ((len = match_a_string(lptr, tptr)) != -1) {
	flag = FALSE;
      } else
	lptr++;
    }
    if (len != -1) {
      lptr += len;
      tptr += len;
    } else
      return FALSE;
  }
  while (*lptr && *tptr) {
    temp2 = tptr + 2;
    if (!*temp2) {
      var_len[*(tptr + 1) - 48] = strlen(lptr);
      var_ptr[*(tptr + 1) - 48] = lptr;
      return TRUE;
    }
    lptr2 = lptr;
    flag = TRUE;
    len = -1;
    while (*lptr2 && flag) {
      if ((len = match_a_string(lptr2, temp2)) != -1) {
	flag = FALSE;
      } else
	lptr2++;
    }
    if (len != -1) {
      var_len[*(tptr + 1) - 48] = lptr2 - lptr;
      var_ptr[*(tptr + 1) - 48] = lptr;
      lptr = lptr2 + len;
      tptr = temp2 + len;
    } else {
      return FALSE;
    }
  }
  if (*tptr)
    return FALSE;
  else
    return TRUE;
}
