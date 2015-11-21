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

char *	SERVNAME	"ug15"
int 	MY_PORT		2222

int main(int cv, char *argv[]) {
	struct	sockaddr_in	server;
	struct	hostent		*host;
	int s;
	char c;

	setServerDetails();

	host = gethostbyname (SERVNAME);

	if (host == NULL) {
		perror ("Client: cannot get host description");
		exit (1);
	}

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
		perror ("Producer: cannot connect to server");
		exit (1);
	}
	if (fork ()) {
		while (1) { // child process write
			c = getchar ();
			write (s, &c, 1);
		}
	} else {
		while (1) { // parent process reads
			read (s, &c, 1);
			putchar (c);
		}
	}
}

void setServerDetails() {
	// read from PROCNANNYCONFIG set data, set the servname and my_port data
}
