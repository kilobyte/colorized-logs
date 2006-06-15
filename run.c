#include <stdio.h>
#include <pty.h>
extern char **environ;

int run(char *command)
{
	int fd;
	switch(forkpty(&fd,0,0,0))
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
