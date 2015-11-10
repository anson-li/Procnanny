#include "apue.h"
#include <sys/wait.h>

#define	DEF_PAGER	"/bin/more"		/* default pager program */

/*
	file initiates an fopen (generic) before forking.
	file sets a fork, separating parent and child
	parent is reading information from an argument and sends to a pipe
	the child checks if it's standard input
	line 60 states: make my 'read' pipe as same as the standard input. 
		so, any program run from here on thinks 
		that the program is running from standard input.
		but wait! if the duplicate is still not the standard input, then there's an error.
	so now, you can close the read pipe as everything else is coming in from standard input ...
	line 69: Condition 1: you can now set an environment variable for 'pager',
		which sets up the connection to the pager program.
		Condition 2: steps over the '/' character to find the 'more' in DEF_PAGER, which is the 
		application name.
	execl is similar to popen, but there's no pipe. This means execute and leave, which is to execute
		the process as given, provide it as standard out, then leave. this NEEDS the bash command.

*/


int main(int argc, char *argv[]) // read the file, whatever it has
{
	int		n;
	int		fd[2];
	pid_t	pid;
	char	*pager, *argv0;
	char	line[MAXLINE];
	FILE	*fp;

	if (argc != 2)
		err_quit("usage: a.out <pathname>");

	if ((fp = fopen(argv[1], "r")) == NULL)
		err_sys("can't open %s", argv[1]);
	if (pipe(fd) < 0)
		err_sys("pipe error");

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid > 0) {	/* parent */
		close(fd[0]);	/* close read end */

		/* parent copies argv[1] to pipe */
		while (fgets(line, MAXLINE, fp) != NULL) {
			n = strlen(line);
			if (write(fd[1], line, n) != n)
				err_sys("write error to pipe");
		}
		if (ferror(fp))
			err_sys("fgets error");

		close(fd[1]);	/* close write end of pipe for reader */

		if (waitpid(pid, NULL, 0) < 0)
			err_sys("waitpid error");
		exit(0);
	} else {										/* child */
		close(fd[1]);	/* close write end */
		if (fd[0] != STDIN_FILENO) { // is the pipe connected to standard input?
			if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO) // dup2 is standard duplication code ... therefore, if 
														   // the read file is a duplicate of
				err_sys("dup2 error to stdin");
			close(fd[0]);	/* don't need this after dup2 */
		}

		/* get arguments for execl() */
		if ((pager = getenv("PAGER")) == NULL)
			pager = DEF_PAGER; // "/bin/more" uses the executable and view the information
		if ((argv0 = strrchr(pager, '/')) != NULL)
			argv0++;		/* step past rightmost slash */
		else
			argv0 = pager;	/* no slash in pager */

		if (execl(pager, argv0, (char *)0) < 0)
			err_sys("execl error for %s", pager);
	}
	exit(0);
}
