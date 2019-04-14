/*
Tyler Higgins
CS 360
Final Project - Client
mftp.c
*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "mftp.h"

#define BACKLOG 4


/* Arguments, serverAddr sockaddr and the listener socket file descriptor,
   creates the port socket. */
void setServAddr(struct sockaddr_in *sAddr, int *lfd) {
	sAddr->sin_family = AF_INET;
	sAddr->sin_port = htons(PORT_NUM);
	sAddr->sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(*lfd, (struct sockaddr *) sAddr, sizeof(*sAddr)) < 0) {
		perror("bind");
		exit(1);
	}
}
/* Argument - connection file descriptor
   Displays sucessful connection to the server. */
void toClient(int *cfd, struct sockaddr_in *sAddr) {
	char buffer[BUF_SIZE];
	strcpy(buffer, "Connection established\n");
	if(write(*cfd, buffer, strlen(buffer)) < 0) {
		perror("could not write to client");
		if(close(*cfd) < 0) {
			perror("close error");
			exit(1);
		}
	}
}
/* displays the hostname of the client (for the server) */
void toServer(struct sockaddr_in *cAddr, int *cfd) {
	struct hostent *hostEntry;
	char *hostName;
	if((hostEntry = gethostbyaddr(&cAddr->sin_addr, sizeof(struct in_addr),
			AF_INET)) == NULL) {
			herror("hostname");
			close(*cfd);
			exit(1);
		}
	hostName = hostEntry->h_name;
	printf("%s connected.\n", hostName);
	waitpid(-1, NULL, 0);
	close(*cfd);
}


/* Main function */
int main(int argc, char *argv[]) {
	int debug = 0;
	if(argc > 3) {
		errno = E2BIG;
		perror("Argument error");
		exit(1);
	}
	if(argc == 2) {
		if(strcmp(argv[1], "-d") != 0) {
			fprintf(stderr, "Argument error: only '-d' command allowed.\n");
			exit(1);
		}
		debug = 1;
	}


	struct sockaddr_in servAddr;
	struct sockaddr_in clientAddr;
	int listenfd, connectfd;
	socklen_t length = sizeof(struct sockaddr_in);


	createSocket(&listenfd);  // create the listener socket
	if(debug)
		printf("created socket.\n");
	memset(&servAddr, 0, sizeof(servAddr));   

	setServAddr(&servAddr, &listenfd);			// build socket to a port
	listen(listenfd, BACKLOG);					// listen for a connection
	if(debug)
		printf("listening for connection.\n");
	while(1) {
		// accept the connection
		connectfd = accept(listenfd, (struct sockaddr *) &clientAddr, &length);		
		if(!fork()) {
			// child (client) code
			if(debug)
				printf("In child\n");
			toClient(&connectfd, &servAddr);
		}
		else {
			// server (parent) code
			if(debug)
				printf("In parent\n");
			toServer(&clientAddr, &connectfd);
		}	
	}
	if(close(listenfd) < 0) {
		perror("close error");
		exit(1);
	}
	return 0;
}
