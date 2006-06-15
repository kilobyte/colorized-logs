/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: misc.c - misc commands                                      */
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
#include "tintin.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* externs */
extern struct listnode *search_node_with_wild();

extern struct session *newactive_session();
extern struct listnode *common_aliases, *common_actions, *common_subs,
 *common_myvars;
extern struct listnode *common_highs, *common_antisubs, *common_pathdirs;
extern char *get_arg_in_braces();
extern struct session *sessionlist,*activesession;
extern struct completenode *complete_head;
extern char tintin_char;
extern int Echo;
extern int speedwalk;
extern int presub;
extern int blank;
extern int togglesubs;
extern int mudcolors;
extern char vars[10][BUFFER_SIZE];	/* the %0, %1, %2,....%9 variables */
extern int mesvar[7];
extern int verbatim;
extern char status[BUFFER_SIZE];
void system_command(char *, struct session *);

/****************************/
/* the cr command           */
/****************************/
void cr_command(ses)
     struct session *ses;
{
  if (ses != NULL)
    write_line_mud("\n", ses);
}
/****************************/
/* the version command      */
/****************************/
void version_command()
{
  char temp[80];

  sprintf(temp, "#You are using TINTIN++ %s\n\r", VERSION_NUM);
  tintin_puts2(temp, NULL);
  prompt(NULL);
}

/****************************/
/* the verbatim command,    */
/* used as a toggle         */
/****************************/
void verbatim_command()
{
  verbatim = !verbatim;
  if (verbatim)
    tintin_puts2("#All text is now sent 'as is'.", (struct sesssion *)NULL);
  else
    tintin_puts2("#Text is no longer sent 'as is'.", (struct session *)NULL);
  prompt(NULL);
}

/********************/
/* the #all command */
/********************/
struct session *all_command(arg, ses)
     char *arg;
     struct session *ses;
{
  struct session *sesptr;

  if (sessionlist) {
    get_arg_in_braces(arg, arg, 1);
    for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
      parse_input(arg, sesptr);
  } else
    tintin_puts("BUT THERE ISN'T ANY SESSION AT ALL!", ses);
  return ses;
}

/*********************/
/* the #bell command */
/*********************/
void bell_command(ses)
     struct session *ses;

{
  user_beep();
}


/*********************/
/* the #boss command */
/*********************/
void boss_command(ses)
     struct session *ses;
{
  char temp[80] = "";
  int i;

  for (i = 0; i < 50; i++) {
    sprintf(temp, "parsed %d branches starting from node %d, %d nodes", i, 50 - i, 23 + i * 3 / 2);
    tintin_puts2(temp, (struct session *)NULL);
  }
  getchar();			/* stop screen from scrolling stuff */
}

/*********************/
/* the #char command */
/*********************/
void char_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char strng[80];

  get_arg_in_braces(arg, arg, 1);
  if (ispunct(*arg)) {
    tintin_char = *arg;
    sprintf(strng, "#OK. TINTIN-CHAR is now {%c}\n", tintin_char);
    tintin_puts2(strng, (struct session *)NULL);
  } else
    tintin_puts2("#SPECIFY A PROPER TINTIN-CHAR! SOMETHING LIKE # OR /!", (struct session *)NULL);
}


/*********************/
/* the #echo command */
/*********************/
void echo_command(ses)
     struct session *ses;
{
  Echo = !Echo;
  if (Echo)
    tintin_puts("#ECHO IS NOW ON.", ses);
  else
    tintin_puts("#ECHO IS NOW OFF.", ses);
}

/*********************/
/* the #end command */
/*********************/
void end_command(command, ses)
     char *command;
     struct session *ses;
{
  struct session *sp;

  if (strcmp(command, "end"))
    tintin_puts("#YOU HAVE TO WRITE #end - NO LESS, TO END!", ses);
  else {
    struct session *sesptr;

    for (sesptr = sessionlist; sesptr; sesptr = sp) {
      sp = sesptr->next;
      cleanup_session(sesptr);
    }
    ses = NULL;
    tintin_puts2("TINTIN suffers from bloodlack, and the lack of a beating heart...", ses);
   tintin_puts2("TINTIN is dead! R.I.P.", ses);
   tintin_puts2("Your blood freezes as you hear TINTIN's death cry.", ses);
    user_done();
    exit(0);
  }
}

/***********************/
/* the #ignore command */
/***********************/
void ignore_command(ses)
     struct session *ses;
{
  if (ses) {
    if (ses->ignore = !ses->ignore)
      tintin_puts("#ACTIONS ARE IGNORED FROM NOW ON.", ses);
    else
      tintin_puts("#ACTIONS ARE NO LONGER IGNORED.", ses);
  } else
    tintin_puts("#No session active => Nothing to ignore!", ses);
}

/**********************/
/* the #presub command */
/**********************/
void presub_command(ses)
     struct session *ses;
{
  presub = !presub;
  if (presub)
    tintin_puts("#ACTIONS ARE NOW PROCESSED ON SUBSTITUTED BUFFER.", ses);
  else
    tintin_puts("#ACTIONS ARE NO LONGER DONE ON SUBSTITUTED BUFFER.", ses);
}

/**********************/
/* the #blank command */
/**********************/
void blank_command(ses)
     struct session *ses;
{
  blank = !blank;
  if (blank)
    tintin_puts("#INCOMING BLANK LINES ARE NOW DISPLAYED.", ses);
  else
    tintin_puts("#INCOMING BLANK LINES ARE NO LONGER DISPLAYED.", ses);
}

/**************************/
/* the #togglesubs command */
/**************************/
void togglesubs_command(ses)
     struct session *ses;
{
  togglesubs = !togglesubs;
  if (togglesubs)
    tintin_puts("#SUBSTITUTES ARE NOW IGNORED.", ses);
  else
    tintin_puts("#SUBSTITUTES ARE NO LONGER IGNORED.", ses);
}

/**************************/
/* the #mudcolors command */
/**************************/
void mudcolors_command(ses)
     struct session *ses;
{
  mudcolors = !mudcolors;
  if (mudcolors)
    tintin_puts("#MUD-colors (substituting ~n~ for colors) enabled.", ses);
  else
    tintin_puts("#MUD-colors function disabled.", ses);
}


/***********************/
/* the #showme command */
/***********************/
void showme_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char result[BUFFER_SIZE], strng[BUFFER_SIZE];

  get_arg_in_braces(arg, arg, 1);
  prepare_actionalias(arg, result, ses);
  sprintf(strng, "%s", result);
  tintin_puts2(strng, ses);	/* KB: no longer check for actions */
  /*sprintf(strng,"%s\n",result);
     if(ses->logfile)
     fwrite(strng, strlen(strng), 1, ses->logfile); */
}

/***********************/
/* the #loop command   */
/***********************/
void loop_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE];
  char result[BUFFER_SIZE];
  int flag, bound1, bound2, counter;

  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  flag = 1;
  substitute_vars(left, result, ses);
  substitute_myvars(result, left, ses);
  if (sscanf(left, "%d,%d", &bound1, &bound2) != 2)
    tintin_puts2("#Wrong number of arguments in #loop", ses);
  else {
    flag = 1;
    counter = bound1;
    while (flag == 1) {
      sprintf(vars[0], "%d", counter);
      substitute_vars(right, result);
      parse_input(result, ses);
      if (bound1 < bound2) {
	counter++;
	if (counter > bound2)
	  flag = 0;
      } else {
	counter--;
	if (counter < bound2)
	  flag = 0;
      }
    }
  }
}

/*************************/
/* the #select command   */
/*************************/
void select_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], mid[BUFFER_SIZE], right[BUFFER_SIZE];
  char result[BUFFER_SIZE];
  struct listnode *ln;
  ln = (ses) ? ses->myvars : common_myvars;


  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, mid, 0);
  arg = get_arg_in_braces(arg, right, 0);
  if(!*left || !*mid || !*right) {
    tintin_puts2("#select <pattern> <pattern> <expr>", ses);
  }
  substitute_vars(left, result, ses);
  substitute_myvars(result, left, ses);
  substitute_vars(mid, result, ses);
  substitute_myvars(result, mid, ses);
  while((ln = search_node_with_wild(ln, left)) != NULL) {
    if(match(ln->left, mid)) {
      sprintf(vars[0], "%s", ln->right);
      substitute_vars(right, result);
      parse_input(result, ses);
      return;
    }
  }
  arg = get_arg_in_braces(arg, left, 0); /* nothing matched, if command */
  if(*left == tintin_char) {
    if(is_abrev(left + 1, "else")) {
      arg = get_arg_in_braces(arg, right, 1);
      substitute_vars(right, result);
      substitute_myvars(result, right, ses);
      parse_input(right, ses);
    }
    else if(is_abrev(left + 1, "elif"))
      if_command(arg, ses);
  }
}

/**************************/
/* the #foreach command   */
/**************************/
void foreach_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE];
  char result[BUFFER_SIZE];
  struct listnode *ln;
  ln = (ses) ? ses->myvars : common_myvars;


  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  if(!*left || !*right) {
    tintin_puts2("#foreach <pattern> <expr>", ses);
  }
  substitute_vars(left, result, ses);
  substitute_myvars(result, left, ses);
/*  tintin_puts2(left, ses);
  tintin_puts2(right, ses);*/
  while((ln = search_node_with_wild(ln, left)) != NULL) {
    sprintf(vars[0], "%s", ln->right);
    substitute_vars(right, result);
    parse_input(result, ses);
  }
}

/************************/
/* the #message command */
/************************/
void message_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char offon[2][20];
  int mestype;
  char ms[7][20], tpstr[80];

  sscanf("aliases actions substitutes antisubstitutes highlights variables all",
	 "%s %s %s %s %s %s %s", ms[0], ms[1], ms[2], ms[3], ms[4], ms[5], ms[6]);
  strcpy(offon[0], "OFF.");
  strcpy(offon[1], "ON.");
  get_arg_in_braces(arg, arg, 1);
  if (!*arg)
  {
  	for (mestype=0;mestype<6;++mestype)
  	{
	    sprintf(tpstr, "#Messages concerning %s are %s",
	         ms[mestype], offon[mesvar[mestype]]);
            tintin_puts2(tpstr, ses);
  	};
  	return;
  };
  
  mestype = 0;
  while (!is_abrev(arg, ms[mestype]) && mestype < 7)
    mestype++;
  if (mestype == 7)
    tintin_puts2("#Invalid message type to toggle.", ses);
  else {
    if (mestype<6)
    {
	mesvar[mestype] = !mesvar[mestype];
    	sprintf(tpstr, "#Ok. messages concerning %s are now %s",
	    ms[mestype], offon[mesvar[mestype]]);
    	tintin_puts2(tpstr, ses);
    }
    else
    {
	int b=1;
	for (mestype=0;mestype<6;mestype++)
	    if (mesvar[mestype])	/* at least one type is ON? */
		b=0;				/* disable them all */
	for (mestype=0;mestype<6;mestype++)
	    mesvar[mestype]=b;
	if (b)
	    tintin_puts2("#Ok. All messages are now ON.");
	else
	    tintin_puts2("#Ok. All messages are now OFF.");
    }
  }
}

/**********************/
/* the #snoop command */
/**********************/
void snoop_command(arg, ses)
     char *arg;
     struct session *ses;
{
  char buf[100];
  struct session *sesptr = ses;

  if (ses) {
    get_arg_in_braces(arg, arg, 1);
    if (*arg) {
      for (sesptr = sessionlist; sesptr && strcmp(sesptr->name, arg); sesptr = sesptr->next) ;
      if (!sesptr) {
	tintin_puts("#NO SESSION WITH THAT NAME!", ses);
	return;
      }
    }
    if (sesptr->snoopstatus) {
      sesptr->snoopstatus = FALSE;
      sprintf(buf, "#UNSNOOPING SESSION '%s'", sesptr->name);
      tintin_puts(buf, ses);
    } else {
      sesptr->snoopstatus = TRUE;
      sprintf(buf, "#SNOOPING SESSION '%s'", sesptr->name);
      tintin_puts(buf, ses);
    }
  } else
    tintin_puts("#NO SESSION ACTIVE => NO SNOOPING", ses);
}

/**************************/
/* the #speedwalk command */
/**************************/
void speedwalk_command(ses)
     struct session *ses;
{
  speedwalk = !speedwalk;
  if (speedwalk)
    tintin_puts("#SPEEDWALK IS NOW ON.", ses);
  else
    tintin_puts("#SPEEDWALK IS NOW OFF.", ses);
}


/***********************/
/* the #status command */
/***********************/
void status_command(char *arg,struct session *ses)
{
	if (ses!=activesession)
		return;
	get_arg_in_braces(arg,arg,1);
	if (*arg)
	{
		char buf1[BUFFER_SIZE],buf2[BUFFER_SIZE];
		substitute_vars(arg,buf1);
		substitute_myvars(buf1,buf2,ses);
		strncpy(status,buf2,BUFFER_SIZE);
	}
	else
		strcpy(status,".");
	show_status();
}


/***********************/
/* the #system command */
/***********************/
void system_command(arg, ses)
     char *arg;
     struct session *ses;
{
  get_arg_in_braces(arg, arg, 1);
  if (*arg) {
    tintin_puts3("^#OK EXECUTING SHELL COMMAND.", ses);
    user_pause();
    system(arg);
    user_resume();
    tintin_puts3("!#OK COMMAND EXECUTED.", ses);
  } else
    tintin_puts2("#EXECUTE WHAT COMMAND?", ses);
  prompt(NULL);

}


/********************/
/* the #zap command */
/********************/
struct session *zap_command(ses)
     struct session *ses;
{
  tintin_puts("#ZZZZZZZAAAAAAAAPPPP!!!!!!!!! LET'S GET OUTTA HERE!!!!!!!!", ses);
  if (ses) {
    cleanup_session(ses);
    return newactive_session();
  } else {
    end_command("end", (struct session *)NULL);
  }
}

void news_command(ses)
struct session *ses;
{
    char line[BUFFER_SIZE];
    FILE* news=fopen("news","r");
    if (news)
    {
	textout("\033[32;2m");
	while (fgets(line,BUFFER_SIZE,news))
	    textout(line);
	textout("\033[0;37;2m\n");
    }
    else
	textout("`news' file not found!\n");
    prompt(ses);
}


/************************/
/* the #wizlist command */
/************************/
/*
   void wizlist_command(ses)
   struct session *ses;
   {
   tintin_puts2("==========================================================================", ses);
   tintin_puts2("                           Implementor:", ses);
   tintin_puts2("                              Valgar ", ses);
   tintin_puts2("", ses);
   tintin_puts2("          Special thanks to Grimmy for all her help :)", ses);
   tintin_puts2("\n\r                         TINTIN++ testers:", ses);
   tintin_puts2(" Nemesis, Urquan, Elvworn, Kare, Merjon, Grumm, Tolebas, Winterblade ", ses);
   tintin_puts2("\n\r A very special hello to Zoe, Oura, GODDESS, Reyna, Randela, Kell, ", ses);
   tintin_puts2("                  and everyone else at GrimneDIKU\n\r", ses);
   tintin_puts2("==========================================================================", ses);
   prompt(ses);
   }
 */

void wizlist_command(ses)
     struct session *ses;
{

  tintin_puts2("==========================================================================", ses);
  tintin_puts2("  There are too many people to thank for making tintin++ into one of the", ses);
  tintin_puts2("finest clients available.  Those deserving mention though would be ", ses);
  tintin_puts2("Peter Unold, Bill Reiss, Joann Ellsworth, Jeremy Jack, and the many people", ses);
  tintin_puts2("who send us bug reports and suggestions.", ses);
  tintin_puts2("            Enjoy!!!  And keep those suggestions coming in!!\n", ses);
  tintin_puts2("                       The Management...", ses);
  tintin_puts2("==========================================================================", ses);
}


/*********************************************************************/
/*   tablist will display the all items in the tab completion file   */
/*********************************************************************/
void tablist(tcomplete)
     struct completenode *tcomplete;
{
  int count, done;
  char tbuf[BUFFER_SIZE];
  struct completenode *tmp;

  done = 0;
  if (tcomplete == NULL) {
    tintin_puts2("Sorry.. But you have no words in your tab completion file", NULL);
    return;
  }
  count = 1;
  *tbuf = '\0';

/* 
   I'll search through the entire list, printing thre names to a line then
   outputing the line.  Creates a nice 3 column effect.  To increase the # 
   if columns, just increase the mod #.  Also.. decrease the # in the %s's
 */

  for (tmp = tcomplete->next; tmp != NULL; tmp = tmp->next) {
    if ((count % 3)) {
      if (count == 1)
	sprintf(tbuf, "%25s", tmp->strng);
      else
	sprintf(tbuf, "%s%25s", tbuf, tmp->strng);
      done = 0;
      ++count;
    } else {
      sprintf(tbuf, "%s%25s", tbuf, tmp->strng);
      tintin_puts2(tbuf, NULL);
      done = 1;
      *tbuf = '\0';
      ++count;
    }
  }
  if (!done)
    tintin_puts2(tbuf, NULL);
  prompt(NULL);
}

void tab_add(arg)
     char *arg;
{
  struct completenode *tmp, *tmpold, *tcomplete;
  struct completenode *newt;
  char *newcomp, buff[BUFFER_SIZE];

  tcomplete = complete_head;

  if ((arg == NULL) || (strlen(arg) <= 0)) {
    tintin_puts("Sorry, you must have some word to add.", NULL);
    prompt(NULL);
    return;
  }
  get_arg_in_braces(arg, buff, 1);

  if ((newcomp = (char *)(malloc(strlen(buff) + 1))) == NULL) {
    user_done();
    fprintf(stderr, "Could not allocate enough memory for that Completion word.\n");
    exit(1);
  }
  strcpy(newcomp, buff);
  tmp = tcomplete;
  while (tmp->next != NULL) {
    tmpold = tmp;
    tmp = tmp->next;
  }

  if ((newt = (struct completenode *)(malloc(sizeof(struct completenode)))) == NULL) {
    user_done();
    fprintf(stderr, "Could not allocate enough memory for that Completion word.\n");
    exit(1);
  }
  newt->strng = newcomp;
  newt->next = NULL;
  tmp->next = newt;
  tmp = newt;
  sprintf(buff, "#New word %s added to tab completion list.", arg);
  tintin_puts(buff, NULL);
  prompt(NULL);
}

void tab_delete(arg)
     char *arg;
{
  struct completenode *tmp, *tmpold, *tmpnext, *tcomplete;
  char s_buff[BUFFER_SIZE], c_buff[BUFFER_SIZE];

  tcomplete = complete_head;

  if ((arg == NULL) || (strlen(arg) <= 0)) {
    tintin_puts("#Sorry, you must have some word to delete.", NULL);
    prompt(NULL);
    return;
  }
  get_arg_in_braces(arg, s_buff, 1);
  tmp = tcomplete->next;
  tmpold = tcomplete;
  if (tmpold->strng == NULL) {	/* (no list if the second node is null) */
    tintin_puts("#There are no words for you to delete!", NULL);
    prompt(NULL);
    return;
  }
  strcpy(c_buff, tmp->strng);
  while ((tmp->next != NULL) && (strcmp(c_buff, s_buff) != 0)) {
    tmpold = tmp;
    tmp = tmp->next;
    strcpy(c_buff, tmp->strng);
  }
  if (tmp->next != NULL) {
    tmpnext = tmp->next;
    tmpold->next = tmpnext;
    free(tmp);
    tintin_puts("#Tab word deleted.", NULL);
    prompt(NULL);
  } else {
    if (strcmp(c_buff, s_buff) == 0) {	/* for the last node to delete */
      tmpold->next = NULL;
      free(tmp);
      tintin_puts("#Tab word deleted.", NULL);
      prompt(NULL);
      return;
    }
    tintin_puts("Word not found in list.", NULL);
    prompt(NULL);
  }
}

void display_info(ses)
     struct session *ses;
{
  char buf[BUFFER_SIZE];
  int actions = 0;
  int aliases = 0;
  int vars = 0;
  int subs = 0;
  int antisubs = 0;
  int highs = 0;
  int ignore;

  actions = count_list((ses) ? ses->actions : common_actions);
  aliases = count_list((ses) ? ses->aliases : common_aliases);
  subs = count_list((ses) ? ses->subs : common_subs);
  antisubs = count_list((ses) ? ses->antisubs : common_antisubs);
  vars = count_list((ses) ? ses->myvars : common_myvars);
  highs = count_list((ses) ? ses->highs : common_highs);
  if (ses)
    ignore = ses->ignore;
  else
    ignore = 0;

  tintin_puts2("You have defined the following:", ses);
  sprintf(buf, "Actions : %d", actions);
  tintin_puts2(buf, ses);
  sprintf(buf, "Aliases : %d", aliases);
  tintin_puts2(buf, ses);
  sprintf(buf, "Substitutes : %d", subs);
  tintin_puts2(buf, ses);
  sprintf(buf, "Antisubstitutes : %d", antisubs);
  tintin_puts2(buf, ses);
  sprintf(buf, "Variables : %d", vars);
  tintin_puts2(buf, ses);
  sprintf(buf, "Highlights : %d", highs);
  tintin_puts2(buf, ses);
  sprintf(buf, "Echo : %d (1 - on, 0 - off)    Speedwalking : %d     Blank : %d", Echo, speedwalk, blank);
  tintin_puts2(buf, ses);
  sprintf(buf, "Toggle Subs: %d   Ignore Actions : %d   PreSub-ing: %d  MUD ~n~ colors: %d", togglesubs, ignore, presub,mudcolors);
  tintin_puts2(buf, ses);


  prompt(ses);
}
