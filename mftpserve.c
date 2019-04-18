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
void toClient(int *cfd, int debug) {
	int nread = 0, n = 0;
	int fromClient[BUF_SIZE];
	int i = 0;
	char c;
	char path[BUF_SIZE];
	char response[BUF_SIZE];
	char *ret;
	int plen = 0;
	// Awknowledge connection between client and server.
	strcpy(response, "A: Connected\n");
	while((n = write(*cfd, &response[i], 1)) > 0) {
		if(debug) {
			printf("n = %d\n", n);
		}
		if(n < 0) {
			perror("E cannot write to client");
			exit(1);
		}
		if(response[i] == '\n')
			break;
		i++;
	}
	// Wait for client directions.
	do {
		nread = 0;
		i = 0;
		n = 0;
		while((n = read(*cfd, &fromClient[i], 1)) > 0) {
			if(debug) {
				printf("fromClient[%d] = %c\nn = %d\n", i, fromClient[i], n);
			}
			
			nread += n;
			if(fromClient[i] == '\n') {
				break;
			}
			i++;	
		}
		// split the command into two
		c = fromClient[0];  // c for command
		// get the length of the string starting from the first index
		plen = nread - 1;
		if(debug) {
			printf("plen = %d\n", plen);
		}
		// if we have more than just a command, build the path into
		// the server.
		if(plen > 1) {
			for(int j = 1; j < nread; j++) {
				path[j - 1] = fromClient[j];
			}
		}
		// add the null terminator from the server.
		path[plen] = '\0';
		if(debug) 
			printf("command received: %c\n", c);
		if(debug && plen > 0) {
			printf("path received: %s\n", path);
		}
		i = 0;
		switch (c) {
			case 'C':
			strcpy(response, "A: C awknowledged\n");
			while((n = write(*cfd, &response[i], 1)) > 0) {
				if(n < 0) {
					perror("E could not write to client");
				}
				i++;
			}
			break;
			case 'L':
			strcpy(response, "A: L awknowledged\n");
			while((n = write(*cfd, &response[i], 1)) > 0) {
				if(n < 0) {
					perror("E could not write to client");
				}
				i++;
			}
			break;
			case 'G':
			strcpy(response, "A: G awknowledged\n");
			while((n = write(*cfd, &response[i], 1)) > 0) {
				if(n < 0) {
					perror("E could not write to client");
				}
				i++;
			}
			break;
			case 'S':
			strcpy(response, "A: S awknowledged\n");
			while((n = write(*cfd, &response[i], 1)) > 0) {
				if(n < 0) {
					perror("E could not write to client");
				}
				i++;
			}
			case 'P':
			strcpy(response, "A: P awknowledged\n");
			write(*cfd, response, strlen(response));
			break;
			case 'Q':
			strcpy(response, "A: quitting session\n");
			while((n = write(*cfd, &response[i], 1)) > 0) {
				if(n < 0) {
					perror("E could not write to client");
				}
				i++;
			}
			break;
			default:
			if(write(*cfd, "E\n", strlen("E\n")) < 0) {
				perror("E could not write to client");
			}
			break;


		}
	} while(c != 'Q');
}
/* displays the hostname of the client (for the server) */
void toServer(struct sockaddr_in *cAddr, int *cfd, int debug) {
	struct hostent *hostEntry;
	char *hostName;
	if((hostEntry = gethostbyaddr(&cAddr->sin_addr, sizeof(struct in_addr),
			AF_INET)) == NULL) {
			herror("E hostname");
			close(*cfd);
			exit(1);
		}
	hostName = hostEntry->h_name;
	printf("%s connected.\n", hostName);
	waitpid(-1, NULL, 0);
	printf("%s connection closed.\n", hostName);
	if(close(*cfd) < 0) {
		perror("E close control desc. serverside");
	}
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
			fprintf(stderr, "E Argument error: only '-d' command allowed.\n");
			exit(1);
		}
		debug = 1;
	}


	struct sockaddr_in servAddr;
	struct sockaddr_in clientAddr;
	int clistenfd, dlistenfd, controlfd, datafd;
	socklen_t length = sizeof(struct sockaddr_in);

	createSocket(&clistenfd);  // create the listener socket
	if(debug)
		printf("created socket.\n");
	memset(&servAddr, 0, sizeof(servAddr));   

	setServAddr(&servAddr, &clistenfd);			// build socket to a port
	listen(clistenfd, BACKLOG);					// listen for a connection
	if(debug)
		printf("listening for connection.\n");
	while(1) {
		// accept the connection
		if((controlfd = accept(clistenfd, (struct sockaddr *) &clientAddr, &length)) < 0){
			perror("E");
		}

		if(!fork()) {
			// child (client) code
			if(debug)
				printf("In child\n");
			toClient(&controlfd, debug);
			if(close(controlfd) < 0) {
				perror("E close control child process");
				exit(1);
			}
			exit(0);
		}
		else {
			// server (parent) code
			if(debug)
				printf("In parent\n");
			toServer(&clientAddr, &controlfd, debug);
		}	
	}
	if(close(clistenfd) < 0) {
		perror("E close error");
		exit(1);
	}
	return 0;
}
