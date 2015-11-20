procnanny.server: server.c
	gcc -Wall -DMEMWATCH -DMW_STDIO -o procnanny.server server.c memwatch.c

procnanny.client: client.c
	gcc -Wall -DMEMWATCH -DMW_STDIO -o procnanny.client client.c memwatch.c
clean:
	rm -f procnanny *.o core
#testfork: testfork.c
#replace with gcc -o -Wall -g testfork testfork.c
#./testfork
