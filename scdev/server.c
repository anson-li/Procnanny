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

/*
xA server process nanny (which can also be a modified form of the procnanny 
from Assignment #2). There is only one server process nanny. The server 
centralizes the configuration information, it centralizes the logging of 
activities related to process monitoring, and it centralizes the clean exit 
of all related processes. The name of your executable for the server must be 
procnanny.server.  Note that procnanny.server does NOT actually do any of 
the monitoring or killing of processes itself.
*/

#define SERVNAME  "ug15"
// have to change servname dynamically
#define MAXMSG 512
#define PORT   2222

static int finalpval = 0;
char hostname[255];
static int parentPID; 


char appdata[280][1000];
char test[280][1000]; //array of strings //length is 10! figure out how to realloc!

int timedata[280];
int counter;

void genericOP(char* data);
void consoleOP(char* data);
void setupProcnanny(char * filepath);

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

  setupProcnanny(argv[1]);

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

  char infolog[100];
  sprintf(infolog, "Procnanny server: PID %d on node %s, port %d", curpid, hostname, finalpval);
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
          char buffer[MAXMSG];
          size = sizeof (clientname);
          new = accept (sock, (struct sockaddr *) &clientname, &size);
          /* Connection accepted at 'new'*/ 
          if (new < 0) {
            perror ("accept");
            exit (EXIT_FAILURE);
          }

          fprintf (stderr,
                   "Server: connect from host %d, port %hd.\n",
                   inet_ntoa (clientname.sin_addr),
                   ntohs (clientname.sin_port));
          FD_SET (new, &active_fd_set);
          
          //memset(&buffer[0], 0, sizeof(buffer));
          printf("init send buffer\n");
          strcpy(buffer, "send data test");
          write(new, &buffer, sizeof(buffer));
          printf("complete send\n");
          /*
          char buffer[MAXMSG];
          for (i = 0; i < counter; i++) {
            if (appdata[i][0] != '\0') {
              memset(&buffer[0], 0, sizeof(buffer));
              sprintf(buffer, "#%s %d", appdata[i], timedata[i]);
              printf("BUFFER: %s\n", buffer);
              write(i, buffer, sizeof(buffer) + 1);
            }
          }*/
          // write config details
          //write(filedes, buffer, sizeof(buffer) + 1); 
        } else {
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

void setupProcnanny(char * filepath) {
  
  deleteProcnannies();
  getParentPID();
  initialisationOP();
  
  const char* s = getenv("PROCNANNYLOGS"); 
  FILE* logfile = fopen(s, "w");
  fclose(logfile);   

  FILE* file = fopen ( filepath, "r" );

  if (file == NULL) {
      printf("ERROR: Nanny.config file presented was not found. Program exiting...");
      exit(EXIT_FAILURE);  
  }

  counter = 0;
  char * pch; 
  int countval = 0;

  if (file != NULL) {
      char line[100];
      while (fgets(line, sizeof line, file) != NULL) { /* read a line from a file */
          // reads sample text: testa 120
          strcpy(test[counter - 1], strtok(line, "\n"));
          pch = strtok (test[counter-1]," ,.-");
          while (pch != NULL) {
              if (countval == 0) {
                  strcpy(appdata[counter], pch);
                  countval++;
              } else if (countval == 1) {
                  timedata[counter] = atoi(pch);
                  countval = 0;
                  break;
              }
              pch = strtok (NULL, " ,.-");
          }
          counter++;
      }
  }
  fclose(file);
}

void deleteProcnannies() {
        FILE * pnfile;
        pid_t curpid;
        curpid = getpid();
        if ( ( pnfile = popen("pgrep procnanny", "r" ) ) == NULL ) {
                perror( "popen" );
        } else { 
            pid_t pidpn;
            char pidbuffer[30];
            while (fgets(pidbuffer, 150, pnfile) != NULL) {
                pidpn = (pid_t) strtol(pidbuffer, NULL, 10);
                // if pid is not the one you currently opened...
                if (pidpn != curpid) {
                    kill(pidpn, SIGKILL);
                }
            }
        }
        fclose(pnfile); 
        return;
}

void getParentPID() {
    pid_t parent_pid = getpid();
    parentPID = getpid();
    printf("Host PID: %d\n", parent_pid);
    return;
}

void initialisationOP() {
    char strPID[1000];
    sprintf(strPID, "Info: Parent process is PID  %d.", parentPID);
    consoleOP(strPID);
    return;
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