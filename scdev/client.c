#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

/* ----------------------------------------------------- */
/* This is a sample client for the "remote login" server */
/* ----------------------------------------------------- */

#define	SERVNAME	"sheerness.cs.ualberta.ca"
#define MY_PORT		2222

main() 
{
	struct	sockaddr_in	server;
	struct	hostent		*host;
	int s;
	char c;

	printf("Beginning child process...");
	host = gethostbyname (SERVNAME);

	if (host == NULL) {
		perror ("Client: cannot get host description");
		exit(EXIT_FAILURE);
	}

	s = socket (AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		perror ("Client: cannot open socket");
		exit(EXIT_FAILURE);
	}

	bzero (&server, sizeof (server));
	bcopy (host->h_addr, & (server.sin_addr), host->h_length);
	server.sin_family = host->h_addrtype;
	server.sin_port = htons (MY_PORT);

	if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Producer: cannot connect to server");
		exit(EXIT_FAILURE);
	}
	if (fork ()) {
		while (1) {
			c = getchar ();
			write (s, &c, 1);
		}
	} else {
		while (1) {
			read (s, &c, 1);
			putchar (c);
		}
	}
}