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
/* Main control for the client program, argument is a pointer
   of the control socket file descriptor and debug flag. */
void mainmenu(int *sfd, int debug) {
	char usercmd[BUF_SIZE];
	char *cmd, *path;
	char toServer[BUF_SIZE];
	char fromServer[BUF_SIZE];
	int num = 0;
	int n = 0;
	int i = 0;
	do {
		write(1, "> ", 2);
		fflush(stdout);
		fgets(usercmd, BUF_SIZE, stdin);
		cmd = strtok(usercmd, " ");
		if(debug) {
			printf("command entered: %s\n", cmd);
		}
		path = strtok(NULL, " ");
		if(debug) {
			printf("path entered %s\n", path);
		}
		if(strcmp(cmd, "cd") == 0) {
			// TODO: write code for cd locally
		}
		else if(strcmp(cmd, "rcd") == 0) {
			strcpy(toServer, "C\n");
			strcat(toServer, path);
			i = 0;
			while((num = write(*sfd, &toServer[i], 1)) > 0) {
				if(num < 0) {
					perror("E couldn't write to server");
					break;
				}
				i++;
			}
			// TODO: write code for cd on the server
		}
		else if(strcmp(cmd, "ls") == 0) {
			// TODO: write code for ls -l locally
		}
		else if(strcmp(cmd, "rls") == 0) {
			strcpy(toServer, "L\n");
			i = 0;
			while((num = write(*sfd, &toServer[i], 1)) > 0) {
				if(num < 0) {
					perror("E couldn't write to server");
					break;
				}
			}
			// TODO: finish ls -l on server.
		}
		else if(strcmp(cmd, "get") == 0) {
			strcpy(toServer, "G");
			strcat(toServer, path);
			i = 0;
			while((num = write(*sfd, &toServer[i], 1)) > 0) {
				if(num < 0) {
					perror("E couldn't write to server");
					break;
				}
				i++;
			}

		}
		else if(strcmp(cmd, "show") == 0) {
			strcpy(toServer, "S\n");
			strcat(toServer, path);
			i = 0;
			while((num = write(*sfd, &toServer[i], 1)) > 0) {
				if(num < 0) {
					perror("E couldn't write to server");
					break;
				}
			}
		}
		else if(strcmp(cmd, "put") == 0) {
			strcpy(toServer, "P\n");
			strcat(toServer, path);
			i = 0;
			while((num = write(*sfd, &toServer[i], 1)) > 0) {
				if(num < 0) {
					perror("E couldn't write to server");
					break;
				}
			}
		}
		else if(strcmp(cmd, "exit\n") == 0) {
			i = 0;
			strcpy(toServer, "Q\n");
			while((num = write(*sfd, &toServer[i], 1)) > 0) {
				if(num < 0) {
					perror("E couldn't write to server");
					break;
				}
				i++;
			}
			i = 0;
			n = 0;
			while((num = read(*sfd, &fromServer[i], 1)) > 0) {
				if(num < 0) {
					perror("E could not read from server");
					break;
				}
				n += num;
				i++;
			}
			if(fromServer[0] == 'E') {
				printf("%s", fromServer);
				strcpy(usercmd, "");
			}
			else {
				printf("%s", fromServer);
			}
		}
	} while(strcmp(usercmd, "exit\n") != 0);
}

int main(int argc, char *argv[]) {
	int socketfd = 0;
	int debug = 0;
	char *server = NULL;
	struct sockaddr_in servAddr;
	struct hostent *hostEntry = NULL;
	struct in_addr **pptr;
	int numRead = 0;
	char buffer[BUF_SIZE];
	// check to see if a hostname and debug was entered
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
	int i = 0;
	int num = 0;
	// Read awknoledgement message from the server
	while((num = read(socketfd, &buffer[i], 1)) > 0) {
		if(debug) {
			printf("in read\n");
		}
		if(num < 0) {
			perror("E");
			exit(1);
		}
		numRead += num;
		if(buffer[i] == '\n'){
			break;
		}
		i++;
	}
	if(debug) {
		printf("passed awknowledgement read\n");
	}
	// write awknoledgement message to stdout.
	if(write(1, buffer, numRead) < 0) {
		perror("E");
		exit(1);
	}
	// call main menu.
	if(debug) {
		printf("calling mainmenu\n");
	}
	mainmenu(&socketfd, debug);
	
	return 0;
}
