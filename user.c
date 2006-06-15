#include <tintin.h>
#include <signal.h>

#define B_LENGTH 4096
#define STATUS_COLOR COLOR_WHITE

char done_input[BUFFER_SIZE],k_input[BUFFER_SIZE],kh_input[BUFFER_SIZE],out_line[BUFFER_SIZE];
int k_len,k_pos,k_spos;
int o_len,o_pos,o_color,o_oldcolor;
int b_first,b_current,b_screenb;
char *b_output[B_LENGTH];
WINDOW *w_in,*w_out,*w_status;
int hist_num;
int need_resize;

extern char status[BUFFER_SIZE];

void countpos()
{
	k_pos=k_len=strlen(k_input);
	k_spos=0;
}

void redraw_in()
{
	wmove(w_in,0,0);
	/*
	if (k_len<COLS)
	{
		k_spos=0;
		waddstr(w_in,k_input);
		wclrtoeol(w_in);
		wmove(w_in,0,k_pos);
		return;
	};
	*/	
	if (k_pos<k_spos)
		k_spos=k_pos;
	if (k_pos-k_spos+(k_spos!=0)>=COLS)
		k_spos=k_pos+2-COLS;
	if (k_spos)
		waddch(w_in,'<'|A_BOLD);
	waddnstr(w_in,&(k_input[k_spos]),COLS-2+(k_spos==0));
	if (k_spos+COLS-2<k_len)
		waddch(w_in,'>'|A_BOLD);
	else
		wclrtoeol(w_in);
	wmove(w_in,0,(k_spos!=0)+k_pos-k_spos);
}

void redraw_status()
{
	char *pos;
	int c;
	
	if (!w_status)
		return;
	werase(w_status);
	wmove(w_status,0,0);
	wattrset(w_status,COLOR_PAIR(9+(STATUS_COLOR==COLOR_BLACK?7:0)));
	if (!*(pos=status))
		return;
	for (;*pos;pos++)
	{
		if (sscanf(pos,"~%u~",&c)!=1)
		    goto nomatch;
		if ((c<0)||(c>15)||(!isdigit(*++pos)))
		{
		    pos--;
		    goto nomatch;
		};
		if (*++pos!='~')
		    if (*++pos!='~')
		    {
			    pos-=2;
			    goto nomatch;
		    };
		wattrset(w_status,((c&8)?A_BOLD:0)|COLOR_PAIR(9+(c&7)));
		continue;
	nomatch:
		waddch(w_status,*pos);
	}
}

int b_shorten()
{
	if (b_first>b_current)
		return(FALSE);
	free(b_output[b_first++%B_LENGTH]);
	return(TRUE);
}

void redraw_out(char *pos)
{
	for (;*pos;pos++)
	{
		if (*pos=='~')
			if ((*(pos+1)=='1')&&(*(pos+2)>='0')&&(*(pos+2)<'6')
			    &&(*(pos+3)=='~'))
			{
				wattrset(w_out,A_BOLD|COLOR_PAIR(1+((*(pos+2)+2-'0')&7)));
				pos+=3;
				continue;
			}
			else if ((*(pos+1)>='0')&&(*(pos+1)<='9')&&(*(pos+2)=='~'))
			{
				wattrset(w_out,(*(pos+1)>'7'?A_BOLD:0)
						|COLOR_PAIR(1+((*(pos+1)-'0')&7)));
				pos+=2;
				continue;
			};
		waddch(w_out,*pos);
	}
}

void b_scroll(int b_to)
{
	int y;
	if (b_screenb==(b_to=(b_to<b_first?b_first:(b_to>b_current?b_current:b_to))))
		return;
	wclear(w_out);
	scrollok(w_out,FALSE);
	for (y=b_to-LINES+(w_status?3:2);y<=b_to;++y)
		if ((y<1)||(y<=b_first))
			waddch(w_out,'\n');
		else
			if (y<b_current)
				redraw_out(b_output[y%B_LENGTH]);
			else
				redraw_out(out_line);
	scrollok(w_out,TRUE);		
	/*		
	sprintf(done_input,"\031b_to=%i, b_screenb=%i, b_first=%i, b_current=%i\027",b_to,b_screenb,b_first,b_current);
	redraw_out(done_input);
	*/
	
	if (b_screenb==b_current)
	{
		int x;
		leaveok(w_in,TRUE);
		wmove(w_in,0,0);
		for (x=0;x<COLS;++x)
			waddch(w_in,'^'|A_BOLD);
	};
	if ((b_screenb=b_to)==b_current)
	{
		leaveok(w_in,FALSE);
		redraw_in();
	};
	wrefresh(w_out);
	wrefresh(w_in);
}

void b_addline()
{
	char *new;
	while (!(new=(char*)malloc(o_len+1)))
		if (!b_shorten())
			syserr("Out of memory");
	strcpy(new,out_line);
	if (b_current==b_first+B_LENGTH)
		b_shorten();
	b_output[b_current%B_LENGTH]=new;
	o_len=o_color<10 ? 3:4;
	o_pos=0;
	sprintf(out_line,"~%d~",o_oldcolor=o_color);
	b_current++;
	if (b_current==b_screenb+1)
	{
		++b_screenb;
		redraw_out(out_line);
	}
}

const int colors[8]={0,4,2,6,1,5,3,7};

void textout(char *txt)
{
	for (;*txt;txt++)
		switch(*txt)
		{
			case 27:
				if (*(txt+1)=='[')
				{
					++txt;
				again:
					switch (*++txt)
					{
						case 0:
							--txt;
							break;
						case ';':
							goto again;
						case 'm':
							break;
						case '0':
							o_color=o_color&8|7;
							goto again;
						case '1':
							o_color|=8;
							goto again;
						case '2':
							o_color&=7;
							goto again;
						case '3':
							if (!*++txt)
								{--txt;break;};
							if ((*txt>'0')&&(*txt<'8'))
							{
								o_color=o_color&8|colors[*txt-'0'];
								goto again;
							}
							else
								--txt;
							break;		
						default:
							--txt;
					};
					break;
				};
				break;
			case '\r':
				break;
			case '\n':
				out_line[o_len]='\n';
				out_line[++o_len]=0;
				if (b_screenb==b_current)
					redraw_out("\n");
				b_addline();
				break;
			case '~':
				if (*(txt+1)=='1')
				        if ((*(txt+2)>='0')&&(*(txt+2)<'6')
					    &&(*(txt+3)=='~'))
					{
						o_color=*(txt+2)+10-'0';
						txt+=3;
						break;    
					};
				if ((*(txt+1)>='0')&&(*(txt+1)<='9')&&
				    (*(txt+2)=='~'))
				{
					o_color=*(txt+1)-'0';
					txt+=2;
					break;
				};
				/* fall through */
			default:
				{
					int o=o_len;
					if (o_oldcolor!=o_color)
					{
						out_line[o_len++]='~';
						if ((o_oldcolor=o_color)>9)
						    out_line[o_len++]='1';
						out_line[o_len++]=o_color%10+'0';
						out_line[o_len++]='~';
					};
					out_line[o_len]=*txt;
					out_line[++o_len]=0;
					++o_pos;
					if (b_screenb==b_current)
						redraw_out(out_line+o);
				};
				if (o_pos==COLS)
					b_addline();
		};
	wrefresh(w_out);
	wrefresh(w_in);
}

int process_kbd(struct session *ses)
{
	int ch=getch();
	switch(ch)
	{
		case ERR:	//no key pressed
			return(0);
		case '\n':
		case '\r':
		case KEY_ENTER:
			if (b_current!=b_screenb)
				b_scroll(b_current);
		
			strcpy(done_input,k_input);
			k_len=k_pos=k_spos=0;
			k_input[0]=0;
			wmove(w_in,0,0); // wclear() had a stupid effect of a flashing
			wclrtoeol(w_in); // blue line going across screen
			wrefresh(w_in);
			return(1);
		case KEY_PPAGE:
			if (b_screenb>b_first+LINES-(w_status?3:2))
				b_scroll(b_screenb+(w_status?3:2)-LINES);
			else
			{
				beep();
				wrefresh(w_in);
			};
			break;
		case KEY_NPAGE:
			if (b_screenb<b_current)
				b_scroll(b_screenb+LINES-(w_status?3:2));
			else
			{
				beep();
				wrefresh(w_in);
			};
			break;
		case KEY_UP:
			if (!ses)
				break;
			if (hist_num==HISTORY_SIZE-1)
				break;
			if (!ses->history[hist_num+1])
				break;
			if (hist_num==-1)
				strcpy(kh_input,k_input);
			strcpy(k_input,ses->history[++hist_num]);
			countpos();
			redraw_in();
			wrefresh(w_in);
			break;
		case KEY_DOWN:
			if (!ses)
				break;
			if (hist_num==-1)
				break;
			do --hist_num;
			while (hist_num&&!(ses->history[hist_num]));
			if (hist_num==-1)
				strcpy(k_input,kh_input);
			else
				strcpy(k_input,ses->history[hist_num]);
			countpos();
			redraw_in();
			wrefresh(w_in);
			break;
		case KEY_LEFT:
			if (b_current!=b_screenb)
				b_scroll(b_current);
			if (k_pos==0)
				break;
			--k_pos;
			if (k_pos>=k_spos)
				wmove(w_in,0,k_pos-k_spos+(k_spos!=0));
			else
				redraw_in();
			wrefresh(w_in);
			break;
		case KEY_RIGHT:
			if (b_current!=b_screenb)
				b_scroll(b_current);
			if (k_pos==k_len)
				break;
			++k_pos;
			if (k_pos<=k_spos+COLS-2)
				wmove(w_in,0,k_pos-k_spos+(k_spos!=0));
			else
				redraw_in();
			wrefresh(w_in);
			break;
		case KEY_HOME:
			if (b_current!=b_screenb)
				b_scroll(b_current);
			if (!k_pos)
				break;
			k_pos=0;
			if (k_pos>=k_spos)
				wmove(w_in,0,k_pos-k_spos+(k_spos!=0));
			else
				redraw_in();
			wrefresh(w_in);
			break;
		case KEY_END:
			if (b_current!=b_screenb)
				b_scroll(b_current);
			if (k_pos==k_len)
				break;
			k_pos=k_len;
			if (k_pos<=k_spos+COLS-2)
				wmove(w_in,0,k_pos-k_spos+(k_spos!=0));
			else
				redraw_in();
			wrefresh(w_in);
			break;
		case 8:			/* ^H */
		case KEY_BACKSPACE:
			if (b_current!=b_screenb)
				b_scroll(b_current);
			if (k_pos==0)
				break;
			{
				int i;
				for (i=--k_pos;i<=k_len;++i)
					k_input[i]=k_input[i+1];
				--k_len;
			};
			redraw_in();
			wrefresh(w_in);
			break;
		default:
			if (b_current!=b_screenb)
				b_scroll(b_current);
			if (k_len==BUFFER_SIZE-1)
			{
				beep();
				wrefresh(w_in);
			}
			else
			{
				if ((ch<32)||(ch>255))
				{
					char txt[128];
					snprintf(txt,128,"unknown keycode %i\n",ch);
					textout(txt);
					break;
				};
                k_input[++k_len]=0;
                {
                	int i;
                	for (i=k_len-1;i>k_pos;--i)
                		k_input[i]=k_input[i-1];              	
                };
                k_input[k_pos++]=ch;
                if (((k_len+1)==k_pos)&&(k_len<COLS))
   	            	waddch(w_in,ch);
                else
                	redraw_in();
				wrefresh(w_in);
			}
	};
	return(0);
}

void user_beep()
{
	beep();
	refresh();
}

void curses_init()
{
	need_resize=0;
	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr,FALSE);
	keypad(stdscr,TRUE);
	noecho;
	if ((status[0]!='.')||status[1])
		w_status=newwin(1,COLS,LINES-1,0);
	else
		w_status=0;
	w_out=newwin(LINES-(w_status?2:1),COLS,0,0);
	w_in=newwin(1,COLS,LINES-(w_status?2:1),0);
	scrollok(w_out,TRUE);
	if (w_status)
		scrollok(w_status,FALSE);
	scrollok(w_in,FALSE);
	init_pair(1+0,COLOR_BLACK  ,COLOR_BLACK);
	init_pair(1+1,COLOR_BLUE   ,COLOR_BLACK);
	init_pair(1+2,COLOR_GREEN  ,COLOR_BLACK);
	init_pair(1+3,COLOR_CYAN   ,COLOR_BLACK);
	init_pair(1+4,COLOR_RED    ,COLOR_BLACK);
	init_pair(1+5,COLOR_MAGENTA,COLOR_BLACK);
	init_pair(1+6,COLOR_YELLOW ,COLOR_BLACK);
	init_pair(1+7,COLOR_WHITE  ,COLOR_BLACK);
	init_pair(9+0,COLOR_BLACK  ,STATUS_COLOR);
	init_pair(9+1,COLOR_BLUE   ,STATUS_COLOR);
	init_pair(9+2,COLOR_GREEN  ,STATUS_COLOR);
	init_pair(9+3,COLOR_CYAN   ,STATUS_COLOR);
	init_pair(9+4,COLOR_RED    ,STATUS_COLOR);
	init_pair(9+5,COLOR_MAGENTA,STATUS_COLOR);
	init_pair(9+6,COLOR_YELLOW ,STATUS_COLOR);
	init_pair(9+7,COLOR_WHITE  ,STATUS_COLOR);
	init_pair(17,COLOR_WHITE,COLOR_BLUE);
	wbkgd(w_out,COLOR_PAIR(1));
	wclear(w_out);
	if (w_status)
	{
		wbkgd(w_status,COLOR_PAIR(8+1));
		wclear(w_status);
	};
	wbkgd(w_in,COLOR_PAIR(17));
	wclear(w_in);
	refresh();
	wrefresh(w_in);
	leaveok(w_out,TRUE);
	if (w_status)
		leaveok(w_status,TRUE);
	leaveok(w_in,FALSE);
}

void sigwinch()
{
	need_resize=1;
}

void user_resize()
{
	delwin(w_in);
	delwin(w_out);
	if (w_status)
	{
		delwin(w_status);
		w_status=0;
	};
	endwin();
	curses_init();
	b_screenb=-666;		/* impossible value */
	b_scroll(b_current);
	{
		char line[BUFFER_SIZE];
		snprintf(line,BUFFER_SIZE,"New size: %ix%i\n",COLS,LINES);
		textout(line);
	};
	if (w_status)
		redraw_status();
	redraw_in();
	wrefresh(w_out);
	if (w_status)
		wrefresh(w_status);
	wrefresh(w_in);
}

void show_status()
{
	int st,w;
	st=((status[0]!='.')||status[1]);
	w=(w_status!=0);
	if (st!=w)
		user_resize();
	else
		if (st)
		{
			redraw_status();
			wrefresh(w_status);
		}
}

void user_init()
{	
	curses_init();

	k_len=0;
	k_pos=0;
	k_spos=0;
	k_input[0]=0;
	
	b_first=0;
	b_current=-1;
	b_screenb=-1;
	b_output[0]=out_line;
	out_line[0]=0;
	o_len=0;
	o_pos=0;
	o_color=16|7;
	o_oldcolor=16|7;
	textout("~12~KB~3~tin ~7~");
	textout(VERSION_NUM);
	textout(" by ~11~kilobyte@mimuw.edu.pl~9~\n");
	{
		int i;
		for (i=0;i<COLS;++i)
			done_input[i]='-';
		done_input[COLS]=0;
	};
	textout(done_input);
	if (!has_colors())
		textout("~7~\nYour terminal seems broken.\n"
		    "If you're using an xterm, I suggest copying /etc/terminfo/l/linux\n"
		    "(or a definition of another decent terminal) to ~/.terminfo/x/xterm\n");
	textout("~7~");
}

void user_pause()
{
	endwin();
}

void user_resume()
{
	doupdate();
}

void user_done()
{
	endwin();
	write(1,"\n",1);
}
