/* $Id: path.c,v 2.2 1998/10/11 18:36:36 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: path.c - stuff for the path feature                         */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                    coded by peter unold 1992                      */
/*                  recoded by Jeremy C. Jack 1994                   */
/*********************************************************************/
/* the path is implemented as a fix-sized queue. It gets a bit messy */
/* here and there, but it should work....                            */
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void check_insert_path();
void insert_path();
int return_flag = TRUE;

extern char *get_arg_in_braces();
extern void prompt();
extern void kill_list();
extern struct listnode *search_node_with_wild();
extern struct listnode *searchnode_list();
extern struct listnode *init_list();
extern struct listnode *deletenode_list();
extern struct listnode *addnode_list();

extern char tintin_char;
extern struct listnode *common_pathdirs;
extern int pdnum;
extern int mesvar[7];

void mark_command(ses)
     struct session *ses;
{
  if (ses) {
    kill_list(ses->path);
    ses->path = init_list(ses->path);
    ses->path_length = 0;
    tintin_puts("#Beginning of path marked.", ses);
  } else
    tintin_puts("#No session active => NO PATH!", ses);
}

void map_command(arg, ses)
     char *arg;
     struct session *ses;
{
  if (ses) {
    get_arg_in_braces(arg, arg, 1);
    check_insert_path(arg, ses);
  } else
    tintin_puts2("No session active => NO PATH!", ses);
}

void savepath_command(arg, ses)
     struct session *ses;
     char *arg;
{
  if (ses) {
    get_arg_in_braces(arg, arg, 1);
    if ((strlen(arg)) && (ses->path_length)) {
      char result[BUFFER_SIZE];
      struct listnode *ln = ses->path;
      int dirlen, len = 0;

      sprintf(result, "%calias {%s} {", tintin_char, arg);
      len = strlen(result);
      while (ln = ln->next) {
	dirlen = strlen(ln->left);
	if (dirlen + len < BUFFER_SIZE) {
	  strcat(result, ln->left);
	  len += dirlen + 1;
	  if (ln->next)
	    strcat(result, ";");
	} else {
	  tintin_puts("#Error - buffer  too small to contain alias", ses);
	  break;
	}
      }
      strcat(result, "}");
      parse_input(result, ses);
    } else
      tintin_puts("#Error: no alias for savepath or no path", ses);
  } else
    tintin_puts2("No session active => NO PATH TO SAVE!", ses);
}

void path_command(ses)
     struct session *ses;
{
  if (ses) {
    int len = 0, dirlen;
    struct listnode *ln = ses->path;
    char mypath[81];

    strcpy(mypath, "#Path:  ");
    while (ln = ln->next) {
      dirlen = strlen(ln->left);
      if (dirlen + len > 70) {
	tintin_puts(mypath, ses);
	strcpy(mypath, "#Path:  ");
	len = 0;
      }
      strcat(mypath, ln->left);
      strcat(mypath, " ");
      len += dirlen + 1;
    }
    tintin_puts(mypath, ses);
  } else
    tintin_puts("No session active => NO PATH!", ses);
}

void return_command(ses)
     struct session *ses;
{
  if (ses) {
    if (ses->path_length) {
      struct listnode *ln = ses->path;
      char command[BUFFER_SIZE];

      ses->path_length--;
      while (ln->next)
	(ln = ln->next);
      strcpy(command, ln->right);
      return_flag = FALSE;	/* temporarily turn off path tracking */
      parse_input(command, ses);
      return_flag = TRUE;	/* restore path tracking */
      deletenode_list(ses->path, ln);
    } else
      tintin_puts("#No place to return from!", ses);
  } else
    tintin_puts("#No session active => NO PATH!", ses);
}

void unpath_command(ses)
     struct session *ses;
{
  if (ses)
    if (ses->path_length) {
      struct listnode *ln = ses->path;

      ses->path_length--;
      while (ln->next)
	(ln = ln->next);
      deletenode_list(ses->path, ln);
      tintin_puts("#Ok.  Forgot that move.", ses);
    } else
      tintin_puts("#No move to forget!", ses);
  else
    tintin_puts("#No session active => NO PATH!", ses);
}

void check_insert_path(command, ses)
     char *command;
     struct session *ses;
{
  struct listnode *ln;

  if (!return_flag)
    return;

  if ((ln = searchnode_list(ses->pathdirs, command)) != NULL) {
    if (ses->path_length != MAX_PATH_LENGTH)
      ses->path_length++;
    else if (ses->path_length)
      deletenode_list(ses->path, ses->path->next);
    addnode_list(ses->path, ln->left, ln->right, "0");
  }
}

void pathdir_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
  struct listnode *mypathdirs, *ln;

  mypathdirs = (ses) ? ses->pathdirs : common_pathdirs;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);

  if (!*left) {
    tintin_puts2("#These PATHDIRS have been defined:", ses);
    show_list(mypathdirs);
    prompt(ses);
  } else if (*left && !*right) {
    if ((ln = search_node_with_wild(mypathdirs, left)) != NULL) {
      while ((mypathdirs = search_node_with_wild(mypathdirs, left)) != NULL)
	shownode_list(mypathdirs);
      prompt(ses);
    } else if (mesvar[6])
      tintin_puts2("#That PATHDIR is not defined.", ses);
  } else {
    if ((ln = searchnode_list(mypathdirs, left)) != NULL)
      deletenode_list(mypathdirs, ln);
    insertnode_list(mypathdirs, left, right, "0", ALPHA);
    if (mesvar[6]) {
      sprintf(arg2, "#Ok.  {%s} is marked in #path. {%s} is it's #return.",
	      left, right);
      tintin_puts2(arg2, ses);
    }
    pdnum++;
  }
}
