/* $Id: parse.c,v 2.8 1999/01/19 08:46:22 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: parse.c - some utility-functions                            */
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

#include <ctype.h>
#include <signal.h>
#include "tintin.h"


#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

struct session *parse_tintin_command();
void do_speedwalk();
char *get_arg_with_spaces();

extern void textout(char* txt);
extern struct listnode *searchnode_list_begin();
extern struct session *all_command();
extern struct session *read_command();
extern struct session *session_command();
extern struct session *status_command(char*arg,struct session *ses);
extern struct session *run_command();
extern struct session *write_command();
extern struct session *writesession_command();
extern struct session *zap_command();
extern struct completenode *complete_head;
extern char *space_out();
extern char *get_arg_in_braces();
void write_com_arg_mud();
void prompt();

extern char k_input[240];
extern struct session *sessionlist, *activesession;
extern struct listnode *common_aliases, *common_actions, *common_subs;
extern struct listnode *common_myvars, *common_antisubs;
extern char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */
extern char tintin_char, verbatim_char;
extern int term_echoing, verbatim, mudcolors;
extern int speedwalk, display_row, display_col, input_row, input_col;
extern char system_com[80];
extern void action_command();
extern void unhighlight_command();
extern char *get_arg_stop_spaces();
extern char *cryptkey;
extern char *get_arg_all();
extern void tstphandler();

/**************************************************************************/
/* parse input, check for TINTIN commands and aliases and send to session */
/**************************************************************************/
struct session *parse_input(input, ses)
     char *input;
     struct session *ses;
{
  char command[BUFFER_SIZE], arg[BUFFER_SIZE], result[BUFFER_SIZE];
  char *input2;
  struct listnode *ln;

  if (!term_echoing && activesession == ses) {
    term_echoing = TRUE;
  }
  if (*input == '\0') {
    if (ses)
      write_line_mud("", ses);
    else
      write_com_arg_mud("", "", ses);
    return ses;
  }
  if (is_abrev(input + 1, "verbatim") && *input == tintin_char) {
    verbatim_command();
    return ses;
  }
  if (verbatim && ses) {
    write_line_mud(input, ses);
    return ses;
  }
  if (*input == verbatim_char) {
    input++;
    write_line_mud(input, ses);
    return ses;
  }
  substitute_myvars(input, result, ses);
  input2 = result;
  while (*input2) {
    if (*input2 == ';')
      input2++;
    input2 = get_arg_stop_spaces(input2, command);
    input2 = get_arg_all(input2, arg);


    if (*command == tintin_char)
      ses = parse_tintin_command(command + 1, arg, ses);

    else if ((ln = searchnode_list_begin((ses) ? ses->aliases : common_aliases, command, ALPHA)) != NULL) {
      int i;
      char *cpsource, *cpsource2, newcommand[BUFFER_SIZE], end;

      strcpy(vars[0], arg);

      for (i = 1, cpsource = arg; i < 10; i++) {
	/* Next lines CHANGED to allow argument grouping with aliases */
	while (*cpsource == ' ')
	  cpsource++;
	end = (*cpsource == '{') ? '}' : ' ';
	cpsource = (*cpsource == '{') ? cpsource + 1 : cpsource;
	for (cpsource2 = cpsource; *cpsource2 && *cpsource2 != end; cpsource2++) ;
	strncpy(vars[i], cpsource, cpsource2 - cpsource);
	*(vars[i] + (cpsource2 - cpsource)) = '\0';
	cpsource = (*cpsource2) ? cpsource2 + 1 : cpsource2;
      }
      prepare_actionalias(ln->right, newcommand, ses);
      if (!strcmp(ln->right, newcommand) && *arg) {
	strcat(newcommand, " ");
	strcat(newcommand, arg);
      }
      ses = parse_input(newcommand, ses);
    } else if (speedwalk && !*arg && is_speedwalk_dirs(command))
      do_speedwalk(command, ses);
    else {
      get_arg_with_spaces(arg, arg);
      write_com_arg_mud(command, arg, ses);
    }
  }
  return (ses);
}

/**********************************************************************/
/* return TRUE if commands only consists of capital letters N,S,E ... */
/**********************************************************************/
int is_speedwalk_dirs(cp)
     char *cp;
{
  int flag;

  flag = FALSE;
  while (*cp) {
    if (*cp != 'n' && *cp != 'e' && *cp != 's' && *cp != 'w' && *cp != 'u' && *cp != 'd' &&
	!isdigit(*cp))
      return FALSE;
    if (!isdigit(*cp))
      flag = TRUE;
    cp++;
  }
  return flag;
}

/**************************/
/* do the speedwalk thing */
/**************************/
void do_speedwalk(cp, ses)
     char *cp;
     struct session *ses;
{
  char sc[2];
  char *loc;
  int multflag, loopcnt, i;

  strcpy(sc, "x");
  while (*cp) {
    loc = cp;
    multflag = FALSE;
    while (isdigit(*cp)) {
      cp++;
      multflag = TRUE;
    }
    if (multflag && *cp) {
      sscanf(loc, "%d%c", &loopcnt, sc);
      i = 0;
      while (i++ < loopcnt)
	write_com_arg_mud(sc, "", ses);
    } else if (*cp) {
      sc[0] = *cp;
      write_com_arg_mud(sc, "", ses);
    }
    /* Added the if to make sure we didn't move the pointer outside the 
       bounds of the origional pointer.  Corrects the bug with speedwalking
       where if you typed "u7" tintin would go apeshit. (JE)
     */
    if (*cp)
      cp++;
  }
}


/*************************************/
/* parse most of the tintin-commands */
/*************************************/
struct session *parse_tintin_command(command, arg, ses)
     char *command;
     char *arg;
     struct session *ses;
{
  struct session *sesptr;

  for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
    if (strcmp(sesptr->name, command) == 0) {
      if (*arg) {
	get_arg_with_spaces(arg, arg);
	parse_input(arg, sesptr);	/* was: #sessioname commands */
	return (ses);
      } else {
	char buf[BUFFER_SIZE];

	activesession = sesptr;
	sprintf(buf, "#SESSION '%s' ACTIVATED.", sesptr->name);
	tintin_puts(buf, sesptr);
	prompt(NULL);
	return (sesptr);
      }
    }
  if (isdigit(*command)) {
    int i = atoi(command);

    if (i > 0) {
      get_arg_in_braces(arg, arg, 1);
      while (i-- > 0)
	ses = parse_input(arg, ses);
    } else {
      tintin_puts("#YEAH RIGHT! GO REPEAT THAT YOURSELF DUDE.", ses);
      prompt(NULL);
    }
    return (ses);
  } else if (is_abrev(command, "action"))
    action_command(arg, ses);

  else if (is_abrev(command, "alias"))
    alias_command(arg, ses);

  else if (is_abrev(command, "all"))
    ses = all_command(arg, ses);

  else if (is_abrev(command, "antisubstitute"))
    parse_antisub(arg, ses);

  else if (is_abrev(command, "bell"))
    bell_command(ses);

  else if (is_abrev(command, "blank"))
    blank_command(ses);

  else if (is_abrev(command, "boss"))
    boss_command(ses);

  else if (is_abrev(command, "char"))
    char_command(arg, ses);

  else if (is_abrev(command, "ctime"))
    ctime_command(arg, ses);

  else if (is_abrev(command, "cr"))
    cr_command(ses);

  else if (is_abrev(command, "delay"))
    delay_command(arg, ses);

  else if (is_abrev(command, "deathlog"))
    deathlog_command(arg, ses);

  else if (is_abrev(command, "echo"))
    echo_command(ses);

  else if (is_abrev(command, "end"))
    end_command(command, ses);

  else if (is_abrev(command, "remove"))
    remove_command(arg, ses);

  else if (is_abrev(command, "foreach"))
    foreach_command(arg, ses);

  else if (is_abrev(command, "firstupper"))
    firstupper_command(arg, ses);

  else if (is_abrev(command, "help"))
    help_command(arg);

  else if (is_abrev(command, "highlight"))
    parse_high(arg, ses);

  else if (is_abrev(command, "history"))
    history_command(ses);

  else if (is_abrev(command, "if"))
    if_command(arg, ses);

  else if (is_abrev(command, "else"))
    tintin_puts("#ELSE WITHOUT IF.", ses);

  else if (is_abrev(command, "elif"))
    tintin_puts("#ELIF WITHOUT IF.", ses);

  else if (is_abrev(command, "ignore"))
    ignore_command(ses);

  else if (is_abrev(command, "index"))
    index_command(arg, ses);

  else if (is_abrev(command, "info"))
    display_info(ses);

  else if (is_abrev(command, "killall"))
    kill_all(ses, CLEAN);

  else if (is_abrev(command, "log"))
    log_command(arg, ses);

  else if (is_abrev(command, "loop"))
    loop_command(arg, ses);

  else if (is_abrev(command, "nop")) ;

  else if (is_abrev(command, "map"))
    map_command(arg, ses);

  else if (is_abrev(command, "math"))
    math_command(arg, ses);

  else if (is_abrev(command, "mark"))
    mark_command(ses);

  else if (is_abrev(command, "message"))
    message_command(arg, ses);

  else if (is_abrev(command, "mudcolors"))
    mudcolors_command(ses);

  else if (is_abrev(command, "news"))
    news_command(ses);

  else if (is_abrev(command, "path"))
    path_command(ses);

  else if (is_abrev(command, "pathdir"))
    pathdir_command(arg, ses);

  else if (is_abrev(command, "presub"))
    presub_command(ses);

  else if (is_abrev(command, "random"))
    random_command(arg, ses);

  else if (is_abrev(command, "retab"))
    read_complete();

  else if (is_abrev(command, "return"))
    return_command(ses);

  else if (is_abrev(command, "read"))
    ses = read_command(arg, ses);

  else if (is_abrev(command, "savepath"))
    savepath_command(arg, ses);

  else if (is_abrev(command, "select"))
    select_command(arg, ses);

  else if (is_abrev(command, "session"))
    ses = session_command(arg, ses);

  else if (is_abrev(command, "run"))
    ses = run_command(arg, ses);
     
  else if (is_abrev(command, "showme"))
    showme_command(arg, ses);

  else if (is_abrev(command, "snoop"))
    snoop_command(arg, ses);

  else if (is_abrev(command, "speedwalk"))
    speedwalk_command(ses);

  else if (is_abrev(command, "strip"))
    strip_command(arg, ses);

  /* CHANGED to allow suspending from within tintin. */
  /* I know, I know, this is a hack *yawn* */
  else if (is_abrev(command, "suspend"))
    tstphandler(SIGTSTP);

  else if (is_abrev(command, "status"))
    status_command(arg,ses);

  else if (is_abrev(command, "tablist"))
    tablist(complete_head);

  else if (is_abrev(command, "tabadd"))
    tab_add(arg);

  else if (is_abrev(command, "tabdelete"))
    tab_delete(arg);

  else if (is_abrev(command, "textin"))
    read_file(arg, ses);

  else if (!strncmp(command, "unlink", 6))
    unlink_file(arg, ses);

  else if (is_abrev(command, "substitute"))
    parse_sub(arg, ses);

  else if (is_abrev(command, "gag")) {
    if (*arg != '{') {
      strcpy(command, arg);
      strcpy(arg, "{");
      strcat(arg, command);
      strcat(arg, "} ");
    }
    strcat(arg, " .");
    parse_sub(arg, ses);
  } else if (is_abrev(command, system_com))
    system_command(arg, ses);

  else if (is_abrev(command, "tick"))
    tick_command(ses);

  else if (is_abrev(command, "tickoff"))
    tickoff_command(ses);

  else if (is_abrev(command, "tickon"))
    tickon_command(ses);

  else if (is_abrev(command, "tickset"))
    tickset_command(ses);

  else if (is_abrev(command, "ticksize"))
    ticksize_command(arg, ses);

  else if (is_abrev(command, "tolower"))
    tolower_command(arg, ses);

  else if (is_abrev(command, "togglesubs"))
    togglesubs_command(ses);

  else if (is_abrev(command, "toupper"))
    toupper_command(arg, ses);

  else if (is_abrev(command, "unaction"))
    unaction_command(arg, ses);

  else if (is_abrev(command, "unalias"))
    unalias_command(arg, ses);

  else if (is_abrev(command, "unantisubstitute"))
    unantisubstitute_command(arg, ses);

  else if (is_abrev(command, "unhighlight"))
    unhighlight_command(arg, ses);

  else if (is_abrev(command, "unsubstitute"))
    unsubstitute_command(arg, ses);

  else if (is_abrev(command, "ungag"))
    unsubstitute_command(arg, ses);

  else if (is_abrev(command, "unpath"))
    unpath_command(ses);

  else if (is_abrev(command, "variable"))
    var_command(arg, ses);

  else if (is_abrev(command, "version"))
    version_command();

  else if (is_abrev(command, "unvariable"))
    unvar_command(arg, ses);

  else if (is_abrev(command, "wizlist"))
    wizlist_command(ses);

  else if (is_abrev(command, "write"))
    ses = write_command(arg, ses);

  else if (is_abrev(command, "writesession"))
    ses = writesession_command(arg, ses);

  else if (is_abrev(command, "zap"))
    ses = zap_command(ses);

  else {
    char buf[BUFFER_SIZE];
    snprintf(buf,BUFFER_SIZE,"#UNKNOWN TINTIN-COMMAND: [%c%s]",tintin_char,command);
    tintin_puts(buf, ses);
    prompt(NULL);
  }
  return (ses);
}


/**********************************************/
/* get all arguments - don't remove "s and \s */
/**********************************************/
char *get_arg_all(s, arg)
     char *s;
     char *arg;
{
  /* int inside=FALSE; */
  int nest = 0;

  s = space_out(s);
  while (*s) {

    if (*s == '\\') {
      *arg++ = *s++;
      if (*s)
	*arg++ = *s++;
    } else if (*s == ';' && nest < 1) {
      break;
    } else if (*s == DEFAULT_OPEN) {
      nest++;
      *arg++ = *s++;
    } else if (*s == DEFAULT_CLOSE) {
      nest--;
      *arg++ = *s++;
    } else
      *arg++ = *s++;
  }

  *arg = '\0';
  return s;
}

/**************************************/
/* get all arguments - remove "s etc. */
/* Example:                           */
/* In: "this is it" way way hmmm;     */
/* Out: this is it way way hmmm       */
/**************************************/
char *get_arg_with_spaces(s, arg)
     char *s;
     char *arg;
{
  int nest = 0;

  /* int inside=FALSE; */

  s = space_out(s);
  while (*s) {

    if (*s == '\\') {
      if (*++s)
	*arg++ = *s++;
    } else if (*s == ';' && nest == 0) {
      break;
    } else if (*s == DEFAULT_OPEN) {
      nest++;
      *arg++ = *s++;
    } else if (*s == DEFAULT_CLOSE) {
      *arg++ = *s++;
      nest--;
    } else
      *arg++ = *s++;
  }
  *arg = '\0';
  return s;
}
/********************/
/* my own routine   */
/********************/
char *get_arg_in_braces(s, arg, flag)
     char *s;
     char *arg;
     int flag;
{
  int nest = 0;
  char *ptr;

  s = space_out(s);
  ptr = s;
  if (*s != DEFAULT_OPEN) {
    if (flag == 0)
      s = get_arg_stop_spaces(ptr, arg);
    else
      s = get_arg_with_spaces(ptr, arg);
    return s;
  }
  s++;
  while (*s != '\0' && !(*s == DEFAULT_CLOSE && nest == 0)) {
    if (*s=='\\')
	;
    else
    if (*s == DEFAULT_OPEN)
	nest++;
    else
    if (*s == DEFAULT_CLOSE)
	nest--;
    *arg++ = *s++;
  }
  if (!*s)
    tintin_puts2("#Unmatched braces error!", (struct session *)NULL);
  else
    s++;
  *arg = '\0';
  return s;

}
/**********************************************/
/* get one arg, stop at spaces                */
/* remove quotes                              */
/**********************************************/
char *get_arg_stop_spaces(s, arg)
     char *s;
     char *arg;
{
  int inside = FALSE;

  s = space_out(s);

  while (*s) {
    if (*s == '\\') {
      if (*++s)
	*arg++ = *s++;
    } else if (*s == '"') {
      s++;
      inside = !inside;
    } else if (*s == ';') {
      if (inside)
	*arg++ = *s++;
      else
	break;
    } else if (!inside && *s == ' ')
      break;
    else
      *arg++ = *s++;
  }

  *arg = '\0';
  return s;
}

/*********************************************/
/* spaceout - advance ptr to next none-space */
/* return: ptr to the first none-space       */
/*********************************************/
char *space_out(s)
     char *s;
{
  while (isspace(*s))
    s++;
  return s;
}

/************************************/
/* send command+argument to the mud */
/************************************/
void write_com_arg_mud(command, argument, ses)
     char *command;
     char *argument;
     struct session *ses;
{
  char outtext[BUFFER_SIZE];
  int i;

  if (!ses) {
    char buf[100];

    sprintf(buf, "#NO SESSION ACTIVE. USE THE %cSESSION-COMMAND TO START ONE.", tintin_char);
    tintin_puts(buf, ses);
    prompt(NULL);
  } else {
    check_insert_path(command, ses);
    strncpy(outtext, command, BUFFER_SIZE);
    if (*argument) {
      strncat(outtext, " ", BUFFER_SIZE - strlen(command) - 1);
      strncat(outtext, argument, BUFFER_SIZE - strlen(command) - 2);
    }
    if (mudcolors)
	do_out_MUD_colors(outtext);
    write_line_mud(outtext, ses);
    outtext[i = strlen(outtext)] = '\n';
    if (ses->logfile)
      fwrite(outtext, i + 1, 1, ses->logfile);
  }
}


/***************************************************************/
/* show a prompt - mud prompt if we're connected/else just a > */
/***************************************************************/
void prompt(ses)
     struct session *ses;
{
  char strng[80];

  if (ses && !PSEUDO_PROMPT)
    write_line_mud("", ses);
  else textout(">\n");
}
