# CMPUT 291 Assignment 2 - PROCNANNY
Procnanny is an application in which processes are monitored and killed if it exceeds a predetermined time limit. If a program exceeds its time limit, it is killed. The resulting data and information is stored within the logfile.

## Name & Descriptor:
* Anson Li, 1354766
* Unix ID: amli
* Lecture Section: EA1
* Lab Section: ED04
* TA: Soham Sinha, <soham@ualberta.ca>

## How to use:
* Delete MEMWATCH.h and MEMWATCH.c as is in the current directory system and place your own.
* Set environment variable as export PROCNANNYLOGS="../tmp/logfile.txt" ... you can test this with echo PROCNANNYLOGS. 
* Then run make.
* Then run ./procnanny nanny.config.

## Collaborators:
* Discussed details of the project with Jake Davidson, Mohamed Salim Ben Khaled

## Paths to use:
* ../tmp/logfile.txt has the logfile data
* ../src/ contains all the processing data
* ../test/ contains all of the test applications. There are three in total - the sole purpose of these test applications is to be killed by the procnanny application.

## How to call reread:
* kill -HUP 30124 (pid value to insert)

## Hints / notes for Assignment 2:
* Have to pipe, then fork after the pipe is created.
* fd[CHILD][READ] is always the read,
* fd[CHILD][WRITE] is always the write.
* We have to use this process to communicate changes; whether or not it's to check if the system is running, or to check if the system should terminate. Permanently polling system?
* Duplicate gives you three different functions (read the manpage for dup2 for more information):
	* if you give your file descriptor, then your duplicate will create a copy of the file descriptor with the lowest number of descriptor (file of 1).
	* But if you use dup2(data, 0) it will duplicate your standard input...
		* 0 is standard input.
		* 1 is standard output.
		* 2 is standard error.
		* However, if the number is already in use, the program will delete the standard input and bring in the new one.
	* This is used if the program 'thinks' its reading from standard input, but is actually read pipe information that's duplicated from the initial location ... then the program can change its settings on the fly.