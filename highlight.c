/* $Id: highlight.c,v 1.3 1998/10/11 18:51:51 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: highlight.c - functions related to the highlight command    */
/*                             TINTIN ++                             */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by Bill Reiss 1993                      */
/*********************************************************************/
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
/* CHANGED to include <ctype.h>, since we use isdigit() etc.
 * Thanks to Brian Ebersole [Harm@GrimneMUD] for the bug report!
 */
#include <ctype.h>
#include "tintin.h"


#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern char *get_arg_in_braces();
extern struct listnode *searchnode_list();
extern struct listnode *search_node_with_wild();
void add_codes();
int is_high_arg();

extern struct listnode *common_highs;
extern char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */
extern int hinum;
extern int mesvar[7];
extern char high_starts[13][80];
extern char high_ends[13][80];

/***************************/
/* the #highlight command  */
/***************************/
void parse_high(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *myhighs, *ln;
  int colflag = TRUE;
  char *pright, *tmp1, *tmp2, tmp3[BUFFER_SIZE];

  pright = right;
  *pright = '\0';
  myhighs = (ses) ? ses->highs : common_highs;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if (!*left) {
    tintin_puts("#THESE HIGHLIGHTS HAVE BEEN DEFINED:", ses);
    show_list(myhighs);
    prompt(ses);
  } else {
    tmp1 = left;
    tmp2 = tmp1;
    while (*tmp2 != '\0') {
      tmp2++;
      while (*tmp2 != ',' && *tmp2 != '\0')
	tmp2++;
      while (isspace(*tmp1))
	tmp1++;
      strncpy(tmp3, tmp1, tmp2 - tmp1);
      tmp3[tmp2 - tmp1] = '\0';
      colflag = is_high_arg(tmp3);
      tmp1 = tmp2 + 1;
    }
    if (colflag == TRUE) {

      if ((ln = searchnode_list(myhighs, right)) != NULL)
	deletenode_list(myhighs, ln);
      insertnode_list(myhighs, right, left, "0", ALPHA);
      hinum++;
      if (mesvar[4]) {
	sprintf(result, "#Ok. {%s} is now highlighted %s.", right, left);
	tintin_puts2(result, ses);
      }
    } else {
      tintin_puts2("Invalid Highlighting color or effect, valid types are red, blue, cyan, green,", ses);
      tintin_puts2("yellow, magenta, white, grey, black, brown, charcoal, light red, light blue,", ses);
      tintin_puts2("light cyan, light magenta, light green, b red, b blue, b cyan, b green,", ses);
      tintin_puts2("b yellow, b magenta, b white, b grey, b black, b brown, b charcoal, b light", ses);
      tintin_puts2("red, b light blue, b light cyan, b light magenta, b light green, bold,", ses);
      tintin_puts2("faint, blink, italic, reverse, or 1-32");
    }


  }
}

int is_high_arg(s)
     char *s;
{
  int code;

  sscanf(s, "%d", &code);
  if (is_abrev(s, "red") || is_abrev(s, "blue") || is_abrev(s, "cyan") ||
      is_abrev(s, "green") || is_abrev(s, "yellow") ||
      is_abrev(s, "magenta") || is_abrev(s, "white") ||
      is_abrev(s, "grey") || is_abrev(s, "black") ||
      is_abrev(s, "brown") || is_abrev(s, "charcoal") ||
      is_abrev(s, "light red") || is_abrev(s, "light blue") ||
      is_abrev(s, "light cyan") || is_abrev(s, "light magenta") ||
      is_abrev(s, "light green") || is_abrev(s, "b red") ||
      is_abrev(s, "b blue") || is_abrev(s, "b cyan") ||
      is_abrev(s, "b green") || is_abrev(s, "b yellow") ||
      is_abrev(s, "b magenta") || is_abrev(s, "b white") ||
      is_abrev(s, "b grey") || is_abrev(s, "b black") ||
      is_abrev(s, "b brown") || is_abrev(s, "b charcoal") ||
      is_abrev(s, "b light red") || is_abrev(s, "b light blue") ||
      is_abrev(s, "b light cyan") || is_abrev(s, "b light magenta") ||
      is_abrev(s, "b light green") || is_abrev(s, "bold") ||
      is_abrev(s, "faint") || is_abrev(s, "blink") ||
      is_abrev(s, "italic") || is_abrev(s, "reverse") ||
      (isdigit(*s) && code < 33 && code > 0))
    return TRUE;
  else
    return FALSE;
}

/*****************************/
/* the #unhighlight command */
/*****************************/

void unhighlight_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *myhighs, *ln, *temp;
  int flag;

  flag = FALSE;
  myhighs = (ses) ? ses->highs : common_highs;
  temp = myhighs;
  arg = get_arg_in_braces(arg, left, 1);
  while ((ln = search_node_with_wild(temp, left)) != NULL) {
    if (mesvar[4]) {
      sprintf(result, "Ok. {%s} is no longer %s.", ln->left, ln->right);
      tintin_puts2(result, ses);
    }
    deletenode_list(myhighs, ln);
    flag = TRUE;
    /*temp = ln;*/
  }
  if (!flag && mesvar[4])
    tintin_puts2("#THAT HIGHLIGHT IS NOT DEFINED.", ses);
}


void do_one_high(line, ses)
     char *line;
     struct session *ses;
{
  /* struct listnode *ln, *myhighs; */
  struct listnode *ln;
  char temp[BUFFER_SIZE], temp2[BUFFER_SIZE], result[BUFFER_SIZE];
  char *lptr, *tptr, *line2, *firstch_ptr;
  char *place;
  int hflag, length;

  hflag = TRUE;
  ln = ses->highs;
  while ((ln = ln->next)) {

    if (check_one_action(line, ln->left, ses)) {
      firstch_ptr = ln->left;
      if (*(firstch_ptr) == '^')
	firstch_ptr++;
      prepare_actionalias(firstch_ptr, temp, ses);
      *(line + strlen(line) + 2) = '\0';
      hflag = TRUE;
      line2 = line;
      while (hflag) {
	lptr = temp;
	place = line2;
	while (*lptr != *line2) {
	  place = ++line2;
	}
	length = 0;
	while (*lptr == *line2 && *lptr != '\0') {
	  length++;
	  lptr++;
	  line2++;
	}
	if (length >= strlen(temp))
	  hflag = FALSE;
      }
      add_codes(temp, temp2, ln->right, 0);
      *(place) = '\0';
      tptr = line2;
      sprintf(result, "%s%s%s", line, temp2, tptr);
      strcpy(line, result);
    }
  }
}

void add_codes(line, result, htype, flag)
     char *line, *result, *htype;
     int flag;
{
  char *tmp1, *tmp2, tmp3[BUFFER_SIZE];
  int code;

  sprintf(result, "%s", DEFAULT_BEGIN_COLOR);
  tmp1 = htype;
  tmp2 = tmp1;
  while (*tmp2 != '\0') {
    tmp2++;
    while (*tmp2 != ',' && *tmp2 != '\0')
      tmp2++;
    while (isspace(*tmp1))
      tmp1++;
    strncpy(tmp3, tmp1, tmp2 - tmp1);
    tmp3[tmp2 - tmp1] = '\0';
    code = -1;
    if (isdigit(*tmp3)) {
      sscanf(tmp3, "%d", &code);
      code--;
    }
    if (is_abrev(tmp3, "black") || code == 0)
      strcat(result, ";30");
    else if (is_abrev(tmp3, "red") || code == 1)
      strcat(result, ";31");
    else if (is_abrev(tmp3, "green") || code == 2)
      strcat(result, ";32");
    else if (is_abrev(tmp3, "brown") || code == 3)
      strcat(result, ";33");
    else if (is_abrev(tmp3, "blue") || code == 4)
      strcat(result, ";34");
    else if (is_abrev(tmp3, "magenta") || code == 5)
      strcat(result, ";35");
    else if (is_abrev(tmp3, "cyan") || code == 6)
      strcat(result, ";36");
    else if (is_abrev(tmp3, "grey") || code == 7)
      strcat(result, ";37");
    else if (is_abrev(tmp3, "charcoal") || code == 8)
      strcat(result, ";30;1");
    else if (is_abrev(tmp3, "light red") || code == 9)
      strcat(result, ";31;1");
    else if (is_abrev(tmp3, "light green") || code == 10)
      strcat(result, ";32;1");
    else if (is_abrev(tmp3, "yellow") || code == 11)
      strcat(result, ";33;1");
    else if (is_abrev(tmp3, "light blue") || code == 12)
      strcat(result, ";34;1");
    else if (is_abrev(tmp3, "light magenta") || code == 13)
      strcat(result, ";35;1");
    else if (is_abrev(tmp3, "light cyan") || code == 14)
      strcat(result, ";36;1");
    else if (is_abrev(tmp3, "white") || code == 15)
      strcat(result, ";37;1");
    else if (is_abrev(tmp3, "b black") || code == 16)
      strcat(result, ";40");
    else if (is_abrev(tmp3, "b red") || code == 17)
      strcat(result, ";41");
    else if (is_abrev(tmp3, "b green") || code == 18)
      strcat(result, ";42");
    else if (is_abrev(tmp3, "b brown") || code == 19)
      strcat(result, ";43");
    else if (is_abrev(tmp3, "b blue") || code == 20)
      strcat(result, ";44");
    else if (is_abrev(tmp3, "b magenta") || code == 21)
      strcat(result, ";45");
    else if (is_abrev(tmp3, "b cyan") || code == 22)
      strcat(result, ";46");
    else if (is_abrev(tmp3, "b grey") || code == 23)
      strcat(result, ";47");
    else if (is_abrev(tmp3, "b charcoal") || code == 24)
      strcat(result, ";40;1");
    else if (is_abrev(tmp3, "b light red") || code == 25)
      strcat(result, ";41;1");
    else if (is_abrev(tmp3, "b light green") || code == 26)
      strcat(result, ";42;1");
    else if (is_abrev(tmp3, "b yellow") || code == 27)
      strcat(result, ";43;1");
    else if (is_abrev(tmp3, "b light blue") || code == 28)
      strcat(result, ";44;1");
    else if (is_abrev(tmp3, "b light magenta") || code == 29)
      strcat(result, ";45;1");
    else if (is_abrev(tmp3, "b light cyan") || code == 30)
      strcat(result, ";46;1");
    else if (is_abrev(tmp3, "b white") || code == 31)
      strcat(result, ";47;1");
    else if (is_abrev(tmp3, "bold"))
      strcat(result, ";1");
    else if (is_abrev(tmp3, "faint"))
      strcat(result, ";2");
    else if (is_abrev(tmp3, "blink"))
      strcat(result, ";5");
    else if (is_abrev(tmp3, "italic"))
      strcat(result, ";3");
    else if (is_abrev(tmp3, "reverse"))
      strcat(result, ";7");
    tmp1 = tmp2 + 1;
  }
  strcat(result, "m");
  strcat(result, line);
  strcat(result, DEFAULT_END_COLOR);
}
