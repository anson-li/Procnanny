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

int clientCount = 0;
int clientsList[32];

static int finalpval = 0;
char hostname[255];
char filepathmain[255];
char hostnamelist[32][255];
int hostnamesize = 0;

static int parentPID; 
int killcount = 0;
int sigintcount = 0;

static int SHFLAG = 0;
static int SIFLAG = 0;

char appdata[280][1000];
char test[280][1000]; //array of strings //length is 10! figure out how to realloc!

int timedata[280];
int counter = 0;

void genericOP(char* data);
void genOPnotime(char* data);
void consoleOP(char* data);
void setupProcnanny(char * filepath);
void endProcess();
void writeEndProcesses();
void killClients();
void sighupProcess();
void pidKilledOP(char * pidval, char * appdata, char * hostname, char * timeStr);
void readProcnanny(char * filepath);
void sendNewData();
void getParentPID();
void initialisationOP();
void deleteProcnannies();


static void catch_function(int signo) {
    SIFLAG = 1;
    endProcess();
}

static void ignore_function(int signo ) { 
    SHFLAG = 1;
    sighupProcess(); // SIGHUP // HAVE TO REWRITE 
}

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
  while (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
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
  char * subtoken;
  int nbytes;

  nbytes = read (filedes, buffer, MAXMSG);
  if (nbytes < 0) {
      /* Read error. */
      perror ("read");
      /**
      * FIXME: if 'connection reset by peer', it means that the station is terminated -- but DON'T EXIT! 
      */
      //exit (EXIT_FAILURE);
  }
  else if (nbytes == 0) {
    /* End-of-file. */
    return -1;
  }
  else {
    /* Data read. */
    memset(&resultString[0], 0, sizeof(resultString));
    fprintf (stderr, "Server: got message: '%s'\n", buffer);
    token = strtok(buffer, "\n"); // grabs the first token... we don't care about the other ones I think.
    printf("Parsed the following message: %s\n", token);
    if (token != NULL) {
      if (strncmp(token, "[", 1) == 0 ) { // just save these, not necessary
        genOPnotime(token);
      } 
      if (strncmp(token, "2", 1) == 0 ) { // 2 is the sigint command

        int procKilled = 0;
        int subcounter = 0;
        subtoken = strtok(token, " ");
        while( subtoken != NULL ) {
          if (subcounter == 1) {
            printf("Total process killed here: %s\n", subtoken);
            procKilled = procKilled + atoi(subtoken);
          }
          subtoken = strtok(NULL, " ");
          subcounter++;
        }
        sigintcount = sigintcount + procKilled;
        return 0;

      } 
      if (strncmp(token, "5", 1) == 0 ) { // 5 is the kill command 
        killcount++;
        int subcounter = 0;

        char pidval[250];
        char appdata[250];
        char timeStr[250];
        char hostname[250];

        subtoken = strtok(token, " ");
        while( subtoken != NULL ) {
          if (subcounter == 1) {
            strcpy(pidval, subtoken);
          } else if (subcounter == 2) {
            strcpy(appdata, subtoken);
          } else if (subcounter == 3) {
            strcpy(hostname, subtoken);
          } else if (subcounter == 4) {
            strcpy(timeStr, subtoken);
          }
          subtoken = strtok(NULL, " ");
          subcounter++;
        }
        int iterh;
        int iterflag = 0;
        for (iterh = 0; iterh < hostnamesize; iterh++) {
          if (strcmp(hostname, hostnamelist[iterh]) == 0) {
            iterflag = 1;
          } 
        }
        if (iterflag == 0) {
          strcpy(hostnamelist[hostnamesize], hostname);
        }
        pidKilledOP(pidval, appdata, hostname, timeStr);
      }
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
  return 0;
}

int main (int c, char *argv[]) {
  extern int make_socket (uint16_t port);
  int sock;
  fd_set active_fd_set, read_fd_set/*, write_fd_set*/;
  int i;
  struct sockaddr_in clientname;
  size_t size;

  signal(SIGHUP, ignore_function);
  signal(SIGINT, catch_function);
  setupProcnanny(argv[1]);
  strcpy(filepathmain, argv[1]);

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
    //write_fd_set = active_fd_set;
    if (select (FD_SETSIZE + 1, &read_fd_set, /*&write_fd_set*/ NULL, NULL, NULL) < 0) {
      perror ("select");
      //exit (EXIT_FAILURE);
      continue;
    }
    printf("Connection made!\n");
    /* Service all the sockets with input pending. */
    for (i = 0; i < FD_SETSIZE; ++i) {
      //printf("FD is set\n");
      if (FD_ISSET (i, &read_fd_set)) {
        //printf("finding sock\n");
        if (i == sock) {
          printf("Connection made on new socket\n");
          /* Connection request on original socket. */
          char buffer[MAXMSG];
          size = sizeof (clientname);
          clientsList[clientCount] = accept (sock, (struct sockaddr *) &clientname, &size);
          /* Connection accepted at 'clientsList[clientCount]'*/ 
          if (clientsList[clientCount] < 0) {
            perror ("accept");
            exit (EXIT_FAILURE);
          }

          fprintf (stderr,
                   "Server: connect from host %d, port %hs.\n",
                   inet_ntoa (clientname.sin_addr),
                   ntohs (clientname.sin_port));
          FD_SET (clientsList[clientCount], &active_fd_set);
          
          //memset(&buffer[0], 0, sizeof(buffer));
          /*
          printf("init send buffer\n");
          strcpy(buffer, "send data test");
          write(new, &buffer, sizeof(buffer));
          printf("complete send\n");
          */
          printf("COUNTER: %d\n", counter);
          for (i = 0; i < counter; i++) {
            if (appdata[i][0] != '\0') {
              memset(&buffer[0], 0, sizeof(buffer));
              sprintf(buffer, "%s %d", appdata[i], timedata[i]);
              printf("BUFFER: %s\n", buffer);
              write(clientsList[clientCount], &buffer, sizeof(buffer));
            }
          }
          memset(&buffer[0], 0, sizeof(buffer));
          strcpy(buffer, "EOF");
          write(clientsList[clientCount], &buffer, sizeof(buffer));
          clientCount++;
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

void sighupProcess() {
  //reread process
  printf("reached sighup...\n");
  readProcnanny(filepathmain);
  sendNewData();
  SHFLAG = 0;
}

void readProcnanny(char * filepath) {
  const char* s = getenv("PROCNANNYLOGS"); 
  FILE* logfile = fopen(s, "w");
  fclose(logfile);   

  FILE* file = fopen ( filepath, "r" );

  if (file == NULL) {
      printf("ERROR: Nanny.config file presented was not found. Program exiting...");
      exit(EXIT_FAILURE);  
  }

  char * pch; 
  int countval = 0;
  counter = 0;

  memset(&test[0], 0, sizeof(test));
  memset(&appdata[0], 0, sizeof(appdata));
  memset(&timedata[0], 0, sizeof(timedata));

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

void sendNewData() {
  char buffer[MAXMSG];
  int i, j;
  fd_set read_fd_set;
  for (i = 0; i < clientCount; i++) {
    FD_ZERO (&read_fd_set);
    FD_SET (clientsList[i], &read_fd_set);
    memset(&buffer[0], 0, sizeof(buffer));
    sprintf(buffer, "3");
    write(clientsList[i], &buffer, sizeof(buffer));
    for (j = 0; j < counter; j++) {
      if (appdata[j][0] != '\0') {
        memset(&buffer[0], 0, sizeof(buffer));
        sprintf(buffer, "%s %d", appdata[j], timedata[j]);
        printf("BUFFER: %s\n", buffer);
        write(clientsList[i], &buffer, sizeof(buffer));
      }
    }
    memset(&buffer[0], 0, sizeof(buffer));
    sprintf(buffer, "EOF");
    printf("BUFFER: %s\n", buffer);
    write(clientsList[i], &buffer, sizeof(buffer));
  }
  sleep(1);
}

void endProcess() {
  killClients();
  writeEndProcesses();
  //sleep(1);
  exit(EXIT_SUCCESS);
}

void killClients() {
  int i;
  char endMsg[MAXMSG];
  char sigintChar[MAXMSG];
  fd_set /*write_fd_set,*/ read_fd_set;
  //struct timeval timedif;
  //timedif.tv_sec = 2;
  //timedif.tv_usec = 0;

  consoleOP("Killing procnanny clients.");
  for (i = 0; i < clientCount; i++) {
    // send signal
    // read response
    char buffer[MAXMSG];
    memset(&buffer[0], 0, sizeof(buffer));
    sprintf(buffer, "1"); // 1 denotes sigint

    //FD_ZERO (&write_fd_set);
    //FD_SET (clientsList[clientCount], &write_fd_set);
    FD_ZERO (&read_fd_set);
    FD_SET (clientsList[i], &read_fd_set);
    // ACCEPT IT HERE TOO
    // block until it can send 
    //printf("Writing now!\n");
    write(clientsList[i], &buffer, sizeof(buffer));
    //printf("Reading now!\n");
    read_from_client(clientsList[i]);
    /*
    retval = select(FD_SETSIZE + 1,&read_fd_set, NULL, NULL, &timedif);
    if (retval >= 0) {
      printf("retval initialised ...\n");
      write(clientsList[i], &buffer, sizeof(buffer));
    } else {

    }
    readval = select(FD_SETSIZE + 1,&read_fd_set, NULL, NULL, &timedif);
    if (readval) {
      read_from_client(clientsList[i]);
    }*/
  }
  //sprintf(printNum, "Info: Caught SIGINT. Exiting cleanly. %d process(es) killed.", killPID);
  strcpy(endMsg, "Info: Caught SIGINT. Exiting cleanly. ");
  sprintf(sigintChar, "%d", sigintcount);
  strcat(endMsg, sigintChar);
  strcat(endMsg, " process(es) killed on ");
  int j;
  for (j = 0; j <= hostnamesize; j++) {
    if (j == 0) {
      strcat(endMsg, hostnamelist[j]);
    } else {
      strcat(endMsg, ", ");
      strcat(endMsg, hostnamelist[j]);
    }
  }
  strcat(endMsg, ".");
  consoleOP(endMsg); // have to specify which nodes removed.
  genericOP(endMsg);
}

void writeEndProcesses() {
  char endproc[255];
  char kcchar[15];
  //sprintf(endproc, "Info: Server exiting. %d processes killed on nodes", killcount);
  strcpy(endproc, "Info: Server exiting. ");
  sprintf(kcchar, "%d", killcount);
  strcat(endproc, kcchar);
  strcat(endproc, " processe(s) killed on node(s) ");
  int i;
  for (i = 0; i <= hostnamesize; i++) {
    if (i == 0) {
      strcat(endproc, hostnamelist[i]);
    } else {
      strcat(endproc, ", ");
      strcat(endproc, hostnamelist[i]);
    }
  }
  strcat(endproc, ".");
  consoleOP(endproc); // have to specify which nodes removed.
  genericOP(endproc);
}

void setupProcnanny(char * filepath) {
  deleteProcnannies();
  getParentPID();
  initialisationOP();
  readProcnanny(filepath);
  return;
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

void pidKilledOP(char pidval[250], char appdata[250], char hostname[250], char timeStr[250]) {
    char str[1000];
    strcpy(str, "Action: PID ");
    strcat(str, pidval);
    strcat(str, " (");
    strcat(str, appdata);
    strcat(str, ") on ");
    strcat(str, hostname);
    strcat(str, " killed after exceeding ");
    strcat(str, timeStr);
    strcat(str, " seconds.");
    genericOP(str);
    return;
}

void genOPnotime(char * data) {
    const char* s = getenv("PROCNANNYLOGS");
    FILE* file= fopen (s, "a" );
    fprintf(file, "%s\n", data);
    fclose(file);
    return;
}

void consoleOP(char * data) {
  time_t ltime;
  time(&ltime); 
  printf("[%s] %s\n", strtok(ctime(&ltime), "\n"), data);
  return;
}