#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

#define	 MY_PORT  2222

/* --------------------------------------------------------------------- */
/* This is a sample client program for the number server. The client and */
/* the server need not run on the same machine.				 */
/* --------------------------------------------------------------------- */

main() 
{
	int	s, number;

	struct	sockaddr_in	server;

	struct	hostent		*host;

	/* Put here the name of the sun on which the server is executed */
	host = gethostbyname ("sheerness.cs.ualberta.ca");

	if (host == NULL) {
		perror ("Client: cannot get host description");
		exit (1);
	}

	while (1) {

		s = socket (AF_INET, SOCK_STREAM, 0);

		if (s < 0) {
			perror ("Client: cannot open socket");
			exit (1);
		}

		bzero (&server, sizeof (server));
		bcopy (host->h_addr, & (server.sin_addr), host->h_length);
		server.sin_family = host->h_addrtype;
		server.sin_port = htons (MY_PORT);

		if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
			perror ("Client: cannot connect to server");
			exit (1);
		}

		read (s, &number, sizeof (number));
		close (s);
		fprintf (stderr, "Process %d gets number %d\n", getpid (),
			ntohl (number));
		sleep (5);
	}
}
