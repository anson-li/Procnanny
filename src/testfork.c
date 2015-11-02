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

static int parentPID; 
static int SHFLAG = 0;

static void catch_function(int signo) {
	// if get_pid() is equal to the global variable (parent)
	// then hangup
	// if not ignore 
	puts("At this point you should kill all child processes... ");
}

static void ignore_function(int signo ) { 
	puts("Hangup!");
	printf("Parent ID Recognised: %d\n", parentPID);
	SHFLAG = 1;
	printf("SHFLAG: %d\n", SHFLAG);
	
	/*
	"you cant pass anything thru here :( "
	"so you have to put the polling in the main funciton, not here. .... . ."
	"only things you can manipulate thru here are globals "
	"make everyrhing a global? :o flags? "
	"my wait statement is wait(&status) ... it doesnt work now LOL"
	" i have to set polling status on that instead, and check on the flag change? to reread the file?"

	"ALSO! if i reread the file do i add all the new processes ? that means that i have to do comparators vs. all"
	" of the func names right? ><'' "
	"that means that if you wanted to monitor a diplicate process you couldnt do so (w/ different pid)... is that "
	"the corrrect processs flow?"

	"you keep track of the process ids that you're currently monitoring and if "
	"you reread the config file you recheck all proceses - if the process is in the list of already monitored processes, you"
	"skip to the next one "
	"also check every 5s via <<pipe>>"
	*/
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
		char line [1000];
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
	int fd[counter][2];


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

				if (pipe(fd[i]) < 0) {
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
						totalProcessCounter = totalProcessCounter + 1;;
						// close(fd[i][0]); // fd[i][0] needs to read the child kill process.
					} else if (pid == 0 ) { // child process
						// close(fd[i][1]); // fd[i][1] needs to write the child kill process.
						strtok(pidval, "\n");
						initProcOP(appdata[i], pidval);

						pidint = (pid_t) strtol(pidval, NULL, 10);
						int sleepLeft = timedata[i];
						while(sleepLeft > 0) {
							printf("Process %s is running with %d seconds left.\n", pidval, sleepLeft);
							if (SHFLAG == 1) {
								consoleOP("SHFLAG VAL 1 RECOGNISED.");
								// not a good way of doing it (not for multiple values ...)
								// find a way to do it using i/o pipes.
								// eg. constantly poll for fd[x][0] == -1 
								// if it's that then close everything there!
							}
							sleep(5);
							sleepLeft = sleepLeft - 5;
						}
						char timeStr[30];
						sprintf(timeStr, "%d", timedata[i]);
						int killresult = kill(pidint, SIGKILL);
							if (killresult == 0) {
								//printf("You killed the process (PID: %d) (Application: %s)\n", pidint, test[i] );
								pidKilledOP(pidval, appdata[i], timeStr);
							} else if (killresult == -1) {
								//printf("ERROR: Process already killed (PID: %d) (Application: %s)\n", pidint, test[i] );
							}
							exit(EXIT_SUCCESS); 
						} 
					}
					if (haspid == 0) {
					//printf("No processes found for %s.\n", test[i]);
					noProcessOP(appdata[i]);
				}		
			}
			pclose(f[i]);
			close(fd[i][0]);
			close(fd[i][1]);
		}
	}	

	// if hangup signal is presented, reread the config and print output
	// * don't do anything yet.
	// kill -1 PIDVAL
  if (signal(SIGHUP, ignore_function) == SIG_ERR) {
		/*
		consoleOP("Info: Caught SIGHUP. Configuration file 'nanny.config' re-read.");    
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
  }

	//printf("Total processes monitored: %d\n", totalProcessCounter);
	int k; 
	int status;
	int signum = 0;
	//int l;
 
	for (k = 0; k < totalProcessCounter; k++) {
		wait(&status);
		printf("Status value: %d\n", status);
		if(status == 0) {
			signum = signum + 1;
		}
	}

	for (k = 0; k < totalProcessCounter; k++) {
		close(fd[k][0]);
		close(fd[k][1]);
	}
	
	killProcessOP(signum);
	//printf("*operations have concluded for this process (all iterations have gone through).*\n");
	return 0;
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
	sprintf(strPID, "ProcNanny initialised with PID %d.", parentPID);
	consoleOP(strPID);
	return;
}