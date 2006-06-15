#include "tintin.h"

extern int colors[];

/*************************************************************************/
/* color definitions on your favourite MUD                               */
/* TODO: this table should be customizable without recompiling KBtin ... */
/*************************************************************************/
char *MUDcolors[]={
	"$0$",		/* black - not present */
	"$BLU$",
	"$GRN$",
	"$CYN$",
	"$RED$",
	"$MAG$",
	"$YEL$",
	"$0$",
	"$0$",		/* gray - not present */
	"$HIB$",
	"$HIG$",
	"$HIC$",
	"$HIR$",
	"$HIM$",
	"$HIY$",
	"$BOLD$"};

int ccolor=7;

void do_in_MUD_colors(char *txt)
{
	char *out,*back;
	
	for (out=txt;*txt;txt++)
		switch(*txt)
		{
			case 27:
				if (*(txt+1)=='[')
				{
					back=txt++;
				again:
					switch (*++txt)
					{
						case 0:
							txt=back;
							break;
						case ';':
							goto again;
						case 'm':
							*out++='~';
							if (ccolor>9)
							    *out++='1';
							*out++=ccolor%10+'0';
							*out++='~';
							break;
						case '0':
							ccolor=7;
							goto again;
						case '1':
							ccolor|=8;
							goto again;
						case '2':
							ccolor&=7;
							goto again;
						case '3':
							if (!*++txt)
								{txt=back;break;};
							if ((*txt>'0')&&(*txt<'8'))
							{
								ccolor=ccolor&8|colors[*txt-'0'];
								goto again;
							};
						case '4':
							if (!*++txt)
								{txt=back;break;};
							if ((*txt>'0')&&(*txt<'8'))
							{
								/* ignore */
								goto again;
							};
						default:
							txt=back;
					};
				};
				break;
			default:
				*out++=*txt;
		};
	*out=0;
}



void do_out_MUD_colors(char *line)
{
	char buf[BUFFER_SIZE];
	char *txt=buf,*pos=line;
	int c;
	
	for (;*pos;pos++)
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
		continue;
	color:
		strcpy(txt,MUDcolors[c]);
		txt+=strlen(MUDcolors[c]);
	};
	*txt=0;
	strcpy(line,buf);
}
