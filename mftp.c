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
#include <sys/stat.h>
#include "mftp.h"


/* Attempts to make a connection to the server specified in argv[1] in main
   Arguments - hostname of the server, the server sockaddr, the socket file descriptor,
   the server hostentry, pointer to the address of the incoming server. */
void connectToServer(char *hname, struct sockaddr_in *sAddr, int *sfd, 
					 struct hostent *hEntry, struct in_addr ***pptr) {
	sAddr->sin_family = AF_INET;
	sAddr->sin_port = htons(PORT_NUM);
	if((hEntry = gethostbyname(hname)) == NULL) {
		herror("hostEntry error ");
		exit(1);
	}
	*pptr = (struct in_addr **) hEntry->h_addr_list;
	memcpy(&sAddr->sin_addr, **pptr, sizeof(struct in_addr));
	if(connect(*sfd, (struct sockaddr *) sAddr, sizeof(*sAddr)) < 0) {
		perror("connect");
		exit(1);

	}
	
}

void promptUser

int main(int argc, char *argv[]) {
	int socketfd = 0;
	int datafd = 0;
	int debug = 0;
	char *server = NULL;
	struct sockaddr_in servAddr;
	struct hostent *hostEntry = NULL;
	struct in_addr **pptr;
	int numRead;
	char buffer[BUF_SIZE];
	// check to see if a hostname was entered
	if(argc < 2) {
		fprintf(stderr, "%s: No server specified.\n", argv[0]);
		exit(1);
	}
	if(argc > 3) {
		errno = E2BIG;
		perror("Argument error");
		exit(1);
	}
	if(strcmp(argv[1], "-d") == 0) {
		debug = 1;
		server = argv[2];
	}
	else {
		server = argv[1];
	}
	createSocket(&socketfd);  // attemt to create socket
	if(debug)
		printf("Socket created\n");

	memset(&servAddr, 0, sizeof(servAddr));
	// create the control connection to the server
	connectToServer(server, &servAddr, &socketfd, hostEntry, &pptr);
	if(debug)
		printf("Connected to server\n");

	
	return 0;
}
