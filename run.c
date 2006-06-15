#include <arpa/inet.h>
#include <stdio.h>
extern char **environ;

int run(char *command)
{
	int p[2];
	if (socketpair(AF_UNIX,SOCK_STREAM,0,p))
		return(0);
	switch(fork())
	{
		case -1:
			close(p[0]);
			close(p[1]);
			return(0);
		case 0:
		{
			char *argv[4];
			argv[0]="sh";
			argv[1]="-c";
			argv[2]=command;
			argv[3]=0;
			close(p[0]);
			if ((dup2(p[1],0)==-1)||(dup2(p[1],1)==-1)||(dup2(p[1],2)==-1))
			{
				write(p[1],"#ERROR: dup2() in #run failed\n",30);
				exit(127);	/* already fork()ed */
			};
			close(p[1]);
			execve("/bin/sh",argv,environ);
			fprintf(stderr,"#ERROR: Couldn't exec `%s'\n",command);
			exit(127);
		}
		default:
			close(p[1]);
			return(p[0]);
	}
}
