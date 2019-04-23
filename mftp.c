/* 
Tyler Higgins
CS 360
mftp.c
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

// TODO: create socket to connect to server
// TODO: connect to server
// TODO: create prompt for user
// TODO: read/write from client to server
// TODO: create local commands
/* connectToServer takes in the hostname, pointer to the socket file descripter 
   pointer to the structure to hold the host This function sets up the
   client and attempts to connect to the server.*/
void connectToServer(char *hnmae, int *sfd, struct sockaddr_in *sAddr) {
	struct hostent *hostEntry;
	struct in_addr **pptr;
	memset(sAddr, 0, sizeof(*sAddr));
	sAddr->sin_family = AF_INET;
	sAddr->sin_port = htons(PORT_NUM);
	if((hEntry = gethostbyname(hname)) == NULL) {
		herror("hostEntry");
		exit(1);
	}
	pptr = (struct in_addr **)hEntry->h_addr_list;
	memcpy(&sAddr->sin_addr, *pptr, sizeof(struct in_addr));
	if(connect(*sfd, (struct sockaddr *) sAddr, sizeof(*sAddr)) < 0) {
		perror("connect");
		exit(1);
	}
}

/* commandMenu takes a pointer to the control file descriptor, this is the
   main driver function for the client commands. */
void commandMenu(int *ctrlfd) {
	char buffer[BUF_SIZE];
	char *cmd, *path, *cwd;
	char serverCmd[BUF_SIZE];
	getcwd(cwd, BUF_SIZE);
	printf("MFTP:~$ ");
	fflush(stdout);
	fgets(buffer, BUF_SIZE, stdin);
	// parse the users command.
	cmd = strtok(buffer, " ");
	path = strtok(NULL, " ");
	do {
		
	} while(strcmp(buffer, "exit\n") != 0);
}

int main(int argc, char *argv[]) {
	int debug = 0;
	char *host = NULL;
	// Check for arguments.
	if(argc >= 2) {
		// If there are more than two arguments
		if(argc > 3 ) {
			fprintf(stderr, "usage: %s [-d] <hostname | IP address>\n", argv[0]);
			exit(1);
		}
		// If there are at least one argument and that argument is '-d'.
		if(argc == 2 && strcmp(argv[1], "-d") == 0) {
			debug = 1;
			// No other arguments.
			if(argc < 3) {
				fprintf(stderr, "usage: %s [-d] <hostname | IP address>\n", argv[0]);
				exit(1);
			}
			else {
				host = argv[2];
			}
		}
		else {
			host = argv[1];
		}
	}
	// less than 1 argument.
	else {
		fprintf(stderr, "usage: %s [-d] <hostname | IP address>\n", argv[0]);
		exit(1);
	}
	if(debug)
		printf("Debug mode enabled\n");
	int controlfd;
	createSocket(&controlfd);
	if(debug)
		printf("Created socket with descriptor %d\n", controlfd);
	struct sockaddr_in servAddr;
	connectToServer(host, &controlfd, &servAddr);
	printf("Connected to host %s\n", host);
	commandMenu(&servAddr);

	return 0;
}
