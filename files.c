/* $Id: files.c,v 1.8 1998/11/25 17:14:00 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: files.c - funtions for logfile and reading/writing comfiles */
/*                             TINTIN + +                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*                    New code by Bill Reiss 1993                    */
/*********************************************************************/
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include <unistd.h>
#include "tintin.h"

struct listnode *common_aliases, *common_actions, *common_subs, *common_myvars,
 *common_highs, *common_antisubs, *common_pathdirs;
struct completenode *complete_head;
void prepare_for_write();

extern struct session *parse_input();
extern char *get_arg_in_braces();
extern void user_done();

extern int puts_echoing;
extern int alnum, acnum, subnum, hinum, varnum, antisubnum;
extern int verbose;
extern char tintin_char;

/**********************************/
/* load a completion file         */
/**********************************/
void read_complete()
{
  FILE *myfile;
  char buffer[BUFFER_SIZE], *cptr;
  int flag;
  struct completenode *tcomplete, *tcomp2;

  flag = TRUE;
  if ((complete_head = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL) {
    fprintf(stderr, "couldn't alloc completehead\n");
    user_done();
    exit(1);
  }
  tcomplete = complete_head;

  if ((myfile = fopen("tab.txt", "r")) == NULL) {
    if ((cptr = (char *)getenv("HOME"))) {
      strcpy(buffer, cptr);
      strcat(buffer, "/.tab.txt");
      myfile = fopen(buffer, "r");
    }
  }
  if (myfile == NULL) {
    tintin_puts("no tab.txt file, no completion list\n", (struct session *)NULL);
    return;
  }
  while (fgets(buffer, sizeof(buffer), myfile)) {
    for (cptr = buffer; *cptr && *cptr != '\n'; cptr++) ;
    *cptr = '\0';
    if ((tcomp2 = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL) {
      fprintf(stderr, "couldn't alloc comletehead\n");
      user_done();
      exit(1);
    }
    if ((cptr = (char *)(malloc(strlen(buffer) + 1))) == NULL) {
      fprintf(stderr, "couldn't alloc memory for string in complete\n");
      user_done();
      exit(1);
    }
    strcpy(cptr, buffer);
    tcomp2->strng = cptr;
    tcomplete->next = tcomp2;
    tcomplete = tcomp2;
  }
  tcomplete->next = NULL;
  fclose(myfile);
  tintin_puts("tab.txt file loaded.\n", (struct session *)NULL);
  prompt(NULL);
  tintin_puts("\n", (struct session *)NULL);

}

/*******************************/
/* remove file from filesystem */
/*******************************/
void unlink_file(arg, ses)
     char *arg;
     struct session *ses;
{
  char file[BUFFER_SIZE], temp[BUFFER_SIZE];

  if(*arg) {
    arg = get_arg_in_braces(arg, file, 1);
    substitute_vars(file, temp);
    substitute_myvars(temp, file, ses);
    unlink(file);
  }
  else {
    tintin_puts("#unlink <filename> to remove it", ses);
  }

}

/*************************/
/* the #deathlog command */
/*************************/
void deathlog_command(arg, ses)
     char *arg;
     struct session *ses;
{
  FILE *fh;
  char fname[BUFFER_SIZE], text[BUFFER_SIZE], temp[BUFFER_SIZE];

  if (*arg) {
    arg = get_arg_in_braces(arg, fname, 0);
    arg = get_arg_in_braces(arg, text, 1);
    substitute_vars(fname, temp);
    if (ses)
      substitute_myvars(temp, fname, ses);
    substitute_vars(text, temp);
    if (ses)
      substitute_myvars(temp, text, ses);
    if ((fh = fopen(fname, "a"))) {
      if (text) {
/*	fwrite(text, strlen(text), 1, fh);
	fputc('\n', fh); */
	fprintf(fh, "%s\n", text);
      }
      fclose(fh);
    } else
      tintin_puts("#COULDN'T OPEN FILE.", ses);
  } else
    tintin_puts("#Syntax: #deathlog <file> <text>", ses);
  prompt(NULL);
}

/********************/
/* the #log command */
/********************/
void log_command(arg, ses)
     char *arg;
     struct session *ses;
{
  if (ses) {
    if (!ses->logfile) {
      if (*arg) {
	if ((ses->logfile = fopen(arg, "w")))
	  tintin_puts("#OK. LOGGING.....", ses);
	else
	  tintin_puts("#COULDN'T OPEN FILE.", ses);
      } else
	tintin_puts("#SPECIFY A FILENAME.", ses);
    } else {
      fclose(ses->logfile);
      ses->logfile = NULL;
      tintin_puts("#OK. LOGGING TURNED OFF.", ses);
    }
  } else
    tintin_puts("#THERE'S NO SESSION TO LOG.", ses);
  prompt(NULL);
}

/***********************************/
/* read and execute a command file */
/***********************************/
struct session *read_command(filename, ses)
     char *filename;
     struct session *ses;
{
  FILE *myfile;
  char buffer[BUFFER_SIZE], *cptr;
  char message[80];
  int flag;

  flag = TRUE;
  get_arg_in_braces(filename, filename, 1);
  if ((myfile = fopen(filename, "r")) == NULL) {
    tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
    prompt(NULL);
    return ses;
  }
  if (!verbose)
    puts_echoing = FALSE;
  alnum = 0;
  acnum = 0;
  subnum = 0;
  varnum = 0;
  hinum = 0;
  antisubnum = 0;
  while (fgets(buffer, sizeof(buffer), myfile)) {
    if (flag) {
      puts_echoing = TRUE;
      char_command(buffer, ses);
      if (!verbose)
	puts_echoing = FALSE;
      flag = FALSE;
    }
    for (cptr = buffer; *cptr && *cptr != '\n'; cptr++) ;
    *cptr = '\0';
    ses = parse_input(buffer, ses);
  }
  if (!verbose) {
    puts_echoing = TRUE;
    if (alnum > 0) {
      sprintf(message, "#OK. %d ALIASES LOADED.", alnum);
      tintin_puts2(message, ses);
    }
    if (acnum > 0) {
      sprintf(message, "#OK. %d ACTIONS LOADED.", acnum);
      tintin_puts2(message, ses);
    }
    if (antisubnum > 0) {
      sprintf(message, "#OK. %d ANTISUBS LOADED.", antisubnum);
      tintin_puts2(message, ses);
    }
    if (subnum > 0) {
      sprintf(message, "#OK. %d SUBSTITUTES LOADED.", subnum);
      tintin_puts2(message, ses);
    }
    if (varnum > 0) {
      sprintf(message, "#OK. %d VARIABLES LOADED.", varnum);
      tintin_puts2(message, ses);
    }
    if (hinum > 0) {
      sprintf(message, "#OK. %d HIGHLIGHTS LOADED.", hinum);
      tintin_puts2(message, ses);
    }
  }
  fclose(myfile);
  prompt(NULL);
  return ses;
}

/************************/
/* write a command file */
/************************/
struct session *write_command(filename, ses)
     char *filename;
     struct session *ses;
{
  FILE *myfile;
  char buffer[BUFFER_SIZE];
  struct listnode *nodeptr;

  get_arg_in_braces(filename, filename, 1);
  if (*filename == '\0') {
    tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
    prompt(NULL);
    return (0);			/* added zero return */
  }
  if ((myfile = fopen(filename, "w")) == NULL) {
    tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
    prompt(NULL);
    return (0);			/* added zero return */
  }
  nodeptr = (ses) ? ses->aliases : common_aliases;
  while ((nodeptr = nodeptr->next)) {
    prepare_for_write("alias", nodeptr->left, nodeptr->right, "\0", buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->actions : common_actions;
  while ((nodeptr = nodeptr->next)) {
    prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr,
		      buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->antisubs : common_antisubs;
  while ((nodeptr = nodeptr->next)) {
    prepare_for_write("antisubstitute", nodeptr->left,
		      nodeptr->right, "\0", buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->subs : common_subs;
  while ((nodeptr = nodeptr->next)) {
    prepare_for_write("substitute", nodeptr->left, nodeptr->right, "\0", buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->myvars : common_myvars;
  while ((nodeptr = nodeptr->next)) {
    prepare_for_write("variable", nodeptr->left, nodeptr->right, "\0", buffer);
    fputs(buffer, myfile);
  }
  nodeptr = (ses) ? ses->highs : common_highs;
  while ((nodeptr = nodeptr->next)) {
    prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
    fputs(buffer, myfile);
  }
  nodeptr = (ses) ? ses->pathdirs : common_pathdirs;
  while ((nodeptr = nodeptr->next)) {
    prepare_for_write("pathdir", nodeptr->right, nodeptr->left, "\0", buffer);
    fputs(buffer, myfile);
  }
  fclose(myfile);
  tintin_puts("#COMMANDO-FILE WRITTEN.", ses);
  return ses;
}

/************************/
/* write a command file */
/************************/
struct session *writesession_command(filename, ses)
     char *filename;
     struct session *ses;
{
  FILE *myfile;
  char buffer[BUFFER_SIZE];
  struct listnode *nodeptr;

  get_arg_in_braces(filename, filename, 1);
  if (*filename == '\0') {
    tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
    prompt(NULL);
    return (0);			/* added zero return */
  }
  if ((myfile = fopen(filename, "w")) == NULL) {
    tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
    prompt(NULL);
    return (0);			/* added zero return */
  }
  nodeptr = (ses) ? ses->aliases : common_aliases;
  while ((nodeptr = nodeptr->next)) {
    if (ses && searchnode_list(common_aliases, nodeptr->left))
      continue;
    prepare_for_write("alias", nodeptr->left, nodeptr->right, "\0", buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->actions : common_actions;
  while ((nodeptr = nodeptr->next)) {
    if (ses && searchnode_list(common_actions, nodeptr->left))
      continue;
    prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr,
		      buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->antisubs : common_antisubs;
  while ((nodeptr = nodeptr->next)) {
    if (ses && searchnode_list(common_antisubs, nodeptr->left))
      continue;
    prepare_for_write("antisubstitute", nodeptr->left, "", "\0", buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->subs : common_subs;
  while ((nodeptr = nodeptr->next)) {
    if (ses && searchnode_list(common_subs, nodeptr->left))
      continue;
    prepare_for_write("substitute", nodeptr->left, nodeptr->right, "\0", buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->myvars : common_myvars;
  while ((nodeptr = nodeptr->next)) {
    if (ses && searchnode_list(common_myvars, nodeptr->left))
      continue;
    prepare_for_write("variable", nodeptr->left, nodeptr->right, "\0", buffer);
    fputs(buffer, myfile);
  }

  nodeptr = (ses) ? ses->highs : common_highs;
  while ((nodeptr = nodeptr->next)) {
    if (ses && searchnode_list(common_highs, nodeptr->left))
      continue;
    prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
    fputs(buffer, myfile);
  }

  fclose(myfile);
  tintin_puts("#COMMANDO-FILE WRITTEN.", ses);
  return ses;
}


void prepare_for_write(command, left, right, pr, result)
     char *command;
     char *left;
     char *right;
     char *pr;
     char *result;
{
  /* char tmpbuf[BUFFER_SIZE]; */
  *result = tintin_char;
  *(result + 1) = '\0';
  strcat(result, command);
  strcat(result, " {");
  strcat(result, left);
  strcat(result, "}");
  if (strlen(right) != 0) {
    strcat(result, " {");
    strcat(result, right);
    strcat(result, "}");
  }
  if (strlen(pr) != 0) {
    strcat(result, " {");
    strcat(result, pr);
    strcat(result, "}");
  }
  strcat(result, "\n");
}

void prepare_quotes(string)
     char *string;
{
  char s[BUFFER_SIZE], *cpsource, *cpdest;
  int nest = FALSE;

  strcpy(s, string);

  cpsource = s;
  cpdest = string;

  while (*cpsource) {
    if (*cpsource == '\\') {
      *cpdest++ = *cpsource++;
      if (*cpsource)
	*cpdest++ = *cpsource++;
    } else if (*cpsource == '\"' && nest == FALSE) {
      *cpdest++ = '\\';
      *cpdest++ = *cpsource++;
    } else if (*cpsource == '{') {
      nest = TRUE;
      *cpdest++ = *cpsource++;
    } else if (*cpsource == '}') {
      nest = FALSE;
      *cpdest++ = *cpsource++;
    } else
      *cpdest++ = *cpsource++;
  }
  *cpdest = '\0';
}
