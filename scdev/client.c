#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

/*
A client process nanny (which is a modified form of the procnanny from Assignment #2). 
Each workstation will have at most one client process nanny. The client must use fork() 
to create child processes that use kill() to kill monitored processes. The name of your 
executable for the client must be procnanny.client.
*/

char SERVNAME[100];
static int MY_PORT;

#define MAXMSG 512

int getConfig();

int main(int cv, char *argv[]) {
	struct	sockaddr_in	server;
	struct	hostent		*host;
	int s, filedes;
	fd_set read_fd_set;
	char c;

	setServerDetails(argv[1], argv[2]);

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

	printf("MY_PORT: %d, SERVNAME: %s\n", MY_PORT, SERVNAME);
	if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Producer: cannot connect to server");
		exit (1);
	}

	// ret = select(maxdesc, &read_from, NULL, NULL, &tv);
	getConfig(s); 

	// need to read all of the information before you do anything...

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

void setServerDetails(char* servname, char* port) {
	// read from PROCNANNYCONFIG set data, set the servname and my_port data
	if (servname == NULL || port == NULL) {
		printf("Invalid parameters entered; client exiting.\n");
		exit(EXIT_FAILURE);
	}
	strcpy(SERVNAME,servname);
	MY_PORT = atoi(port);
}

int getConfig(int filedes) {
	char buffer[MAXMSG];
	fd_set read_fd_set;
	char resultString[100];
	char * token;
	int nbytes;
	int totalFill = 10000;
	struct timeval timedif;
	timedif.tv_sec = 0;
    timedif.tv_usec = 0;


	printf("Waiting for config file now.\n");
	while (1) {

		FD_ZERO (&read_fd_set);
		FD_SET (filedes, &read_fd_set);

		if (select(totalFill, &read_fd_set, NULL, NULL, &timedif)) { 

			printf("Entered...\n");
			nbytes = read (filedes, buffer, MAXMSG);
			if (nbytes < 0) {
				/* Read error. */
			    perror ("read");
			    exit (EXIT_FAILURE);
			} else if (nbytes == 0) { 
				/* End-of-file. */
			    return -1;
			} else {
				/* Data read. */
			    memset(&resultString[0], 0, sizeof(resultString));
		        fprintf (stderr, "Server: got message: '%s'\n", buffer);
		        //token = strtok(buffer, "\n"); // grabs the first token... we don't care about the other ones I think.
		        //printf("Parsed the following message: %s\n", token);
		        if (strcmp(buffer, "EOF")) {
		        	printf("eof...\n");
		      		return 0;
		      	} else {
		      		// parse the config data here / first application val is the app, second is the time
		      		printf("valid statement!\n");
		      		char * token;
		      		token = strtok(buffer, " ");
   					/* walk through other tokens */
   					while( token != NULL ) {
      					printf( "TOKEN: %s\n", token );
      					token = strtok(NULL, " ");
   					}
		      	}
			}
		}
	}
}