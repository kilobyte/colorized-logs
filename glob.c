/* $Id: glob.c,v 1.3 1998/10/11 18:36:34 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*
 * match -- returns 1 if `string' satisfised `regex' and 0 otherwise
 * stolen from Spencer Sun: only recognizes * and \ as special characters
 */
 
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
#include <stdlib.h>
 
int match(char *regex, char *string)
{
  char *rp = regex, *sp = string, ch, *save;

  while (*rp != '\0') {
    switch (ch = *rp++) {
    case '*':
      if ('\0' == *sp)		/* match empty string at end of `string' */
	return ('\0' == *rp);	/* but only if we're done with the pattern */
      /* greedy algorithm: save starting location, then find end of string */
      save = sp;
      sp += strlen(sp);
      do {
	if (match(rp, sp))	/* return success if we can match here */
	  return 1;
      } while (--sp >= save);	/* otherwise back up and try again */
      /*
       * Backed up all the way to starting location (i.e. `*' matches
       * empty string) and we _still_ can't match here.  Give up.
       */
      return 0;
      /* break; not reached */
    case '\\':
      if ((ch = *rp++) != '\0') {
	/* if not end of pattern, match next char explicitly */
	if (ch != *sp++)
	  return 0;
	break;
      }
      /* else FALL THROUGH to match a backslash */
    default:			/* normal character */
      if (ch != *sp++)
	return 0;
      break;
    }
  }
  /*
   * OK, we successfully matched the pattern if we got here.  Now return
   * a match if we also reached end of string, otherwise failure
   */
  return ('\0' == *sp);
}


int is_literal(char *txt)
{
    return !strchr(txt, '*');
}


int find(char *text,char *pat,int *from,int *to)
{
	char *a,*b,*txt,*m1,*m2;
	int i;
	txt=text;
	if (*pat=='^')
	{
		for (pat++;(*pat)&&(*pat!='*');)
			if (*(pat++)!=*(txt++))
				return(0);
		if (!*pat)
		{
			*from=0;
			*to=txt-text-1;
			return 1;
		};
		m1=malloc(strlen(pat)+1);
		strcpy(m1,pat);
		pat=m1;
		goto start;
	};
	if (!(b=strchr(pat,'*')))
	{
		a=strstr(txt,pat);
		if (a)
		{
			*from=a-text;
			*to=*from+strlen(pat)-1;
			return 1;
		}
		else
			return 0;
	};
	i=b-pat;
	m1=malloc(strlen(pat)+1);
	strcpy(m1,pat);
	pat=m1;
	pat[i]=0;
	txt=strstr(txt,pat);
	if (!txt)
	{
		free(m1);
		return 0;
	};
	*from=txt-text;
	txt+=i;
	pat+=i+1;
	while (*pat=='*')
		pat++;
start:
	i=strlen(pat);
	if (!*pat)
	{
		*to=strlen(text)-1;
		free(m1);
		return 1;
	};
	a=pat;
	b=pat+i-1;
	while (a<b)
	{
		char c=*a;
		*a++=*b;
		*b--=c;
	};
	i=strlen(txt);
	m2=malloc(i+1);
	for (a=m2+i-1;*txt;)
		*a--=*txt++;
	m2[i]=0;
	txt=m2;
	*to=-1;
	do
	{
		b=strchr(pat,'*');
		if (b)
			*b=0;
		a=strstr(txt,pat);
		if (!a)
		{
			free(m1);
			free(m2);
			return 0;
		};
		if (*to==-1)
			*to=strlen(text)-(a-txt)-1;
		txt=a+strlen(pat);
		if (b)
			pat=b;
		else
			pat=strchr(pat,0);
	} while (*pat);
	free(m1);
	free(m2);
	return 1;
}
