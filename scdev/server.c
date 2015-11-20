#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>  /* _exit, fork */


/* --------------------------------------------------------------- */
/* This  is  a simple "login server" that accepts connections from */
/* remote clients and, for each client, opens a shell on the local */
/* machine.                                                        */
/* --------------------------------------------------------------- */

/*
#define	SERVNAME	"sheerness.cs.ualberta.ca"
#define PORT    2222
#define MAXMSG  512

int make_socket (uint16_t port)
{
  int sock;
  struct sockaddr_in name;
  
  // create socket
  printf("Init process...");
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  // name socket
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      perror ("bind");
      exit(EXIT_FAILURE);
    }

  return sock;
}

int read_from_client (int filedes)
{
  char buffer[MAXMSG];
  int nbytes;

  nbytes = read (filedes, buffer, MAXMSG);
  if (nbytes < 0)
    {
      perror ("read");
      exit (EXIT_FAILURE);
    }
  else if (nbytes == 0)
    // End of file
    return -1;
  else
    {
      // data read
      fprintf (stderr, "Server: got message: `%s'\n", buffer);
      return 0;
    }
}*/

int main (int c)
{
  printf("eh");
  /*
  extern int make_socket (uint16_t port);
  int sock;
  fd_set active_fd_set, read_fd_set; //two active sets that will be monitored
  int i;
  struct sockaddr_in clientname;
  size_t size;

  printf("debuggery");
  // Create the socket and set it up to accept connections.
  sock = make_socket (PORT);
  if (listen (sock, 1) < 0)
    {
      perror ("listen");
      exit (EXIT_FAILURE);
    }

  // Initialize the set of active sockets.
  FD_ZERO (&active_fd_set); 
  FD_SET (sock, &active_fd_set);
  printf("You made it on the server! beginning process...");
  while (1)
    {
      // Block until input arrives on one or more active sockets.
      read_fd_set = active_fd_set;
      if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) 
        {
          perror ("select");
          exit (EXIT_FAILURE);
        }

      // Service all the sockets with input pending.
      for (i = 0; i < FD_SETSIZE; ++i)	// // calls select on the socket / so blocked until the socket gets the flag.
        if (FD_ISSET (i, &read_fd_set))
          {
            if (i == sock)
              {
                /* Connection request on original socket.
                int new; // use this one to accept to the socket to talk to the existing client
                size = sizeof (clientname);
                new = accept (sock,
                              (struct sockaddr *) &clientname,
                              &size);
                if (new < 0)
                  {
                    perror ("accept");
                    exit (EXIT_FAILURE);
                  }
                fprintf (stderr,
                         "Server: connect from host %s, port %hd.\n", // get the actual connection
                         inet_ntoa (clientname.sin_addr),
                         ntohs (clientname.sin_port));
                FD_SET (new, &active_fd_set);
              }
            else
              {
                /* Data arriving on an already-connected socket.
                if (read_from_client (i) < 0)
                  {
                    close (i);
                    FD_CLR (i, &active_fd_set);
                  }
              }
          }
    } */
}