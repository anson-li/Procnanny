# PROCNANNY
Procnanny is an application in which processes are monitored and killed if it exceeds a predetermined time limit. If a program exceeds its time limit, it is killed. The resulting data and information is stored within the logfile.

## How to use:
* Delete MEMWATCH.h and MEMWATCH.c as is in the current directory system and place your own.
* Set environment variable as export PROCNANNYLOGS="../tmp/logfile.txt" ... you can test this with echo PROCNANNYLOGS. 
* Set environment variable as export PROCNANNYSERVERINFO="../tmp/logfile.info"
* Then run make.
* Then run ./procnanny nanny.config.

## Paths to use:
* ../tmp/logfile.txt has the logfile data
* ../src/ contains all the processing data
* ../test/ contains all of the test applications. There are three in total - the sole purpose of these test applications is to be killed by the procnanny application.

## How to call reread:
* kill -HUP 30124 (pid value to insert)
