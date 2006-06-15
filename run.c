#include <tintin.h>
#include <stdio.h>
#include <pty.h>
extern char **environ;

int run(char *command)
{
	int fd;
	
	struct termios ta;
	ta.c_iflag=0;
	ta.c_oflag=0;
	ta.c_cflag=0;
	ta.c_lflag=0;
	cfmakeraw(&ta);
	
	switch(forkpty(&fd,0,&ta,0))
	{
		case -1:
			return(0);
		case 0:
		{
			char *argv[4];
			argv[0]="sh";
			argv[1]="-c";
			argv[2]=command;
			argv[3]=0;
			execve("/bin/sh",argv,environ);
			fprintf(stderr,"#ERROR: Couldn't exec `%s'\n",command);
			exit(127);
		}
		default:
			return(fd);
	}
}
