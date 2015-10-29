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
void killProcessOP(int signum);
void pidKilledOP(char * pidval, char * appdata, char * timeStr);
void noProcessOP(char * appdata);
void initProcOP(char * appdata, char * pidval );
void deleteProcnannies();

int main(int c, char *argv[]) {
	// pgrep other procnannies applications ... 
	// for each one, if it's not your current pid, kill process
	deleteProcnannies();
	
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
	
				while (fgets(pidval, 150, f[i]) != NULL) {
					haspid = 1;
					pid = fork();
					if (pid < 0) { //
				 		fprintf(stderr, "can't fork, error %d\n", errno);
						exit(EXIT_FAILURE);
					} else if (pid > 0) { // parent process
						totalProcessCounter = totalProcessCounter + 1;;
					} else if (pid == 0 ) { // child process
						strtok(pidval, "\n");
						initProcOP(appdata[i], pidval);

						pidint = (pid_t) strtol(pidval, NULL, 10);
						sleep(timedata[i]);
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
	}
	}	

	//printf("Total processes monitored: %d\n", totalProcessCounter);
	int k; 
	int status;
	int signum = 0;
 
	for (k = 0; k < totalProcessCounter; k++) {
		wait(&status);
		printf("Status value: %d\n", status);
		if(status == 0) {
			signum = signum + 1;
		}
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