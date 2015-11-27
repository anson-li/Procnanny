#include<signal.h>
#include<stdio.h>
#include<string.h>
#include<sys/select.h>

/* Written by Zhaochen Guo <zhaochen@cs.ualberta.ca> */

/**
 * A utility function used for monitoring a file descriptors, waiting 
 * until the fd becomes "ready" for read.
 *
 * @param fd	The file descriptor to be monitored.
 * @param timeout	The upper bound on the amount of time(seconds) 
 * elapsed before this function returns. Zeros means this function 
 * returns immediately. -1 to ask the function to block indefinitely
 * (until fd is "ready" for read).
 *
 * @return -1 An error happens when function is called.
 * 			0 Timeout expires before fd becomes ready for read.
 * 			1 The file descriptor becomes ready for read.
 */
int my_alarm(int fd, long timeout) {
	int maxdesc, ret;
	fd_set read_from;
	struct timeval tv;

	maxdesc = getdtablesize();
	FD_ZERO(&read_from);
	FD_SET(fd, &read_from);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	if (timeout < 0)
		ret = select(maxdesc, &read_from, NULL, NULL, NULL);
	else
		ret = select(maxdesc, &read_from, NULL, NULL, &tv);

	return ret;
}

/*
int main(void) {
	int ret = my_alarm(0, 10);
	if (ret < 0)
		printf("error\n");
	else if (ret == 0) 	//timeout
		printf("timeout\n");
	else	//file ready for read.
	{
		char buf[256];
		memset(buf, 0, 256);
		ret = read(0, buf, 256);
		printf("%s\n", buf);
	}

	return 0;
}
*/


