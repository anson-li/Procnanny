#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>   /* errno */
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/*
A client process nanny (which is a modified form of the procnanny from Assignment #2). 
Each workstation will have at most one client process nanny. The client must use fork() 
to create child processes that use kill() to kill monitored processes. The name of your 
executable for the client must be procnanny.client.
*/

char hostname[255];
char SERVNAME[100];
static int MY_PORT;

char appdata[280][1000];
char test[280][1000]; //array of strings //length is 10! figure out how to realloc!

char appProcessed[1000];
int timeProcessed;

int timedata[280];
int counter;
int signum = 0;

int timeread[280];
char appread[280][1000];
char testread[280][1000];
int countread = 0;
char pidval[150];

char newpid[150];

static int parentPID; 
static int SHFLAG = 0;
static int SIFLAG = 0;
int isnewnode = 0;

#define MAXMSG 512
#define READ 0
#define WRITE 1 
#define CHILD 0
#define PARENT 1

/* socket specific variables */
int s;


int getConfig();
void setServerDetails(char* servname, char* port);
int monitorProcesses(int filedes);
void genericOP(char* data);
void consoleOP(char* data);
void killProcessOP(int signum);
void pidKilledOP(char * pidval, char * appdata, char * timeStr);
void noProcessOP(char * appdata);
void initProcOP(char * appdata, char * pidval );
void deleteProcnannies();
void sigintProcnannies();
void getParentPID();
void initialisationOP();
void write_to_pipe(int file, char * data);
void read_from_pipe(int file);
void read_from_server();
void genericOPnotime(char * data);


/*
static void catch_function(int signo) {
    SIFLAG = 1;
}*/

static void fail_function(int signo) {
    exit(EXIT_FAILURE);
}

/*
static void ignore_function(int signo ) { 
    SHFLAG = 1; 
}*/

int main(int cv, char *argv[]) {
	struct	sockaddr_in	server;
	struct	hostent		*host;
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

	hostname[254] = '\0';
 gethostname(hostname, 255);

 bzero (&server, sizeof (server));
 bcopy (host->h_addr, & (server.sin_addr), host->h_length);
 server.sin_family = host->h_addrtype;
 server.sin_port = htons (MY_PORT);

 //printf("MY_PORT: %d, SERVNAME: %s\n", MY_PORT, SERVNAME);
 if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
  perror ("Producer: cannot connect to server");
  exit (1);
}

	// ret = select(maxdesc, &read_from, NULL, NULL, &tv);
getConfig(s); 
monitorProcesses(s);

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
    counter = 0;

    //printf("Waiting for config file now.\n");
    while (1) {
		//char * token;
      FD_ZERO (&read_fd_set);
      FD_SET (filedes, &read_fd_set);

      if (select(totalFill, &read_fd_set, NULL, NULL, &timedif)) { 

         //printf("Entered...\n");
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
            //fprintf (stderr, "Server: got message: '%s'\n", buffer);
		        //token = strtok(buffer, "\n"); // grabs the first token... we don't care about the other ones I think.
		        //printf("Parsed the following message: %s\n", token);
            if (strcmp(buffer, "EOF") == 0) {
                 int i;
                 for (i = 0; i < counter; i++) {
                    //printf("Application: %s, for %d seconds\n", appdata[i], timedata[i]);
                 }
                 return 0;
            } else {
		      		// parse the config data here / first application val is the app, second is the time
              int countval = 0;
              token = strtok(buffer, " ");
   					/* walk through other tokens */
                while( token != NULL ) {
                    if (countval == 0) {
                        //printf( "APP: %s\n", token );
                        strcpy(appdata[counter], token);
                        countval++;
                    } else {
                        //printf("TIME: %s\n", token);
                        countval = 0;
                        timedata[counter] = atoi(token);
                    }
                    token = strtok(NULL, " ");
                }
                counter++;
            }
        }
     }
    }
}

int monitorProcesses(int filedes) {
	
	FILE* f[1000]; // may have to change this, original value counter - 1 ; need to realloc then.
    int i;
    int totalProcessCounter = 0;
    pid_t pid; 
    int fd[counter][2][2];

    for (i = 0; i < counter; i++) {
        if (appdata[i][0] != '\0') { 
            //printf("Application Testing: %s\n", test[i]);
            char grepip[1000];
            strcpy(grepip, "pgrep ");
            strcat(grepip, appdata[i]);
            if ( ( f[i] = popen( grepip, "r" ) ) == NULL ) {
                //perror( "popen" );
            } else {
                pid_t pidint;
                int haspid = 0;

                if (pipe(fd[i][PARENT]) < 0) {
                    exit(EXIT_FAILURE); 
                }
                if (pipe(fd[i][CHILD]) < 0) {
                    exit(EXIT_FAILURE); 
                }

                while (fgets(pidval, 150, f[i]) != NULL) {
                    haspid = 1;
                    pid = fork();
                    if (pid < 0) { // error process

                        //fprintf(stderr, "can't fork, error %d\n", errno);
                        exit(EXIT_FAILURE);

                    } else if (pid > 0) { // parent process

                        totalProcessCounter = totalProcessCounter + 1;

                    } else if (pid == 0 ) { // child process

                        signal(SIGINT, fail_function);

                        strcpy(appProcessed, appdata[i]);
                        timeProcessed = timedata[i];

                        childMonitoring:;
                        //printf("PIDVAL22222: %s\n", pidval);
                        //printf("NEWPID2222: %s\n", newpid);

                        if (isnewnode == 1) {
                            strcpy(pidval, newpid);
                            isnewnode = 0;
                        }

                        int count = 0;
                        char buff[1000];
                        bzero(buff, 1000);
                        char byte = 0;

                        strtok(pidval, "\n");
                        initProcOP(appProcessed, pidval);

                        pidint = (pid_t) strtol(pidval, NULL, 10);
                        fcntl(fd[i][PARENT][READ], F_SETFL, O_NONBLOCK);

                        int sleepLeft = timeProcessed;
                        while(sleepLeft > 0) {
                            sleep(5);
                            // Need to start the reading at every 5s break; if its (-1) then end the program, if its new pid then clear pid and 
                            // write to inform the child is busy.
                            while (SIFLAG == 1 || read(fd[i][PARENT][READ], &byte, 1) == 1) {
                                if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
                                    sigintProcnannies();
                                    goto completeProcess;
                                }
                                if (ioctl(fd[i][PARENT][READ], FIONREAD, &count) != -1) {
                                    buff[0] = byte;
                                    char printWrite[150];
                                    if (read(fd[i][PARENT][READ], buff+1, count) == count) {
                                        int charbuf = atoi(buff);
                                        if (charbuf == 1) {
                                        sprintf(printWrite, "2"); // sends for onwait process
                                        write_to_pipe(fd[i][CHILD][WRITE], printWrite);
                                    } else if (charbuf == -1) {
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                        }
                        // inform busy ...
                        sleepLeft = sleepLeft - 5;
                    }

                    pidint = (pid_t) strtol(pidval, NULL, 10);
                    char timeStr[30];
                    sprintf(timeStr, "%d", timeProcessed);
                    char prntChild[150];
                    //printf("killpid: %d\n", pidint);
                    //printf("KILLING THE PROCESS\n");
                    int killresult = kill(pidint, SIGKILL);
                    if (killresult == 0) {
                            //printf("You killed the process (PID: %d) (Application: %s)\n", pidint, test[i] );
                        pidKilledOP(pidval, appProcessed, timeStr);
                        printf("111111\n");
                        sprintf(prntChild, "1 %s", appProcessed);
                        printf("222222\n");
                    } else if (killresult == -1) {
                            //printf("ERROR: Process already killed (PID: %d) (Application: %s)\n", pidint, test[i] );
                      sprintf(prntChild, "0 %s", appProcessed);
                    }
                        //pclose(f[i]);
                        write_to_pipe(fd[i][CHILD][WRITE], prntChild); // writing to parent that is polling
                        int ops;
                        ops = fcntl(fd[i][PARENT][READ],F_GETFL); // reenable blocking
                        fcntl(fd[i][PARENT][READ], F_SETFL, ops & ~O_NONBLOCK); 
                        while (SIFLAG == 1 || read(fd[i][PARENT][READ], &byte, 1) == 1) {
                            if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
                                sigintProcnannies();
                                goto completeProcess;
                            }
                            if (ioctl(fd[i][PARENT][READ], FIONREAD, &count) != -1) {
                                buff[0] = byte;
                                if (read(fd[i][PARENT][READ], buff+1, count) == count) {
                                    int bufval = atoi(buff);
                                    if (bufval == -1) {
                                        goto waitingProc;
                                    } else if (bufval == 1) {
                                        // new process started!
                                        char prntReady[150];
                                        sprintf(prntReady, "1"); // sends for onwait process
                                        write_to_pipe(fd[i][CHILD][WRITE], prntReady);
                                        // NEEDS TO WAIT @ THIS POINT
                                        while (SIFLAG == 1 || read(fd[i][PARENT][READ], &byte, 1) == 1) {
                                            if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
                                                sigintProcnannies();
                                                goto completeProcess;
                                            }
                                            if (ioctl(fd[i][PARENT][READ], FIONREAD, &count) != -1) {
                                                buff[0] = byte;
                                                if (read(fd[i][PARENT][READ], buff+1, count) == count) {
                                                    char * bch;
                                                    int countvalb = 0;
                                                    // read the buff value
                                                    // parse buff into 2 parts;
                                                    bch = strtok (buff," ,.*");
                                                    while (bch != NULL) {
                                                        if (countvalb == 0) {
                                                            strcpy(appProcessed, bch);
                                                            countvalb++;
                                                        } else if (countvalb == 1) {
                                                            timeProcessed = atoi(bch);
                                                            countvalb = 0;
                                                            strcpy(grepip, "pgrep ");
                                                            strcat(grepip, appProcessed);
                                                            if ( ( f[i] = popen( grepip, "r" ) ) == NULL ) {
                                                                //perror( "popen" );
                                                            } else {
                                                                if (fgets(pidval, 150, f[i]) != NULL) {
                                                                    goto childMonitoring;
                                                                } else {
                                                                    noProcessOP(appdata[i]);
                                                                    goto waitingProc;
                                                                }
                                                            }
                                                        }
                                                        bch = strtok (NULL, " ,.*");
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        goto waitingProc;
                    } 
                }
                if (haspid == 0) {
                    //printf("No processes found for %s.\n", test[i]);
                    noProcessOP(appdata[i]);
                }       
            }
            //pclose(f[i]);
        }
    }  

    int k; 
    char buff[1000];
    bzero(buff, 1000);
    char byte = 0;
    int count = 0;
    int h = 0;
    //int totalFill = 10000;
    fd_set read_fd_set;
    struct timeval timedif;

    int pidstatus;
    char * bch;
    int countvalb = 0;
    int countProcCompleted = 0;

    /*
    if (totalProcessCounter == 0) {
        goto completeProcess;
    }*/ 

        parentMonitoring:;
        for (k = 0; k < totalProcessCounter; k++ ) {
            fcntl(fd[k][CHILD][READ], F_SETFL, O_NONBLOCK);
        }

        k = 0;
        while(1) {
           //printf("new while iterate!\n");

           FD_ZERO (&read_fd_set);
           FD_SET (filedes, &read_fd_set);
		// see if the parent has anything to send 
		/**
		* FIXME: make this a conditional read once parents read...
		**/
		timedif.tv_sec = 1;
       timedif.tv_usec = 0;
       if (select(FD_SETSIZE + 1, &read_fd_set, NULL, NULL, &timedif)) { 
          read_from_server(filedes);
      } else {
          //printf("failed to properly read from server.\n");
      }
        if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
            sigintProcnannies();
            goto completeProcess;
        }
        if (k < totalProcessCounter) {
            k++;
        } else {
            k = 0;
        }
        fcntl(fd[k][CHILD][READ], F_SETFL, O_NONBLOCK);
        if (SHFLAG == 1 || read(fd[k][CHILD][READ], &byte, 1) == 1) {
            // if the ioctl is -1 then switch to the next k value...
            if (SHFLAG == 1) { // REREAD FILE
               int validChild = 0;
               char switchProc[1000];
               char buff2[1000];
               bzero(buff2, 1000);
               char byte2 = 0;
               //char test2[280][1000];
               int count2 = 0;

               //consoleOP("Info: Caught SIGHUP. Configuration file 'nanny.config' re-read.");
               //genericOP("Info: Caught SIGHUP. Configuration file 'nanny.config' re-read.");
                // if there's a new program then search, so search w/ respect to the current appname list
                //FILE* file2 = fopen ( argv[1], "r" );

                //if (file2 != NULL) {
                    //char line [1000];
                    //while (fgets(line, sizeof line, file2) != NULL) { // read a line from a file 
               int n;
               for (n = 0; n < countread; n++) {
                    // reads sample text: testa 120
                validChild = 0;
                int countval = 0;
                counter = totalProcessCounter + 1;
                    //strcpy(test2[counter - 1], strtok(line, "\n"));
                    //pch = strtok (test2[counter-1]," ,.*");
                int validAppData = 0;
                    //while (pch != NULL) {
                    //if (countval == 0) {
                int m;
                for (m = 0; m < totalProcessCounter; m++) {
                    validAppData = 0;
                    if (strcmp(appdata[m], appread[m]) == 0) {
                        validAppData = 1;
                    } 
                }
                if (validAppData == 0) {
                    strcpy(appdata[counter], appread[n]);
                    countval++;
                        //pch = strtok (NULL, " ,.*");
                }
                    //} 
                if (countval == 1 && validAppData == 0) {
                    timedata[counter] = timeread[n];
                    //printf("appdata: %s, timedata: %d", appdata[counter], timedata[counter]);
                        //  this is where you query, because everything is cleared.
                    for (h = 0; h < totalProcessCounter; h++) {
                            //h = h + 1; // sync w child process?
                        if (validChild == 0) {
                            sprintf(switchProc, "1");
                            write_to_pipe(fd[h][PARENT][WRITE], switchProc);

                            while (SIFLAG == 1 || read(fd[h][CHILD][READ], &byte2, 1) == 1) {
                                    if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
                                        sigintProcnannies();
                                        goto completeProcess;
                                    }
                                    if (ioctl(fd[h][CHILD][READ], FIONREAD, &count2) != -1) {
                                        buff2[0] = byte2;
                                        if (SIFLAG == 1 || read(fd[h][CHILD][READ], buff2+1, count2) == count2) {
                                            if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
                                                sigintProcnannies();
                                                goto completeProcess;
                                            }
                                            int tmpinitval = atoi(buff2);
                                            if (tmpinitval == 2) {
                                                fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
                                            } else if (tmpinitval == 1) {
                                                // pass all the variables required here.
                                                // the child can: a. pass the variables and simply open a new process to monitor, or
                                                // b. reset the process UP THERE and rerun the forked process - which is probably better imo...
                                                //printf("Sending data to child now...");
                                                char idToMonitor[150];
                                                sprintf(idToMonitor, "%s %d", appdata[counter], timedata[counter]);
                                                write_to_pipe(fd[h][PARENT][WRITE], idToMonitor);
                                                validChild = 1;
                                                fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
                                            } else { // it got a bad value in the pipe... send it back! 
                                                write_to_pipe(fd[h][CHILD][READ], buff2);
                                                fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
                                            }
                                        }
                                    }
                                }
                                fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
                            }
                        }
                        if (validChild == 0) {
                            // FORK PROCESS HERE @ THIS POINT. HAVE TO FORK...?
                            // set the i value (counter val) as the one you're using
                            // fork here; the child is redirected to the child process UP THERE
                            // the parent is just going to pass and do nothing i guess.
                            //printf("Child not found - sending to new node now");
                            char grepip[1000];
                            strcpy(grepip, "pgrep ");
                            strcat(grepip, appdata[counter]);
                            //printf("grepip: %s\n", grepip);
                            if ( ( f[counter] = popen( grepip, "r" ) ) == NULL ) {
                                perror( "popen" );
                            } else {
                                char pidval[150];
                                bzero(pidval, 150);

                                if (pipe(fd[counter][PARENT]) < 0) {
                                    exit(EXIT_FAILURE); 
                                }
                                if (pipe(fd[counter][CHILD]) < 0) {
                                    exit(EXIT_FAILURE); 
                                }
                                
                                fcntl(fd[counter][CHILD][READ], F_SETFL, O_NONBLOCK);

                                while (fgets(pidval, 150, f[counter]) != NULL) {
                                    pid = fork();
                                    if (pid < 0) { // error process
                                        fprintf(stderr, "can't fork, error %d\n", errno);
                                        exit(EXIT_FAILURE);

                                    } else if (pid > 0) { // parent process
                                        totalProcessCounter = totalProcessCounter + 1;

                                    } else if (pid == 0 ) { // child process

                                        signal(SIGINT, fail_function);

                                        strcpy(appProcessed, appdata[counter]);
                                        timeProcessed = timedata[counter];
                                        //printf("PIDVAL: %s\n", pidval);
                                        strcpy(newpid, pidval);
                                        //printf("NEWPID222222: %s\n", newpid);
                                        isnewnode = 1;
                                        goto childMonitoring;
                                    }
                                }
                            }
                        }
                    	//countval = 0;
                    	break;
                    }
                    counter++;
                }
                //}
                //fclose(file2);
                SHFLAG = 0;
                goto parentMonitoring;
            } 
            if (ioctl(fd[k][CHILD][READ], FIONREAD, &count) == -1) {
                buff[0] = byte;
                if (k < totalProcessCounter) {
                    k++;
                } else {
                    k = 0;
                }
            } else if (ioctl(fd[k][CHILD][READ], FIONREAD, &count) != -1) {
		        //buff = malloc(count+1);
		        //bzero(buff, count+1);
              buff[0] = byte;
              if (read(fd[k][CHILD][READ], buff+1, count) == count) {
                bch = strtok (buff," ,.*");
                while (bch != NULL) {
                    if (countvalb == 0) {
                        pidstatus = atoi(bch);
                        if (pidstatus == 1) {
                            countProcCompleted++;
                            strcpy(appdata[k], "");
                            signum++;
                        } else if (pidstatus == 0) {
                            countProcCompleted++;
                        }
                        countvalb++;
                    } else if (countvalb == 1) {
                        char procCompleted[150];
                        strcpy(procCompleted, bch);
                        int p;
                        for (p = 0; p < (sizeof(appdata) / sizeof(appdata[0])); p++) {
                                if (strcmp(appdata[p], procCompleted) == 0) { //matches
                                    strcpy(appdata[p], "");
                                }
                            }
                            countvalb = 0;
                        }
                        bch = strtok (NULL, " ,.*");
                    }
                } 

                if (countProcCompleted == totalProcessCounter) {
                    goto parentMonitoring;
                }
                k++;
            }
            //printf("bug: out of while loop\n");
            if (countProcCompleted == totalProcessCounter) {
                goto parentMonitoring;
            }
        }
    }

    completeProcess:
    //consoleOP("Operations have concluded for this process (all iterations have gone through).");
    killProcessOP(signum);
    return 0;

    waitingProc:
    for (k = 0; k < totalProcessCounter; k++) {
        close(fd[k][PARENT][READ]);
        close(fd[k][PARENT][WRITE]);
        close(fd[k][CHILD][READ]);
        close(fd[k][CHILD][WRITE]);
    }
    
    return 0;
}

void read_from_server(int filedes) {
	char buffer[MAXMSG];
	int nbytes;
  nbytes = read (filedes, buffer, MAXMSG);
  //printf("read_from_server statement\n");
  if (nbytes < 0) {
      	/* Read error. */
     perror ("read");
     exit (EXIT_FAILURE);
 } else if (nbytes == 0) {
    	/* End-of-file. */
        //return -1;
    return;
} else {
  		// parser here
    char * token;
    char * subtoken;
    char * subsubtoken;
  		token = strtok(buffer, "\n"); // grabs the first token... we don't care about the other ones I think.
  		//printf("READ: %s\n", token);
  		if (token != NULL) {
	  		if (strcmp(token, "1") == 0) { //simulate the killprocs
	  			sigintProcnannies();
               killProcessOP(signum);
               SIFLAG = 1;
           }
	  		if (strcmp(token, "3") == 0) { //simulate the sighup
	  			memset(&testread[0], 0, sizeof(testread));
                memset(&appread[0], 0, sizeof(appread));
                memset(&timeread[0], 0, sizeof(timeread));
                countread = 0;
                while(1) {

                   int countval = 0;
                   read (filedes, buffer, MAXMSG);
	  			  	subtoken = strtok(buffer, "\n"); // grabs the first token... we don't care about the other ones I think.
	  			  	//printf("SUBTOKEN: %s\n", subtoken);
	  			  	subsubtoken = strtok(subtoken, " ");
	  			  	while (subsubtoken != NULL) {
                      if (strcmp(subtoken, "EOF") != 0) {
                         //printf("SUBSUBTOKEN: %s\n", subsubtoken);
                         if (countval == 0) {
                            strcpy(appread[countread], subsubtoken);
                            countval++;
                        } else {
                            timeread[countread] = atoi(subsubtoken);
                            countval = 0;
                            countread++;
                        }
                    }
                    else {
                     //printf("Parsing completed; returning.\n");
                     SHFLAG = 1;
                     return;
                 }
                 subsubtoken = strtok(NULL, " ");
             }
	  				// wait till it reads '4' then end.
         } 
     }
 }
}
}

void write_to_pipe (int file, char* data)
{
  write(file, data, strlen(data));
}

void read_from_pipe (int file)
{
  FILE *stream;
  int c;
  stream = fdopen (file, "r");
  while ((c = fgetc (stream)) != EOF)
    putchar (c);
fclose (stream);
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

void sigintProcnannies() {
    FILE * pnfile;
    pid_t curpid;
    int killPID = 0;
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
                int sigk = kill(pidpn, SIGINT);
                if (sigk == 0) {
                    killPID++;
                }
            }
        }
    }
    fclose(pnfile); 
    char printNum[150];
    sprintf(printNum, "2 %d", killPID);
    //consoleOP(printNum);
    genericOPnotime(printNum);
    return;
}

void genericOP(char* data) {
    time_t ltime;
    time(&ltime); 
    char printop[150];
    sprintf(printop, "[%s] %s\n", strtok(ctime(&ltime), "\n"), data);    
    write(s, &printop, sizeof(printop));
    return;
}

void genericOPnotime(char * data) {
	char printop[150];
	sprintf(printop, "%s\n", data);    
    write(s, &printop, sizeof(printop));
    return;
}

void consoleOP(char * data) {
    time_t ltime;
    time(&ltime); 
    printf("#[%s] %s\n", strtok(ctime(&ltime), "\n"), data);
    return;
}

void killProcessOP(int signum) {
    char strresult[250];    
    char strTPC[30];
    sprintf(strTPC, "%d", signum);
    strcpy(strresult, "Info: Exiting. ");
    strcat(strresult, strTPC);
    strcat(strresult, " process(es) killed.");
    genericOP(strresult);
    return;
}

void pidKilledOP(char * pidval, char * appdata, char * timeStr) {
    char str[1000];
    strcpy(str, "5 ");
    strcat(str, pidval);
    strcat(str, " ");
    strcat(str, appdata);
    strcat(str, " ");
    strcat(str, hostname);
    strcat(str, " ");
    strcat(str, timeStr);
    /*
    strcpy(str, "Action: PID ");
    strcat(str, pidval);
    strcat(str, " (");
    strcat(str, appdata);
    strcat(str, ") on ");
    strcat(str, hostname);
    strcat(str, " killed after exceeding ");
    strcat(str, timeStr);
    strcat(str, " seconds.");*/
    genericOPnotime(str);
    return;
}

void noProcessOP(char * appdata) {
    char noProcess[150];
    strcpy(noProcess, "Info: No '");
    strcat(noProcess, appdata);
    strcat(noProcess, "' processes found on ");
    strcat(noProcess, hostname);
    strcat(noProcess, ".");
    genericOP(noProcess);
    return;
}

void getParentPID() {
    pid_t parent_pid = getpid();
    parentPID = getpid();
    printf("Host PID: %d\n", parent_pid);
    return;
}

void initProcOP(char * appdata, char * pidval ) {
    char str[1000];
    strcpy(str, "Info: Initializing monitoring of process '");
    strcat(str, appdata);
    strcat(str, "' (PID ");
        strcat(str, pidval);
        strcat(str, ") on node ");
        strcat(str, hostname);
        strcat(str, ".");
        genericOP(str); 
        return;
    }

    void initialisationOP() {
        char strPID[1000];
        sprintf(strPID, "Info: Parent process is PID  %d.", parentPID);
        //consoleOP(strPID);
        return;
    }
