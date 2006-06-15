/* $Id: glob.c,v 1.3 1998/10/11 18:36:34 jku Exp $ */
/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*
 * match -- returns 1 if `string' satisfised `regex' and 0 otherwise
 * stolen from Spencer Sun: only recognizes * and \ as special characters
 */
int match(regex, string)
     char *regex;
     char *string;
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
