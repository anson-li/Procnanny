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

static void catch_function(int signo) {
	puts("SIFLAG set.");
	SIFLAG = 1;
}

static void fail_function(int signo) {
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

						signal(SIGINT, fail_function);

						childMonitoring:
						close(fd[i][PARENT][WRITE]); 
						close(fd[i][CHILD][READ]);

					  int count = 0;
						char buff[1000];
						bzero(buff, 1000);
  					char byte = 0;

						strtok(pidval, "\n");
						initProcOP(appdata[i], pidval);

						pidint = (pid_t) strtol(pidval, NULL, 10);
						fcntl(fd[i][PARENT][READ], F_SETFL, O_NONBLOCK);

						int sleepLeft = timedata[i];
						while(sleepLeft > 0) {
							printf("Process %s is running with %d seconds left.\n", pidval, sleepLeft);
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
	            			printf("charbuf: %d\n", charbuf);
	            			if (charbuf == 1) {
											printf("1 found, skipping to next one. i = %d\n", i);
											sprintf(printWrite, "2"); // sends for onwait process
	            				write_to_pipe(fd[i][CHILD][WRITE], printWrite);
										} else if (charbuf == -1) {
											printf("Process terminated.\n");
											exit(EXIT_FAILURE);
	            			}
	            		}
	            	}
	            }
	            // inform busy
							sleepLeft = sleepLeft - 5;
						}
						char timeStr[30];
						sprintf(timeStr, "%d", timedata[i]);
						char prntChild[150];
						printf("pidint: %d\n", pidint);
						int killresult = kill(pidint, SIGKILL);
						if (killresult == 0) {
							//printf("You killed the process (PID: %d) (Application: %s)\n", pidint, test[i] );
<<<<<<< HEAD
							//pidKilledOP(pidval, appProcessed, timeStr);
							sprintf(prntChild, "1 %s %s %s", pidval, appProcessed, timeStr);
=======
							pidKilledOP(pidval, appdata[i], timeStr);
							sprintf(prntChild, "1 %s", pidval);
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
						} else if (killresult == -1) {
							//printf("ERROR: Process already killed (PID: %d) (Application: %s)\n", pidint, test[i] );
						  sprintf(prntChild, "0 %s", pidval);
						}
						printf("%s\n", prntChild);
						consoleOP("Process monitoring complete.");
<<<<<<< HEAD
						pollingChild:;
						//pclose(f[i]);
						printf("i child is: %d\n", i);
=======
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
						write_to_pipe(fd[i][CHILD][WRITE], prntChild); // writing to parent that is polling
						printf("Write succeeded at i = %d\n", i);
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
									printf("Buff read for new process: %d\n", bufval);
									if (bufval == -1) {
										printf(":(\n");
										goto waitingProc;
									} else if (bufval == 1) {
										// new process started!
										char prntReady[150];
										printf("this one is free!.\n");
										sprintf(prntReady, "0"); // sends for onwait process
            				write_to_pipe(fd[i][CHILD][WRITE], prntReady);
            				// NEEDS TO WAIT @ THIS POINT
<<<<<<< HEAD
            				printf("I value initial: %d\n", i);
            				while (read(fd[i][PARENT][READ], &byte, 1) == 1) {
            					buff[0] = byte;
											if (ioctl(fd[i][PARENT][READ], FIONREAD, &count) != -1) {
												buff[0] = byte;
												if (read(fd[i][PARENT][READ], buff+1, count) == count) {
													printf("I value second: %d\n", i);
													char * bch;
													int countvalb = 0;
													// read the buff value
													// parse buff into 2 parts;
													printf("%s\n", buff);
													bch = strtok (buff," ,.-");
													while (bch != NULL) {
														if (countvalb == 0) {
															strcpy(appProcessed, bch);
															printf("NEW app to be processed: %s\n", appProcessed);
															countvalb++;
														} else if (countvalb == 1) {
															timeProcessed = atoi(bch);
															printf("NEW time to be processed: %d\n", timeProcessed);
															bzero(bch, 1000);
															countvalb = 0;
															strcpy(grepip, "pgrep ");
															strcat(grepip, appProcessed);
															printf("i value before opening is: %d\n", i);
															if ( ( f[i] = popen( grepip, "r" ) ) == NULL ) {
																perror( "popen" );
															} else {
																if (fgets(pidval, 150, f[i]) != NULL) {
																	printf("pidval: %s\n", pidval);
																	goto childMonitoring;
																} else {
																	noProcessOP(appdata[i]);
																	goto pollingChild;
																}
															}
														}
														bch = strtok (NULL, " ,.-");
													}
												}
											}
										}
=======
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
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

	printf("The totalProcessCounter is: %d\n", totalProcessCounter);
	if (totalProcessCounter == 0) {
		goto completeProcess;
	}

	parentMonitoring:;
	for (k = 0; k < totalProcessCounter; k++ ) {
		fcntl(fd[k][CHILD][READ], F_SETFL, O_NONBLOCK);
	}

<<<<<<< HEAD
=======
	parentMonitoring:
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
	k = 0;
	printf("im here now\n");
	while(1) {
		if (SIFLAG == 1) { //setup to prevent early completion via sighup... 
			printf("Statement completed?\n");
			/*
			for (k = 0; k < totalProcessCounter; k++ ) {
				//k++;
				//char failState[150];
				//sprintf(failState, "-1");
				//write_to_pipe(fd[k][PARENT][WRITE], failState);
				//kill(pidlist[k], SIGINT);
			}
			*/
			sigintProcnannies();
      goto completeProcess;
    }
<<<<<<< HEAD
    if (k < totalProcessCounter) {
			k++;
		} else {
			k = 0;
		}
		if (SHFLAG == 1 || read(fd[k][CHILD][READ], &byte, 1) == 1) {
			printf("K value now is: %d\n", k);
=======
		//for (k = 0; k < totalProcessCounter; k++) {
		while (SHFLAG == 1 || read(fd[k][CHILD][READ], &byte, 1) == 1) {
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
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
				counter = totalProcessCounter + 1;

				if (file2 != NULL) {
					char line [1000];
					while (fgets(line, sizeof line, file2) != NULL) { // read a line from a file 
						// reads sample text: testa 120
<<<<<<< HEAD
						validChild = 0;
						counter = totalProcessCounter + 1;
						printf("LINE READ FROM FILE: %s\n", line);
=======
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
						strcpy(test2[counter - 1], strtok(line, "\n"));
						printf("Test2: %s\n", test2[counter - 1]);
						pch = strtok (test2[counter-1]," ,.-");
						int validAppData = 0;
						while (pch != NULL) {
							if (countval == 0) {
								int m;
								for (m = 0; m < totalProcessCounter; m++) {
									validAppData = 0;
									printf("Comparison between two values: Test2: %s; Appdata: %s\n", pch, appdata[m]);
									if (strcmp(appdata[m], pch) == 0) {
										printf("Duplicate function '%s' already exists; ignoring this one.\n", pch);
										validAppData = 1;
									} 
								}
								if (validAppData == 0) {
									strcpy(appdata[counter], pch);
									printf("Appdata new: %s\n", appdata[counter]);
								}
								countval++;
							} else if (countval == 1 && validAppData == 0) {
								timedata[counter] = atoi(pch);
								printf("Timedata new: %d\n", timedata[counter]);

								//  this is where you query, because everything is cleared.
								for (h = 0; h < totalProcessCounter; h++) {
									//h = h + 1; // sync w child process?
<<<<<<< HEAD
									if (validChild == 0) {
										sprintf(switchProc, "1");
										write_to_pipe(fd[h][PARENT][WRITE], switchProc);
										// just print 1
										int ops;
										ops = fcntl(fd[h][CHILD][READ],F_GETFL); // reenable blocking
										fcntl(fd[h][CHILD][READ], F_SETFL, ops & ~O_NONBLOCK);
								  	while (read(fd[h][CHILD][READ], &byte2, 1) == 1) {
								  		printf("PASS ONE\n");
								  		buff2[0] = byte2;
											if (ioctl(fd[h][CHILD][READ], FIONREAD, &count2) != -1) {
												printf("PASS TWO\n");
												if (read(fd[h][CHILD][READ], buff2+1, count2) == count2) {
													//buff2[0] = byte2;
													printf("PASS THREE: buff2 = %s\n", buff2);
													int tmpinitval = atoi(buff2);
													if (tmpinitval == 2) {
														printf("2 found, skipping to next one.\n");
													} else if (tmpinitval == 0) {
														printf("using this child process to begin next process\n");
														// pass all the variables required here.
														// the child can: a. pass the variables and simply open a new process to monitor, or
														// b. reset the process UP THERE and rerun the forked process - which is probably better imo...
														char idToMonitor[150];
														sprintf(idToMonitor, "%s %d", appdata[counter], timedata[counter]);
														write_to_pipe(fd[h][PARENT][WRITE], idToMonitor);
														validChild = 1;
														fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
													} else {
														printf("? tmpinitval = %d\n", tmpinitval);
													}
													fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
=======
									sprintf(switchProc, "1");
									write_to_pipe(fd[h][PARENT][WRITE], switchProc);
									int ops;
									ops = fcntl(fd[h][CHILD][READ],F_GETFL); // reenable blocking
									fcntl(fd[h][CHILD][READ], F_SETFL, ops & ~O_NONBLOCK);
									printf("h val parent is: %d\n", h);
							  	while (read(fd[h][CHILD][READ], &byte2, 1) == 1) {
										if (ioctl(fd[h][CHILD][READ], FIONREAD, &count2) != -1) {
											buff2[0] = byte2;
											if (read(fd[h][CHILD][READ], buff2+1, count2) == count2) {
												printf("buff2 value: %s\n", buff2);
												int tmpinitval = atoi(buff2);
												if (tmpinitval == 2) {
													printf("2 found, skipping to next one.\n");
												} else if (tmpinitval == 0) {
													printf("using this child process to begin next process\n");
													//sprintf(idToMonitor, "%s %d", appdata[counter], timedata[counter]);
													//write_to_pipe(fd[h][PARENT][WRITE], idToMonitor);
													validChild = 1;
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
												}
												fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
											}
										}
									}
								}

<<<<<<< HEAD
								if (validChild == 0) {
									// FORK PROCESS HERE @ THIS POINT. HAVE TO FORK...?
									// set the i value (counter val) as the one you're using
									// fork here; the child is redirected to the child process UP THERE
									// the parent is just going to pass and do nothing i guess.
									totalProcessCounter++;
									i = counter - 1; // FIXME: dependent
									char grepip[1000];
									strcpy(grepip, "pgrep ");
									strcat(grepip, appdata[counter]);
									printf("grepip: %s\n", grepip);
									if ( ( f[counter] = popen( grepip, "r" ) ) == NULL ) {
										perror( "popen" );
									} else {
										char pidval[150];
										bzero(pidval, 150);

										printf("counter = %d\n", counter);

										if (pipe(fd[i][PARENT]) < 0) {
											printf("ERROR: ProcNanny cannot create pipe. Program exiting...");
											exit(EXIT_FAILURE); 
										}
										if (pipe(fd[i][CHILD]) < 0) {
											printf("ERROR: ProcNanny cannot create pipe. Program exiting...");
											exit(EXIT_FAILURE); 
										}
										
										while (fgets(pidval, 150, f[counter]) != NULL) {
											pid = fork();
											if (pid < 0) { // error process
										 		fprintf(stderr, "can't fork, error %d\n", errno);
												exit(EXIT_FAILURE);

											} else if (pid > 0) { // parent process

												close(fd[i][CHILD][WRITE]);
												close(fd[i][PARENT][READ]);
												//totalProcessCounter = totalProcessCounter + 1;
												fcntl(fd[i][CHILD][READ], F_SETFL, O_NONBLOCK);

											} else if (pid == 0 ) { // child process

												signal(SIGINT, fail_function);

												strcpy(appProcessed, appdata[counter]);
												timeProcessed = timedata[counter];

												close(fd[i][PARENT][WRITE]); 
												close(fd[i][CHILD][READ]);

												goto childMonitoring;
											}
										}
									}
								}
=======
								countval = 0;
								break;
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
							}
							pch = strtok (NULL, " ,.-");
						}
						if (validChild = 0) {
							// FORK PROCESS HERE @ THIS POINT. HAVE TO FORK...?
						}
						//counter++;
					}
				}
				fclose(file2);

				/*
				for (h = 0; h < totalProcessCounter; h++) {
					h = h + 1; // sync w child process?
					printf("h value: %d\n", h);
					sprintf(switchProc, "1");
					write_to_pipe(fd[h][PARENT][WRITE], switchProc);
					int ops;
					ops = fcntl(fd[h][CHILD][READ],F_GETFL); // reenable blocking
					fcntl(fd[h][CHILD][READ], F_SETFL, ops & ~O_NONBLOCK);
					printf("h val parent is: %d\n", h);
			  	while (read(fd[h][CHILD][READ], &byte2, 1) == 1) {
						if (ioctl(fd[h][CHILD][READ], FIONREAD, &count2) != -1) {
							buff2[0] = byte2;
							if (read(fd[h][CHILD][READ], buff2+1, count2) == count2) {
								printf("buff2 value: %s\n", buff2);
								int tmpinitval = atoi(buff2);
								if (tmpinitval == 2) {
									printf("2 found, skipping to next one.\n");
								} else if (tmpinitval == 0) {
									printf("using this child process to begin next process\n");
								}
								fcntl(fd[h][CHILD][READ], F_SETFL, O_NONBLOCK);
							}
						}
					}
				}
				*/
				SHFLAG = 0;
			} else {
				printf("SHFLAG = 0;...\n");
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
					char procCompleted[150];
					char timeStr[30];
					char pidval[30];
					int verifyComplete = 0;
					while (bch != NULL) {
						printf("BCH: %s\n", bch);
						if (countvalb == 0) {
							pidstatus = atoi(bch);
							printf("PIDSTATUS: %d\n", pidstatus);
							if (pidstatus == 1) {
								countProcCompleted++;
								printf("SIGNUM++\n");
								signum++;
								verifyComplete = 1;
							} else if (pidstatus == 0) {
        				countProcCompleted++;
							}
							countvalb++;
<<<<<<< HEAD
						} else if (countvalb == 1 && verifyComplete == 1) {
							strcpy(pidval, bch);
							countvalb++;
						} else if (countvalb == 2 && verifyComplete == 1) {
							strcpy(procCompleted, bch);
							int p;
							for (p = 0; p < (sizeof(appdata) / sizeof(appdata[0])); p++) {
								if (strcmp(appdata[p], procCompleted) == 0) { //matches
									printf("%s matched with database, removed.\n", appdata[p]);
									strcpy(appdata[p], "");
								}
							}
							countvalb++;
						} else if (countvalb == 3 && verifyComplete == 1) {
							strcpy(timeStr, bch);
							pidKilledOP(pidval, procCompleted, timeStr);
							printf("pidkilled is written.");
=======
						} else if (countvalb == 1) {
							pidchange = atoi(bch);
							printf("PIDCHANGE: %d\n", pidchange);
>>>>>>> parent of b3efc46... Basic functionality completed, bugfixing
							countvalb = 0;
						} 
						bch = strtok (NULL, " ,.-");
					}
        }        
        if (countProcCompleted == totalProcessCounter) {
        	goto parentMonitoring;
        }
        //k++;
      }
      if (countProcCompleted == totalProcessCounter) {
        goto parentMonitoring;
      }
    }
	}

	//printf("Heyo sombrero\n");

	completeProcess:
	consoleOP("Operations have concluded for this process (all iterations have gone through).");
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
					killPID++;
					kill(pidpn, SIGINT);
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