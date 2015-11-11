/**
*	Processes still need to be completed:
* Terminate procnanny only after SIGINT is called: https://eclass.srv.ualberta.ca/mod/forum/discuss.php?d=560748
    IF SIGINT is called, then set a flag that makes procnanny terminate on read w/ the number of processes killed up to that pt.
    Send kill signal to the children; don't make them exit on their own. Make this an IPC as well
		THIS ONE IS DONE.
* Add *NEW* functionality to nanny.config and call it to run: https://eclass.srv.ualberta.ca/mod/page/view.php?id=1653150
    IF the process was already logged don't test it.
    IF the process is new, check if any child process can handle it; if all child processes return busy fork again...
    IF a child process is polling then make it monitor the old process.
**/

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
#include <fcntl.h>
#include "memwatch.h"

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

#define READ 0
#define WRITE 1 
#define CHILD 0
#define PARENT 1

static int parentPID; 
static int SHFLAG = 0;
static int SIFLAG = 0;

char test[280][1000]; //array of strings //length is 10! figure out how to realloc!
char appdata[280][1000];
int timedata[280];

char appProcessed[1000];
int timeProcessed;

static void catch_function(int signo) {
	SIFLAG = 1;
}

static void fail_function(int signo) {
	exit(EXIT_FAILURE);
}

static void ignore_function(int signo ) { 
	SHFLAG = 1;	
}

int main(int c, char *argv[]) {
	// pgrep other procnannies applications ... 
	// for each one, if it's not your current pid, kill process
	// there's a key combination which sends a hangup 
	/* DO THIS AT THE BEGINNING OF THE ASSIGNMENT*/
	signal(SIGHUP, ignore_function);
	signal(SIGINT, catch_function);

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
				perror( "popen" );
			} else {
				char pidval[150];
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

				 		fprintf(stderr, "can't fork, error %d\n", errno);
						exit(EXIT_FAILURE);

					} else if (pid > 0) { // parent process

						totalProcessCounter = totalProcessCounter + 1;

					} else if (pid == 0 ) { // child process

						signal(SIGINT, fail_function);

						strcpy(appProcessed, appdata[i]);
						timeProcessed = timedata[i];

						childMonitoring:;

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
								if (ioctl(fd[i][PARENT][READ], FIONREAD, &count) != -1)
	            	{
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
						int killresult = kill(pidint, SIGKILL);
						if (killresult == 0) {
							//printf("You killed the process (PID: %d) (Application: %s)\n", pidint, test[i] );
							pidKilledOP(pidval, appProcessed, timeStr);
							sprintf(prntChild, "1 %s", appProcessed);
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
													bch = strtok (buff," ,.-");
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
																perror( "popen" );
															} else {
																if (fgets(pidval, 150, f[i]) != NULL) {
																	goto childMonitoring;
																} else {
																	noProcessOP(appdata[i]);
																	goto waitingProc;
																}
															}
														}
														bch = strtok (NULL, " ,.-");
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
	int signum = 0;
	char buff[1000];
	bzero(buff, 1000);
  char byte = 0;
  int count = 0;
  int h = 0;

	int pidstatus;
	char * bch;
	int countvalb = 0;
	int countProcCompleted = 0;

	if (totalProcessCounter == 0) {
		goto completeProcess;
	}

	parentMonitoring:;
	for (k = 0; k < totalProcessCounter; k++ ) {
		fcntl(fd[k][CHILD][READ], F_SETFL, O_NONBLOCK);
	}

	k = 0;
	while(1) {
		if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
			sigintProcnannies();
      goto completeProcess;
    }
    if (k < totalProcessCounter) {
			k++;
		} else {
			k = 0;
		}
		if (SHFLAG == 1 || read(fd[k][CHILD][READ], &byte, 1) == 1) {
			// if the ioctl is -1 then switch to the next k value...
			if (SHFLAG == 1) { // REREAD FILE
				int validChild = 0;
				char switchProc[1000];
				char buff2[1000];
				bzero(buff2, 1000);
  			char byte2 = 0;
  			char test2[280][1000];
  			int count2 = 0;

				consoleOP("Info: Caught SIGHUP. Configuration file 'nanny.config' re-read.");
				genericOP("Info: Caught SIGHUP. Configuration file 'nanny.config' re-read.");
				// if there's a new program then search, so search w/ respect to the current appname list
				FILE* file2 = fopen ( argv[1], "r" );

				if (file2 != NULL) {
					char line [1000];
					while (fgets(line, sizeof line, file2) != NULL) { // read a line from a file 
						// reads sample text: testa 120
						validChild = 0;
						counter = totalProcessCounter + 1;
						strcpy(test2[counter - 1], strtok(line, "\n"));
						pch = strtok (test2[counter-1]," ,.-");
						int validAppData = 0;
						while (pch != NULL) {
							if (countval == 0) {
								int m;
								for (m = 0; m < totalProcessCounter; m++) {
									validAppData = 0;
									if (strcmp(appdata[m], pch) == 0) {
										validAppData = 1;
									} 
								}
								if (validAppData == 0) {
									strcpy(appdata[counter], pch);
									countval++;
									pch = strtok (NULL, " ,.-");
								}
							} 
							if (countval == 1 && validAppData == 0) {
								timedata[counter] = atoi(pch);
								//  this is where you query, because everything is cleared.
								for (h = 0; h < totalProcessCounter; h++) {
									//h = h + 1; // sync w child process?
									if (validChild == 0) {
										sprintf(switchProc, "1");
										write_to_pipe(fd[h][PARENT][WRITE], switchProc);
										/*
										int ops;
										ops = fcntl(fd[h][CHILD][READ],F_GETFL); // reenable blocking
										fcntl(fd[h][CHILD][READ], F_SETFL, ops & ~O_NONBLOCK);
										*/
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
									char grepip[1000];
									strcpy(grepip, "pgrep ");
									strcat(grepip, appdata[counter]);
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

												goto childMonitoring;

											}
										}
									}
								}
							}
							countval = 0;
							break;
						}
						counter++;
					}
				}
				fclose(file2);
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
					bch = strtok (buff," ,.-");
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
						bch = strtok (NULL, " ,.-");
					}
        }        
        if (countProcCompleted == totalProcessCounter) {
        	goto parentMonitoring;
        }
        k++;
      }
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
		sprintf(printNum, "Info: Caught SIGINT. Exiting cleanly. %d process(es) killed.", killPID);
		consoleOP(printNum);
		genericOP(printNum);
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