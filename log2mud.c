#include <stdio.h>

char *cnames[16]={"$0$","$BLU$","$GRN$","$CYN$","$RED$","$MAG$","$YEL$","$0$",
                 "$0$","$HIB$","$HIG$","$HIC$","$HIR$","$HIM$","$HIY$","$BOLD$"};
int cols[16]={0,4,2,6,1,5,3,7,8,12,10,14,9,13,11,15};

int main()
{
	int ch;
	int oldcolor=7, color=7;
normal:
	ch=getchar();
	if (ch==EOF)
		goto end;
	if (ch==27)
		goto esc;
	if ((color!=oldcolor))
	{
		if (oldcolor!=7)
			printf("$0$");
		oldcolor=color;
		fprintf(stderr,"(%d)\n",cols[color]);
		if (color!=7)
			printf("%s",cnames[cols[color]]);
	};
	if (ch)
		putchar(ch);
	goto normal;
esc:
	switch(ch=getchar())
	{
		case EOF:
			goto end;
		case '[':
			goto esc;
		case '0':
			color=7;
			break;
		case '1':
			color=color|8;
			break;
		case '2':
			color=color&7;
			break;
		case '3':
			color=(color&8)|(getchar()-'0');
	};
	goto igs;
igs:
	switch(ch=getchar())
	{
		case 'm':
			goto normal;
		case EOF:
			goto end;
		case ';':
			goto esc;
		default:
			goto igs;
	};
end:
	if (color!=7)
		printf("$0$");
	return(0);
}
