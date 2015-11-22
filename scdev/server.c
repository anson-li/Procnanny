#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "memwatch.h"

#define SERVNAME  "ug15"
// have to change servname dynamically
#define MAXMSG 512
#define PORT   2222

static int finalpval = 0;
char hostname[255];

void genericOP(char* data);
void consoleOP(char* data);

int make_socket (uint16_t port)
{
  int sock;
  struct sockaddr_in name;

  /* Create the socket. */
  /*
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }
    */
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror ("Server: cannot open master socket");
    exit (1);
  }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      perror ("bind");
      //exit (EXIT_FAILURE);
      port++;
      name.sin_port = htons (port);
    }
  finalpval = port;
  return sock;
}

int read_from_client (int filedes)
{
  char buffer[MAXMSG];
  char resultString[100];
  char * token;
  int nbytes;

  nbytes = read (filedes, buffer, MAXMSG);
  if (nbytes < 0)
    {
      /* Read error. */
      perror ("read");
      exit (EXIT_FAILURE);
    }
  else if (nbytes == 0)
    /* End-of-file. */
    return -1;
  else
    {
      /* Data read. */
      memset(&resultString[0], 0, sizeof(resultString));
      fprintf (stderr, "Server: got message: `%s'\n", buffer);
      token = strtok(buffer, "\n"); // grabs the first token... we don't care about the other ones I think.
      printf("Parsed the following message: %s\n", token);
      if (token != NULL) {
        if (strcmp(token, "1") == 0) { // if entered input is 1
          printf("1 called - initProcNanny calls!\n");
          strcpy(resultString, "#init Procnanny\n");
          //write(filedes, resultString, (strlen(resultString)+1));
        }
        else if (strcmp(token, "2") == 0) { // if entered input is 1
          printf("2 called - sigint ProcNanny calls!\n");
          strcpy(resultString, "#sigint Procnanny\n");
          //write(filedes, resultString, (strlen(resultString)+1));
        }
        else if (strcmp(token, "3") == 0) { // if entered input is 1
          printf("3 called - sighup ProcNanny calls!\n");
          strcpy(resultString, "#sighup Procnanny\n");
          //write(filedes, resultString, (strlen(resultString)+1));
        } else {
          strcpy(resultString, "#nothing of use\n");
        }
      }
      memset(&buffer[0], 0, sizeof(buffer));
      strcpy(buffer, resultString);
      write(filedes, buffer, sizeof(buffer) + 1); 
      return 0;
    }
}



int main (int c, char *argv[]) {
  extern int make_socket (uint16_t port);
  int sock;
  fd_set active_fd_set, read_fd_set, write_fd_set;
  int i;
  struct sockaddr_in clientname;
  size_t size;

  /* Create the socket and set it up to accept connections. */
  sock = make_socket (PORT);
  if (listen (sock, 1) < 0)
    {
      perror ("listen");
      exit (EXIT_FAILURE);
    }

  /* Initialize the set of active sockets. */
  FD_ZERO (&active_fd_set);
  FD_SET (sock, &active_fd_set);

  hostname[254] = '\0';
  gethostname(hostname, 255);
  int curpid;
  curpid = getpid();

  char * infolog;
  sprintf(infolog, "Procnanny server: PID %d on node %s, port %d\n", curpid, hostname, finalpval);
  consoleOP(infolog);

  while (1) {
    /* Block until input arrives on one or more active sockets. */
    read_fd_set = active_fd_set;
    write_fd_set = active_fd_set;
    if (select (FD_SETSIZE + 1, &read_fd_set, /*&write_fd_set*/ NULL, NULL, NULL) < 0) {
      perror ("select");
      exit (EXIT_FAILURE);
    }
    printf("Connection made!\n");
    /* Service all the sockets with input pending. */
    for (i = 0; i < FD_SETSIZE; ++i) {
      //printf("FD is set\n");
      if (FD_ISSET (i, &read_fd_set)) {
        //printf("finding sock\n");
        if (i == sock) {
          printf("Connection made on new socket");
          /* Connection request on original socket. */
          int new;
          size = sizeof (clientname);
          new = accept (sock, (struct sockaddr *) &clientname, &size);
          if (new < 0) {
            perror ("accept");
            exit (EXIT_FAILURE);
          }
          fprintf (stderr,
                   "Server: connect from host %d, port %hd.\n",
                   inet_ntoa (clientname.sin_addr),
                   ntohs (clientname.sin_port));
          FD_SET (new, &active_fd_set);
          }
        else {
          /* Data arriving on an already-connected socket. */
          if (read_from_client (i) < 0) {
            close (i);
            FD_CLR (i, &active_fd_set);
          }
        }
      }
    }
  } 
}

void genericOP(char* data) {
    const char* s = getenv("PROCNANNYLOGS");
    FILE* file= fopen (s, "a" );
    time_t ltime;
    time(&ltime); 
    fprintf(file, "[%s] %s\n", strtok(ctime(&ltime), "\n"), data);
    fclose(file);
    return;
}

void consoleOP(char * data) {
  time_t ltime;
  time(&ltime); 
  printf("[%s] %s\n", strtok(ctime(&ltime), "\n"), data);
  return;
}