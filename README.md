# CMPUT 291 Assignment 1 - PROCNANNY
Procnanny is a development framework of which processes are monitored and killed if it exceeds the time limit.

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
* ../test/ contains all of the test applications. There are three in total
