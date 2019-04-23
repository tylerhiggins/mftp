/*
Tyler Higgins
cs 360
mftpserve.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "mftp.h"

#define BACKLOG 4
/* Checks for debug flag */
int setDebug(int ac, char *av[]) {
	if(ac < 2 && ac > 2)
		return -1;
	if(strcmp(av[1], "-d") != 0)
		return -1;
	return 1;

}
/* Creating server address, arguments: server address and the
   listener file discriptor */
void setServAddr(struct sockaddr_in *sAddr, int *lfd) {
	memset(sAddr, 0, sizeof(sAddr));
	sAddr->sin_family = AF_INET;
	sAddr->sin_port = htons(PORT_NUM);
	sAddr->sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(*lfd, (struct sock_addr *)sAddr, sizeof(*sAddr)) < 0){
		perror("Parent: fatel error binding socket to port 49999 -> %s", strerror(errno));
		exit(1);
	}

}

int getHost(struct sockaddr_in *cAddr, char **n, int cid) {
	struct hostent hostEntry;
	char *hname = *n;
	if((hostEntry = gethostbyaddr(&cAddr->sin_addr, sizeof(struct in_addr),
		AF_INET)) == NULL) {
		printf("Child %d: Translation of client hostname failed -> %s\n", cid, 
			strerror(errno));
		return -1;
	}
	hname = hostEntry->h_name;
	return 0;

}


void client(int *cid, struct socaddr_in *cAddr, int *ctrlfd, int debug) {
	char *hostname;
	char buffer[BUF_SIZE];
	if(getHost(cAddr, &hostname, *cid) != 0) {
		hostname = inet_ntoa(cAddr->sin_addr);
		printf("Child %d: Client IP address %s\n", inet_ntoa(cAddr->sin_addr));
	}
	printf("Child %d: Conenction accepted from host %s\n", hostname);
	receiveCommand(char &buffer);

}
// TODO: Create Listener socket
// TODO: Create reader/writer functions
// TODO: fork client and server processes
// TODO: accept connections
// TODO: create data connection
int main(int argc, char *argv[]) {
	int debug = 0;
	if((debug = setDebug(argc, argv)) < 0) {
		fprintf(stderr, "usage: %s [-d]\n");
		exit(1);
	}
	if(debug)
		printf("Parent: debug mode enabled\n");
	int listenfd;
	createSocket(&listenfd);
	if(debug)
		printf("Parent: socket created with discriptor %d\n", listenfd);
	struct sockaddr_in servAddr;
	setServAddr(&servAddr, &listenfd);
	if(debug)
		printf("Parent: socket bound to port %ld\n", servAddr.sin_port);
	listen(listenfd, BACKLOG);
	if(debug)
		printf("Parent: listening with connection queue of %d\n", BACKLOG);
	// Loop until accepting a conneciton.
	struct sockaddr_in clientaddr;
	int controlfd;
	int pid;
	for(;;;) {
		waitpid(-1, NULL, WNOHANG);
		controlfd = accept(listenfd, (struct sockaddr_in *) &clientAddr, sizeof(struct sockaddr_in));
		if(debug)
			printf("Parent: accepted client with file descriptor of %d", controlfd);
		if((pid = fork()) < 0) {
			perror("fork error");
			exit(1);
		}
		if(pid) {
			close(controlfd);
			if(debug)
				printf("Parent: spawned child %d and awaiting another connection.\n", pid);
		}
		else {
			int cid = getpid();
			if(debug)
				printf("Child %d: started\n", cid);
			client(&cid, &clientAddr, &controlfd, debug);
		}

	}

	return 0;
}
