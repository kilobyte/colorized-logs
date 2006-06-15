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

struct colordef
{
	int num;
	char *name;
} cNames[]=
{
	{ 0,"black"},
	{ 1,"blue"},
	{ 2,"green"},
	{ 3,"cyan"},
	{ 4,"red"},
	{ 5,"magenta"},
	{ 6,"brown"},
	{ 7,"grey"},
	{ 7,"gray"},
	{ 7,"normal"},
	{ 8,"charcoal"},
	{ 9,"light blue"},
	{10,"light green"},
	{11,"light cyan"},
	{12,"light red"},
	{13,"light magenta"},
	{14,"yellow"},
	{15,"white"},
	{15,"bold"},
	{-1,""},
};

int get_high(char *hig)
{
	char *e;
	int code;
	
	if (!*hig)
		return(-1);
	code=strtol(hig,&e,0);
	if ((!*e)&&(code>=0)&&(code<16))
		return(code);
	for (code=0;cNames[code].num!=-1;code++)
		if (is_abrev(hig,cNames[code].name))
			return(cNames[code].num);
	return(-1);
}


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
      colflag = get_high(tmp3)!=-1;
      tmp1 = tmp2 + 1;
    }
    if (colflag == TRUE) {

      if ((ln = searchnode_list(myhighs, right)) != NULL)
	deletenode_list(myhighs, ln);
      sprintf(tmp3,"%d",BUFFER_SIZE-strlen(right));
      insertnode_list(myhighs, right, left,tmp3,PRIORITY);
      hinum++;
      if (mesvar[4]) {
	sprintf(result, "#Ok. {%s} is now highlighted %s.", right, left);
	tintin_puts2(result, ses);
      }
    } else {
      int i;
      tintin_puts2("Invalid highlighting color, valid colors are:", ses);
      tmp3[0]=0;
      tmp1=tmp3;
      for (i=0;cNames[i].num!=-1;i++) {
        sprintf(left,"%s~7~,",cNames[i].name);
        if (cNames[i].num)
          tmp1+=sprintf(tmp1,"~%i~%-20s ",cNames[i].num,left);
        else
          tmp1+=sprintf(tmp1,"~7~%-20s ",left);
      	if ((i%4)==3) {
      	  tintin_puts2(tmp3,ses);
      	  tmp3[0]=0;
      	  tmp1=tmp3;
      	}
      };
      strcpy(tmp1,"or 0..15");
      tintin_puts2(tmp3,ses);
    }


  }
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
      sprintf(temp2,"~%d~%s~7~",get_high(ln->right),temp);
      *(place) = '\0';
      tptr = line2;
      sprintf(result, "%s%s%s", line, temp2, tptr);
      strcpy(line, result);
    }
  }
}

void do_all_high(char *line,struct session *ses)
{
	char text[BUFFER_SIZE];
	int attr[BUFFER_SIZE];
	int c;
	char *pos,*txt;
	int *atr;
	int l,r,i;
	struct listnode *ln;
	
	c=-1;
	txt=text;
	atr=attr;
	for (pos=line;*pos;pos++)
	{
		if (*pos=='~')
			if ((*(pos+1)=='1')&&(*(pos+2)>='0')&&(*(pos+2)<'6')
			    &&(*(pos+3)=='~'))
			{
				c=*(pos+2)+10-'0';
				pos+=3;
				goto color;
			}
			else if ((*(pos+1)>='0')&&(*(pos+1)<='9')&&(*(pos+2)=='~'))
			{
				c=*(pos+1)-'0';
				pos+=2;
				goto color;
			};
		*txt++=*pos;
		*atr++=c;
		continue;
	color:
	};
	*txt=0;
	*atr=c;
	ln=ses->highs;
	while (ln=ln->next)
	{
		txt=text;
		while (*txt&&find(txt,ln->left,&l,&r))
		{
			c=get_high(ln->right);
			r+=txt-text;
			l+=txt-text;
			/* changed: no longer highligh in half of a word */
			if (((l==0)||!isalpha(text[l-1]))&&
			    !isalpha(text[r+1]))
				for (i=l;i<=r;i++)
					attr[i]=c;
			txt=text+r+1;
		}
	};
	c=-1;
	txt=text;
	atr=attr;
	while (*txt)
	{
		if (c!=*atr)
			line+=sprintf(line,"~%d~",c=*atr);
		*line++=*txt++;
		atr++;
	};
	if (c!=*atr)
		line+=sprintf(line,"~%d~",c=*atr);
	*line=0;
}

