#include <stdio.h>   /* printf, stderr, fprintf */
#include <sys/types.h> /* pid_t */
#include <unistd.h>  /* _exit, fork */
#include <stdlib.h>  /* exit */
#include <errno.h>   /* errno */
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "memwatch.h"

void genericOP(char* data);
void consoleOP(char* data);
void killProcessOP(int signum);
void pidKilledOP(char * pidval, char * appdata, char * timeStr);
void noProcessOP(char * appdata);
void initProcOP(char * appdata, char * pidval );
void deleteProcnannies();
void getParentPID();
void initialisationOP();
void write_to_pipe(int file, char * data);
void read_from_pipe(int file);

#define READ 0
#define WRITE 1 
#define CHILD 0
#define PARENT 1

static int parentPID; 
static int SHFLAG = 0;

static void catch_function(int signo) {
	// if get_pid() is equal to the global variable (parent)
	// then hangup
	// if not ignore 
	puts("At this point you should kill all child processes... ");
	exit(EXIT_FAILURE);
}

static void ignore_function(int signo ) { 
	puts("SHFlag set.");
	SHFLAG = 1;	
}

int main(int c, char *argv[]) {
	// pgrep other procnannies applications ... 
	// for each one, if it's not your current pid, kill process
	// there's a key combination which sends a hangup 
	/* DO THIS AT THE BEGINNING OF THE ASSIGNMENT*/
	if (signal(SIGINT, catch_function) == SIG_ERR) {
    fputs("An error occurred while setting a signal handler.\n", stderr);
    return EXIT_FAILURE;
  }

	deleteProcnannies();
	getParentPID();
	initialisationOP();
	
	const char* s = getenv("PROCNANNYLOGS"); 
	FILE* logfile = fopen(s, "w");
	fclose(logfile);   

	FILE* file = fopen ( argv[1], "r" );

	if (file == NULL) {
		printf("ERROR: Nanny.config file presented was not found. Program exiting...");
		exit(EXIT_FAILURE);  
	}

	int counter = 0;
	char test[280][1000]; //array of strings //length is 10! figure out how to realloc!
	char appdata[280][1000];
	int timedata[280];
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

	FILE* f[counter - 1];
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
				perror( "popen" );
			} else {
				char pidval[150];
				pid_t pidint;
				int haspid = 0;

				if (pipe(fd[i][PARENT]) < 0) {
					printf("ERROR: ProcNanny cannot create pipe. Program exiting...");
					exit(EXIT_FAILURE); 
				}
				if (pipe(fd[i][CHILD]) < 0) {
					printf("ERROR: ProcNanny cannot create pipe. Program exiting...");
					exit(EXIT_FAILURE); 
				}
	
				while (fgets(pidval, 150, f[i]) != NULL) {
					haspid = 1;
					pid = fork();
					if (pid < 0) { // error process
				 		fprintf(stderr, "can't fork, error %d\n", errno);
						exit(EXIT_FAILURE);
					} else if (pid > 0) { // parent process
						close(fd[i][CHILD][WRITE]);
						close(fd[i][PARENT][READ]);
						totalProcessCounter = totalProcessCounter + 1;;
					} else if (pid == 0 ) { // child process
						childMonitoring:
						close(fd[i][PARENT][WRITE]); 
						close(fd[i][CHILD][READ]);

						printf("this is 1\n");

						strtok(pidval, "\n");
						initProcOP(appdata[i], pidval);

						pidint = (pid_t) strtol(pidval, NULL, 10);
						int sleepLeft = timedata[i];
						while(sleepLeft > 0) {
							printf("Process %s is running with %d seconds left.\n", pidval, sleepLeft);
							
							sleep(5);
							sleepLeft = sleepLeft - 5;
						}
						char timeStr[30];
						sprintf(timeStr, "%d", timedata[i]);
						char prntChild[150];
						int killresult = kill(pidint, SIGKILL);
							if (killresult == 0) {
								//printf("You killed the process (PID: %d) (Application: %s)\n", pidint, test[i] );
								pidKilledOP(pidval, appdata[i], timeStr);
								sprintf(prntChild, "1 %s", pidval);
							} else if (killresult == -1) {
								//printf("ERROR: Process already killed (PID: %d) (Application: %s)\n", pidint, test[i] );
							  sprintf(prntChild, "0 %s", pidval);
							}
							consoleOP("Process monitoring complete.");
							write_to_pipe(fd[i][CHILD][WRITE], prntChild);
							
							int count = 0;
							int signum = 0;
							char buff[1000];
							bzero(buff, 1000);
  						char byte = 0;

							while (read(fd[i][PARENT][READ], &byte, 1) == 1) {
								if (ioctl(fd[i][PARENT][READ], FIONREAD, &count) != -1) {
									buff[0] = byte;
									if (read(fd[i][PARENT][READ], buff+1, count) == count) {
										printf("%s\n", buff);
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
			pclose(f[i]);
		}
	}	

  if (signal(SIGHUP, ignore_function) == SIG_ERR) {}

	int k = 0; 
	int signum = 0;
	char buff[1000];
	bzero(buff, 1000);
  char byte = 0;
  int count = 0;
  int h = 0;

	int pidchange;
	int pidstatus;
	char * bch;
	int countvalb = 0;
	int countProcCompleted = 0;

	printf("The totalProcessCounter is: %d\n", totalProcessCounter);
	if (totalProcessCounter == 0) {
		goto completeProcess;
	}

	while(1) {
		//for (k = 0; k < totalProcessCounter; k++) {
		while (read(fd[k][CHILD][READ], &byte, 1) == 1) {
			// if the ioctl is -1 then switch to the next k value...
			printf("K value is: %d\n", k);
			if (SHFLAG == 1) {
				consoleOP("Info: Caught SIGHUP. Configuration file 'nanny.config' re-read.");
				genericOP("Info: Caught SIGHUP. Configuration file 'nanny.config' re-read.");
				/*
				file = fopen ( argv[1], "r" );
				if (file != NULL) {
					char line [1000];
					while (fgets(line, sizeof line, file) != NULL) { // read a line from a file 
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
				*/
				char switchProc[1000];
				for (h = 0; h < totalProcessCounter; h++) {
					sprintf(switchProc, "1 PROCNAME");
					write_to_pipe(fd[h][PARENT][WRITE], switchProc);
				}
				SHFLAG = 0;
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
        countProcCompleted++;
        if (read(fd[k][CHILD][READ], buff+1, count) == count) {
					bch = strtok (buff," ,.-");
					while (bch != NULL) {
						printf("BCH: %s\n", bch);
						if (countvalb == 0) {
							pidstatus = atoi(bch);
							printf("PIDSTATUS: %d\n", pidstatus);
							countvalb++;
						} else if (countvalb == 1) {
							pidchange = atoi(bch);
							printf("PIDCHANGE: %d\n", pidchange);
							countvalb = 0;
						}
						bch = strtok (NULL, " ,.-");
					}
        }
        if (totalProcessCounter == countProcCompleted) {
        	goto completeProcess;
        }
        k++;
      }
    }
	}

	completeProcess:
	consoleOP("Operations have concluded for this process (all iterations have gone through).");

	waitingProc:
	for (k = 0; k < totalProcessCounter; k++) {
		close(fd[k][PARENT][READ]);
		close(fd[k][PARENT][WRITE]);
		close(fd[k][CHILD][READ]);
		close(fd[k][CHILD][WRITE]);
	}
	
	killProcessOP(signum);
	return 0;
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
	strcpy(str, "Action: PID ");
	strcat(str, pidval);
	strcat(str, " (");
	strcat(str, appdata);
	strcat(str, ") killed after exceeding ");
	strcat(str, timeStr);
	strcat(str, " seconds.");
	genericOP(str);
	return;
}

void noProcessOP(char * appdata) {
	char noProcess[150];
	strcpy(noProcess, "Info: No '");
	strcat(noProcess, appdata);
	strcat(noProcess, "' processes found.");
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
	strcat(str, ").");
	genericOP(str);	
	return;
}

void initialisationOP() {
	char strPID[1000];
	sprintf(strPID, "Info: Parent process is PID  %d.", parentPID);
	consoleOP(strPID);
	return;
}