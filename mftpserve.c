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
	if(ac > 2)
		return -1;
	if(strcmp(av[1], "-d") != 0)
		return -1;
	return 1;

}
/* Creating server address, arguments: server address and the
   listener file discriptor */
void setServAddr(struct sockaddr_in *sAddr, int *lfd) {
	memset(sAddr, 0, sizeof(*sAddr));
	sAddr->sin_family = AF_INET;
	sAddr->sin_port = htons(PORT_NUM);
	printf("Port number = %d\n", PORT_NUM);
	sAddr->sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(*lfd, (struct sockaddr *)sAddr, sizeof(*sAddr)) < 0){
		fprintf(stderr, "Parent: fatel error binding socket to port 49999 -> %s", strerror(errno));
		exit(1);
	}

}
/* getHost takes the clients address information, a pointer to
   a character string, and the child pid as arguments.  This
   function attempts to retrieve a string representation of
   the client that is connected to the server. */
int getHost(struct sockaddr_in *cAddr, char **n, int cid) {
	struct hostent *hostEntry;
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
/* receieve command reads from the client and places it in the buffer
   arguments is a pointer to the control file descriptor, buffer to store
   the command and the child id. */
void receieveCommand(int *ctrlfd, char buf[], int cid) {
	int n;
	int nread = 0;
	for(int i = 0; i < BUF_SIZE; i++) {
		if((n = read(*ctrlfd, &buf[i], 1)) < 0){
			fprintf(stderr, "Child %d: could not retrieve message -> %s\n", cid, strerror(errno));
			break;
		}
		nread += n;
		if(buf[i] == '\0')
			break;
	}

}
/* Write to the client, arguments is the pointer to the control file descriptor, 
   the awknowledgment or error message, the child id, and the debug flag */
void writeCommand(int *ctrlfd, char *message, int cid, int debug) {
	int i = 0;
	if(debug)
		printf("Child %d: sending positive awknowledgment\n", cid);
	for(i = 0; i < BUF_SIZE; i++) {
		if(write(*ctrlfd, &message[i], 1) < 0) {
			fprintf(stderr, "Child %d: could not send message -> %s\n", cid, strerror(errno));
			break;
		}
		if(message[i] == '\0')
			break;
	}

}
/* The main driver function for the client, it takes in the child id, the address
   of the client, the the control file descriptor, and debug flag as arguments*/
void client(int cid, struct sockaddr_in cAddr, int ctrlfd, int debug) {
	char *hostname;
	char buffer[BUF_SIZE], cmd, path[BUF_SIZE];
	int plen;
	if(getHost(&cAddr, &hostname, cid) != 0) {
		hostname = inet_ntoa(cAddr.sin_addr);
		printf("Child %d: Client IP address %s\n", cid, inet_ntoa(cAddr.sin_addr));
	}
	printf("Child %d: Conenction accepted from host %s\n", cid, hostname);
	do{ 
		plen = 0;
		receieveCommand(&ctrlfd, buffer, cid);
		cmd = buffer[0];
		for(int i = 1; i < BUF_SIZE; i++) {
			path[i-1] = buffer[i];
			plen++;
			if(path[i-1] == '\0')
				break;
		}
		switch(cmd){
			case 'Q':
			if(debug)
				printf("Child %d: Quitting\n", cid);
			writeCommand(&ctrlfd, "A\0", cid, debug);
			break;
			default:
			break;
		}
	}while(cmd != 'Q');
	if(debug)
		printf("Child %d: exiting normally\n", cid);
}
// TODO: Create Listener socket
// TODO: Create reader/writer functions
// TODO: fork client and server processes
// TODO: accept connections
// TODO: create data connection
int main(int argc, char *argv[]) {
	int debug = 0;
	if(argc > 1) {
		if((debug = setDebug(argc, argv)) < 0) {
			fprintf(stderr, "usage: %s [-d]\n", argv[0]);
			exit(1);
		}
	}
	if(debug)
		printf("Parent: debug mode enabled\n");
	int listenfd;
	createSocket(&listenfd);
	if(debug)
		printf("Parent: socket created with descriptor %d\n", listenfd);
	struct sockaddr_in servAddr;
	setServAddr(&servAddr, &listenfd);
	if(debug)
		printf("Parent: socket bound to port 49999\n");
	listen(listenfd, BACKLOG);
	if(debug)
		printf("Parent: listening with connection queue of %d\n", BACKLOG);
	// Loop until accepting a conneciton.
	struct sockaddr_in clientAddr;
	int controlfd;
	int pid;
	socklen_t l = sizeof(struct sockaddr_in);
	for(;;) {
		waitpid(-1, NULL, WNOHANG);
		controlfd = accept(listenfd, (struct sockaddr *) &clientAddr, &l);
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
			client(cid, clientAddr, controlfd, debug);
		}

	}

	return 0;
}
