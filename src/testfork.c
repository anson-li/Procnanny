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

void writedis(char* data);

int main(int c, char *argv[])
{
    
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
   	
    // pgrep other procnannies applications ... 
    // for each one, if it's not your current pid, kill process
	
    const char* s = getenv("PROCNANNYLOGS"); 
    FILE* logfile = fopen(s, "w");
    fclose(logfile);    

    FILE* file = fopen ( argv[1], "r" );

    if (file == NULL) {
        printf("ERROR: Nanny.config file presented was not found. Program exiting...");
    	exit(EXIT_FAILURE);  
    }

    int counter = 0;
    int timeVar = 0;
    char test[280][1000]; //array of strings //length is 10! figure out how to realloc!

    //setenv("PROCNANNYLOGS", "../tmp/logfile.txt", 0);
    // environment variables will be instantiated outside of the process

    if (file != NULL) {
        char line [1000];
        while (fgets(line, sizeof line, file) != NULL) { /* read a line from a file */
            //fprintf(stdout,"%s",line); //print the file contents on stdout.
            if (counter == 0) {
                timeVar = atoi(line);
            }
            else {
                //test = realloc(test, counter * char[100].sizeof);
                strcpy(test[counter - 1], strtok(line, "\n"));
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
	if (test[i][0] != '\0') { 
            //printf("Application Testing: %s\n", test[i]);
	    char grepip[1000];
	    strcpy(grepip, "pgrep ");
	    strcat(grepip, test[i]);
	    if ( ( f[i] = popen( grepip, "r" ) ) == NULL ) {
	        perror( "popen" );
            } else {
	        char str[1000];
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
		    } else if (pid == 0 ){ // child process
			
			strtok(pidval, "\n");
			strcpy(str, "Info: Initializing monitoring of process '");
                	strcat(str, test[i]);
                	strcat(str, "' (PID ");
                	strcat(str, pidval);
			strcat(str, ").");
			writedis(str);
        
			pidint = (pid_t) strtol(pidval, NULL, 10);
			printf("PID (%d) Running\n", pidint);
			sleep(timeVar);
			char timeStr[30];
			sprintf(timeStr, "%d", timeVar);
			int killresult = kill(pidint, SIGKILL);
			if (killresult == 0) {
			    //printf("You killed the process (PID: %d) (Application: %s)\n", pidint, test[i] );
			    strcpy(str, "Action: PID ");
                	    strcat(str, pidval);
                	    strcat(str, " (");
                	    strcat(str, test[i]);
			    strcat(str, ") killed after exceeding ");
			    strcat(str, timeStr);
			    strcat(str, " seconds.");
			    writedis(str);
       
			} else if (killresult == -1) {
			    //printf("ERROR: Process already killed (PID: %d) (Application: %s)\n", pidint, test[i] );
			}
			exit(EXIT_SUCCESS); 
		    } 
		}

		if (haspid == 0) {
		    char noProcess[150];
		    //printf("No processes found for %s.\n", test[i]);
		    strcpy(noProcess, "Info: No '");
		    strcat(noProcess, test[i]);
		    strcat(noProcess, "' processes found.");
		    writedis(noProcess);
		}		
            }
	    pclose(f[i]);
	}	
    }

    //printf("Total processes monitored: %d\n", totalProcessCounter);
    int k; 
    int status;
    char strresult[250];    
 
    for (k = 0; k < totalProcessCounter; k++) {
	wait(&status);
    }
	
    char strTPC[30];
    sprintf(strTPC, "%d", totalProcessCounter);
    strcpy(strresult, "Info: Exiting. ");
    strcat(strresult, strTPC);
    strcat(strresult, " process(es) killed.");
    writedis(strresult); 

    //printf("*operations have concluded for this process (all iterations have gone through).*\n");
    return 0;
}

void writedis(char* data) {
    const char* s = getenv("PROCNANNYLOGS");
    FILE* file= fopen (s, "a" );
    time_t ltime;
    time(&ltime); 
    fprintf(file, "[%s] %s\n", strtok(ctime(&ltime), "\n"), data);
    fclose(file);
}

