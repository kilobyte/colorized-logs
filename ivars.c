/* $Id: ivars.c,v 2.2 1998/11/25 17:14:00 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
#include "config.h"
#include "tintin.h"
#include <ctype.h>
int stacks[100][3];
extern struct listnode *common_myvars;

#ifdef HAVE_UNISTD_H
#include <stdlib.h>
#include <unistd.h>
#endif

extern char *get_arg_in_braces();
extern char *space_out();
extern struct listnode *searchnode_list();

extern char tintin_char;
extern int mesvar[5];

void math_command(line, ses)
     char *line;
     struct session *ses;
{
  /* char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], */
  char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE], result[BUFFER_SIZE];
  struct listnode *my_vars, *ln;
  int i;

  my_vars = (ses) ? ses->myvars : common_myvars;
  line = get_arg_in_braces(line, left, 0);
  line = get_arg_in_braces(line, right, 1);
  substitute_vars(right, result);
  substitute_myvars(result, right, ses);
  i = eval_expression(right);
  sprintf(temp, "%d", i);
  if ((ln = searchnode_list(my_vars, left)) != NULL)
    deletenode_list(my_vars, ln);
  insertnode_list(my_vars, left, temp, "0", ALPHA);
}

void if_command(line, ses)
     char *line;
     struct session *ses;
{
  /* char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE],  */
  char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE];

  /* int i; */
  line = get_arg_in_braces(line, left, 0);
  line = get_arg_in_braces(line, right, 1);
  substitute_vars(right, temp);
  substitute_myvars(temp, right, ses);
  substitute_vars(left, temp);
  substitute_myvars(temp, left, ses);
  if (eval_expression(left)) {
    parse_input(right, ses);
  } else {
    line = get_arg_in_braces(line, left, 0);
    if (*left == tintin_char) {

      if (is_abrev(left + 1, "else")) {
	line = get_arg_in_braces(line, right, 1);
	substitute_vars(right, temp);
	substitute_myvars(temp, right, ses);
	parse_input(right, ses);
      }
      if (is_abrev(left + 1, "elif"))
	if_command(line, ses);
    }
  }
}


int eval_expression(arg)
     char *arg;
{
  /* int i, begin, end, flag, prev, ptr; */
  int i, begin, end, flag, prev;
  char temp[BUFFER_SIZE];

  i = conv_to_ints(arg);
  if (i) {
    while (1) {
      i = 0;
      flag = 1;
      begin = -1;
      end = -1;
      prev = -1;
      while (stacks[i][0] && flag) {
	if (stacks[i][1] == 0) {
	  begin = i;
	} else if (stacks[i][1] == 1) {
	  end = i;
	  flag = 0;
	}
	prev = i;
	i = stacks[i][0];
      }
      if ((flag && (begin != -1)) || (!flag && (begin == -1))) {
	tintin_puts2("#Unmatched parentheses error.", (struct session *)NULL);
	return 0;
      }
      if (flag) {
	if (prev == -1)
	  return (stacks[0][2]);
	begin = -1;
	end = i;
      }
      i = do_one_inside(begin, end);
      if (!i) {
	sprintf(temp, "#Invalid expression to evaluate in {%s}", arg);
	tintin_puts2(temp, (struct session *)NULL);
	return 0;
      }
    }
  } else
    return 0;
}
int conv_to_ints(arg)
     char *arg;
{
  int i, flag, result;
  int m; /* =0 should match, =1 should differ */
  int regex; /* =0 strncmp, =1 regex match */
  char *ptr, *tptr;
  char temp[BUFFER_SIZE];
  char left[BUFFER_SIZE], right[BUFFER_SIZE];

  i = 0;
  ptr = arg;
  while (*ptr) {
    if (*ptr == ' ') ;
    /* jku: comparing strings with = and != */
    else if (*ptr == '[') {
      ptr++;
      tptr=left;
      while((*ptr) && (*ptr != ']') && (*ptr != '=') && (*ptr != '!')) {
        *tptr = *ptr;
        ptr++;
        tptr++;
      }
      *tptr='\0';
      if(!*ptr)
        return 0; /* error */
      if(*ptr == ']') {
        sprintf(temp, "#Compare %s to what ? (only one var between [ ])", left);
        tintin_puts2(temp, (struct session *)NULL);
      }
      /* fprintf(stderr, "Left argument = '%s'\n", left); */
      switch(*ptr) {
        case '!' : 
          ptr++;
          m=1;
          switch(*ptr) {
            case '=' : regex=0; ptr++; break;
            case '~' : regex=1; ptr++; break;
            default : return 0;
          }
          break;
        case '=' : 
          ptr++;
          m=0;
          switch(*ptr) {
            case '=' : regex=0; ptr++; break;
            case '~' : regex=1; ptr++; break;
            default : break;
          }
          break;
        default : return 0;
      }
      
      /* fprintf(stderr, "%c - %s match\n", (m) ? '=' : '!', (regex) ? "regex" : "string"); */
      
      tptr=right;
      while((*ptr) && (*ptr != ']')) {
        *tptr = *ptr;
        ptr++;
        tptr++;
      }
      *tptr='\0';
      /* fprintf(stderr, "Right argument = '%s'\n", right); */
      if(!*ptr)
        return 0;
      if(regex)
        result = match(right, left) ? 0 : 1;
      else
        result = strcmp(left, right);
      if((result == 0 && m == 0) || (result != 0 && m != 0)) { /* success */
        stacks[i][1] = 15;
        stacks[i][2] = 1;
        /* fprintf(stderr, "Expr TRUE\n"); */
      }
      else {
        stacks[i][1] = 15;
        stacks[i][2] = 0;
        /* fprintf(stderr, "Expr FALSE\n"); */
      }
    }
    /* jku: end of comparing strings */
    /* jku: undefined variables are now assigned value 0 (false) */
    else if (*ptr == '$') {
      if (mesvar[5]) {
	sprintf(temp, "#Undefined variable in expression: %s", arg);
	tintin_puts2(temp, (struct session *)NULL);
      }
      stacks[i][1] = 15;
      stacks[i][2] = 0;
      while (isalpha(*(ptr + 1)))
	ptr++;
    }
    /* jku: end of changes */
    else if (*ptr == '(') {
      stacks[i][1] = 0;
    } else if (*ptr == ')') {
      stacks[i][1] = 1;
    } else if (*ptr == '!') {
      if (*(ptr + 1) == '=') {
	stacks[i][1] = 12;
	ptr++;
      } else
	stacks[i][1] = 2;
    } else if (*ptr == '*') {
      stacks[i][1] = 3;
    } else if (*ptr == '/') {
      stacks[i][1] = 4;
    } else if (*ptr == '+') {
      stacks[i][1] = 5;
    } else if (*ptr == '-') {
      flag = -1;
      if (i > 0)
	flag = stacks[i - 1][1];
      if (flag == 15)
	stacks[i][1] = 6;
      else {
	tptr = ptr;
	ptr++;
	while (isdigit(*ptr))
	  ptr++;
	sscanf(tptr, "%d", &stacks[i][2]);
	stacks[i][1] = 15;
	ptr--;
      }
    } else if (*ptr == '>') {
      if (*(ptr + 1) == '=') {
	stacks[i][1] = 8;
	ptr++;
      } else
	stacks[i][1] = 7;
    } else if (*ptr == '<') {
      if (*(ptr + 1) == '=') {
	ptr++;
	stacks[i][1] = 10;
      } else
	stacks[i][1] = 9;
    } else if (*ptr == '=') {
      stacks[i][1] = 11;
      if (*(ptr + 1) == '=')
	ptr++;
    } else if (*ptr == '&') {
      stacks[i][1] = 13;
      if (*(ptr + 1) == '&')
	ptr++;
    } else if (*ptr == '|') {
      stacks[i][1] = 14;
      if (*(ptr + 1) == '|')
	ptr++;
    } else if (isdigit(*ptr)) {
      stacks[i][1] = 15;
      tptr = ptr;
      while (isdigit(*ptr))
	ptr++;
      sscanf(tptr, "%d", &stacks[i][2]);
      ptr--;
    } else if (*ptr == 'T') {
      stacks[i][1] = 15;
      stacks[i][2] = 1;
    } else if (*ptr == 'F') {
      stacks[i][1] = 15;
      stacks[i][2] = 0;
    } else {
      tintin_puts2("#Error. Invalid expression in #if or #math", (struct session *)NULL);
      return 0;
    }
    if (*ptr != ' ') {
      stacks[i][0] = i + 1;
      i++;
    }
    ptr++;
  }
  if (i > 0)
    stacks[i][0] = 0;
  return 1;
}

int do_one_inside(begin, end)
     int begin;
     int end;
{
  /* int prev, ptr, highest, loc, ploc, next, nval, flag; */
  int prev, ptr, highest, loc, ploc, next;

  while (1) {
    ptr = 0;
    if (begin > -1)
      ptr = stacks[begin][0];
    highest = 16;
    loc = -1;
    ploc = -1;
    prev = -1;
    while (ptr < end) {
      if (stacks[ptr][1] < highest) {
	highest = stacks[ptr][1];
	loc = ptr;
	ploc = prev;
      }
      prev = ptr;
      ptr = stacks[ptr][0];
    }
    if (highest == 15) {
      if (begin > -1) {
	stacks[begin][1] = 15;
	stacks[begin][2] = stacks[loc][2];
	stacks[begin][0] = stacks[end][0];
	return 1;
      } else {
	stacks[0][0] = stacks[end][0];
	stacks[0][1] = 15;
	stacks[0][2] = stacks[loc][2];
	return 1;
      }
    } else if (highest == 2) {
      next = stacks[loc][0];
      if (stacks[next][1] != 15 || stacks[next][0] == 0) {
	return 0;
      }
      stacks[loc][0] = stacks[next][0];
      stacks[loc][1] = 15;
      stacks[loc][2] = !stacks[next][2];
    } else {
      next = stacks[loc][0];
      if (ploc == -1 || stacks[next][0] == 0 || stacks[next][1] != 15)
	return 0;
      if (stacks[ploc][1] != 15)
	return 0;
      switch (highest) {
      case 3:			/* highest priority is * */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] *= stacks[next][2];
	break;
      case 4:			/* highest priority is / */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] /= stacks[next][2];
	break;
      case 5:			/* highest priority is + */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] += stacks[next][2];
	break;
      case 6:			/* highest priority is - */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] -= stacks[next][2];
	break;
      case 7:			/* highest priority is > */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] > stacks[next][2]);
	break;
      case 8:			/* highest priority is >= */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] >= stacks[next][2]);
	break;
      case 9:			/* highest priority is < */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] < stacks[next][2]);
	break;
      case 10:			/* highest priority is <= */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] <= stacks[next][2]);
	break;
      case 11:			/* highest priority is == */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] == stacks[next][2]);
	break;
      case 12:			/* highest priority is != */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] != stacks[next][2]);
	break;
      case 13:			/* highest priority is && */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] && stacks[next][2]);
	break;
      case 14:			/* highest priority is || */
	stacks[ploc][0] = stacks[next][0];
	stacks[ploc][2] = (stacks[ploc][2] || stacks[next][2]);
	break;
      default:
	tintin_puts2("#Programming error *slap Bill*", (struct session *)NULL);
	return 0;
      }
    }
  }
}
