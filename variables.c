/* $Id: variables.c,v 2.8 1999/01/19 08:46:22 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: variables.c - functions related the the variables           */
/*                             TINTIN ++                             */
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
#include "tintin.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

void substitute_myvars();

extern char *get_arg_in_braces();
extern struct listnode *search_node_with_wild();
extern struct listnode *searchnode_list();

extern struct listnode *common_myvars;
extern char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */
extern int varnum;
extern int mesvar[5];

/*************************/
/* the #variable command */
/*************************/
void var_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
  struct listnode *tempvars, *ln;

  /* char right2[BUFFER_SIZE]; */
  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if (!*left) {
    tintin_puts2("#THESE VARIABLES HAVE BEEN SET:", ses);
    show_list(tempvars);
    prompt(ses);
  } else if (*left && !*right) {
    if ((ln = search_node_with_wild(tempvars, left)) != NULL) {
      while ((tempvars = search_node_with_wild(tempvars, left)) != NULL) {
	shownode_list(tempvars);
      }
      prompt(ses);
    } else if (mesvar[5])
      tintin_puts2("#THAT VARIABLE IS NOT DEFINED.", ses);
  } else {
    if ((ln = searchnode_list(tempvars, left)) != NULL)
      deletenode_list(tempvars, ln);
    insertnode_list(tempvars, left, right, "0", ALPHA);
    varnum++;
    if (mesvar[5]) {
      sprintf(arg2, "#Ok. $%s is now set to {%s}.", left, right);
      tintin_puts2(arg2, ses);
    }
  }
}

/************************/
/* the #unvar   command */
/************************/
void unvar_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *tempvars, *ln, *temp;
  int flag;

  flag = FALSE;
  tempvars = (ses) ? ses->myvars : common_myvars;
  temp = tempvars;
  arg = get_arg_in_braces(arg, left, 1);
  while ((ln = search_node_with_wild(temp, left)) != NULL) {
    if (mesvar[5]) {
      sprintf(result, "#Ok. $%s is no longer a variable.", ln->left);
      tintin_puts2(result, ses);
    }
    deletenode_list(tempvars, ln);
    flag = TRUE;
    /*temp=ln; */
  }
  if (!flag && mesvar[5])
    tintin_puts2("#THAT VARIABLE IS NOT DEFINED.", ses);
}
/*************************************************************************/
/* copy the arg text into the result-space, but substitute the variables */
/* $<string> with the values they stand for                              */
/*************************************************************************/
void substitute_myvars(arg, result, ses)
     char *arg;
     char *result;
     struct session *ses;
{
  /* int varflag=FALSE;
     char *right; */
  char varname[40];
  int nest = 0, counter, varlen;
  struct listnode *ln, *tempvars;

  tempvars = (ses) ? ses->myvars : common_myvars;
  while (*arg) {

    if (*arg == '$') {		/* substitute variable */
      counter = 0;
      while (*(arg + counter) == '$')
	counter++;
      varlen = 0;
      while (isalpha(*(arg + varlen + counter)))
	varlen++;
      if (varlen > 0)
	strncpy(varname, arg + counter, varlen);
      *(varname + varlen) = '\0';
      if (counter == nest + 1 && !isdigit(*(arg + counter + 1))) {
	if ((ln = searchnode_list(tempvars, varname)) != NULL) {
	  strcpy(result, ln->right);
	  result += strlen(ln->right);
	  arg += counter + varlen;
	} else {
	  strncpy(result, arg, counter + varlen);
	  result += varlen + counter;
	  arg += varlen + counter;
	}
      } else {
	strncpy(result, arg, counter + varlen);
	result += varlen + counter;
	arg += varlen + counter;
      }
    } else if (*arg == DEFAULT_OPEN) {
      nest++;
      *result++ = *arg++;
    } else if (*arg == DEFAULT_CLOSE) {
      nest--;
      *result++ = *arg++;
    } else if (*arg == '\\' && *(arg + 1) == '$' && nest == 0) {
      arg++;
      *result++ = *arg++;
    } else
      *result++ = *arg++;
  }
  *result = '\0';
}

/************************/
/* the #tolower command */
/************************/
void tolower_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], *p;
  struct listnode *tempvars, *ln;

  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if (!*left || !*right) {
    tintin_puts2("#Syntax: #tolower <var> <text>", ses);
  } else {
    if ((ln = searchnode_list(tempvars, left)) != NULL)
      deletenode_list(tempvars, ln);
    for (p = right; *p; p++)
      *p = tolower(*p);
    insertnode_list(tempvars, left, right, "0", ALPHA);
    varnum++;
    if (mesvar[5]) {
      sprintf(arg2, "#Ok. $%s is now set to {%s}.", left, right);
      tintin_puts2(arg2, ses);
    }
  }
}

/************************/
/* the #toupper command */
/************************/
void toupper_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], *p;
  struct listnode *tempvars, *ln;

  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if (!*left || !*right) {
    tintin_puts2("#Syntax: #toupper <var> <text>", ses);
  } else {
    if ((ln = searchnode_list(tempvars, left)) != NULL)
      deletenode_list(tempvars, ln);
    for (p = right; *p; p++)
      *p = toupper(*p);
    insertnode_list(tempvars, left, right, "0", ALPHA);
    varnum++;
    if (mesvar[5]) {
      sprintf(arg2, "#Ok. $%s is now set to {%s}.", left, right);
      tintin_puts2(arg2, ses);
    }
  }
}

/***************************/
/* the #firstupper command */
/***************************/
void firstupper_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], *p;
  struct listnode *tempvars, *ln;

  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if (!*left || !*right) {
    tintin_puts2("#Syntax: #firstupper <var> <text>", ses);
  } else {
    if ((ln = searchnode_list(tempvars, left)) != NULL)
      deletenode_list(tempvars, ln);
    for (p = right; *p; p++)
      *p = tolower(*p);
    *right = toupper(*right);
    insertnode_list(tempvars, left, right, "0", ALPHA);
    varnum++;
    if (mesvar[5]) {
      sprintf(arg2, "#Ok. $%s is now set to {%s}.", left, right);
      tintin_puts2(arg2, ses);
    }
  }
}

/***********************/
/* the #random command */
/***********************/
void random_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *tempvars, *ln;
  int low, high, number;

  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if (!*left || !*right) {
    tintin_puts2("#Syntax: #random <var> <low,high>", ses);
  } else {
    substitute_vars(right, result, ses);
    substitute_myvars(result, right, ses);
    if (sscanf(right, "%d,%d", &low, &high) != 2)
      tintin_puts2("#Wrong number of range arguments in #random", ses);
    else if (low < 0 || high < 0) {
      tintin_puts2("#Both arguments of range in #random should be >0", ses);
    } else {
      if (low > high) {
	number = low;
	low = high, high = number;
      };
      number = low + rand() % (high - low + 1);
      sprintf(right, "%d", number);
      if ((ln = searchnode_list(tempvars, left)) != NULL)
	deletenode_list(tempvars, ln);
      insertnode_list(tempvars, left, right, "0", ALPHA);
      varnum++;
      if (mesvar[5]) {
	sprintf(result, "#Ok. $%s is now (randomly) set to {%s}.", left, right);
	tintin_puts2(result, ses);
      }
    }
  }
}

/***********************/
/* the #strip command */
/***********************/
void strip_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE], *p;
  struct listnode *tempvars, *ln;

  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if (!*left || !*right) {
    tintin_puts2("#Syntax: #strip <var> <string>", ses);
  } else {
    substitute_vars(right, result, ses);
    substitute_myvars(result, right, ses);
    p=right;
    while(*p) {
      if(*p == ' ')
        *p = '_';
      else if(*p == ',') {
        *p='\0';
        break;
      }
      p++;
    }
    if ((ln = searchnode_list(tempvars, left)) != NULL)
      deletenode_list(tempvars, ln);
    insertnode_list(tempvars, left, right, "0", ALPHA);
    varnum++;
    if (mesvar[5]) {
      sprintf(result, "#Ok. $%s is now set to {%s}.", left, right);
      tintin_puts2(result, ses);
    }
  }
}

/**********************/
/* the #ctime command */
/**********************/
void ctime_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], arg2[BUFFER_SIZE], *ct, *p;
  struct listnode *tempvars, *ln;
  int tt=time(NULL);

  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  p=ct=ctime((time_t *)&tt);
  while(p && *p) {
    if(*p == '\n') {
      *p='\0';
      break;
    }
    p++;
  }
  if (!*left) {
    sprintf(arg2, "#%s.", ct);
    tintin_puts2(arg2, ses);
  } else {
    if ((ln = searchnode_list(tempvars, left)) != NULL)
      deletenode_list(tempvars, ln);
    insertnode_list(tempvars, left, ct, "0", ALPHA);
    varnum++;
    if (mesvar[5]) {
      sprintf(arg2, "#Ok. $%s is now set to {%s}.", left, ct);
      tintin_puts2(arg2, ses);
    }
  }
}

/**********************/
/* the #index command */
/**********************/
void index_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], mid[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
  struct listnode *tempvars, *ln;
  int index;

  tempvars = (ses) ? ses->myvars : common_myvars;
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, mid, 0);
  arg = get_arg_in_braces(arg, right, 1);
  index=atoi(mid);

  if (!*left || !*right || !*mid) {
    sprintf(arg2, "#index <var> <index> <string>");
    tintin_puts2(arg2, ses);
  }
  else {
    if(index>=strlen(right) && mesvar[5]) {
      sprintf(arg2, "#index %d is outside string %s", index, right);
      tintin_puts2(arg2, ses);
      mid[0]='\0';
    } 
    else {
      mid[0]=right[index];
      mid[1]='\0';
    }
    if ((ln = searchnode_list(tempvars, left)) != NULL)
      deletenode_list(tempvars, ln);
    insertnode_list(tempvars, left, mid, "0", ALPHA);
    varnum++;
    if (mesvar[5]) {
      sprintf(arg2, "#Ok. $%s is now set to {%s}.", left, mid);
      tintin_puts2(arg2, ses);
    }
  }
}
