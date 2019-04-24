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
#include <sys/stat.h>
#include <fcntl.h>
#include "mftp.h"

/* connectToServer takes in the hostname, pointer to the socket file descripter 
   pointer to the structure to hold the host This function sets up the
   client and attempts to connect to the server.*/
void connectToServer(char *hname, int *sfd, struct sockaddr_in *sAddr) {
	struct hostent *hostEntry;
	struct in_addr **pptr;
	memset(sAddr, 0, sizeof(*sAddr));
	sAddr->sin_family = AF_INET;
	sAddr->sin_port = htons(PORT_NUM);
	if((hostEntry = gethostbyname(hname)) == NULL) {
		herror("hostEntry");
		exit(1);
	}
	pptr = (struct in_addr **)hostEntry->h_addr_list;
	memcpy(&sAddr->sin_addr, *pptr, sizeof(struct in_addr));
	if(connect(*sfd, (struct sockaddr *) sAddr, sizeof(*sAddr)) < 0) {
		perror("connect");
		exit(1);
	}
}
/* Make data connection takes the hostname, pointer to the data file descriptor, the 
   data address, and the port as arguments. makeDataConnection sets up the data connection
   on the client and connects to the server. */
int makeDataConnection(char *hname, int *datafd, struct sockaddr_in *dAddr, char *port) {
	struct hostent *hostEntry;
	struct in_addr **pptr;
	memset(dAddr, 0, sizeof(*dAddr));
	dAddr->sin_family = AF_INET;
	dAddr->sin_port = htons(atoi(port));
	if((hostEntry = gethostbyname(hname)) == NULL) {
		herror("hostEntry");
		return -1;
	}
	pptr = (struct in_addr **)hostEntry->h_addr_list;
	memcpy(&dAddr->sin_addr, *pptr, sizeof(struct in_addr));
	if(connect(*datafd, (struct sockaddr *) dAddr, sizeof(*dAddr)) < 0) {
		perror("connect");
		return -1;
	}
	return 0;
}

/* Send specified message to the server arguments are the pointer to the
  control file descriptor and the command */
void sendToServer(int *ctrlfd, char *cmd) {
	for(int i = 0; i < BUF_SIZE; i++) {
		if(write(*ctrlfd, &cmd[i],1) < 0) {
			perror("Could not send message to server");
			break;
		}
		if(cmd[i] == '\n')
			break;
	}
}
/* execMore reads from the data connection and executes 'more -20'
   arguments is a pointer the data file descriptor and the debug 
   command.*/
void execMore(int *datafd, int debug) {
	int pid;
	int fd[2];
	pipe(fd);
	int rdr, wtr;
	rdr = fd[0]; wtr = fd[1];
	if((pid = fork()) < 0){
		perror("forking error");
		return;
	}
	if(pid) {
		close(rdr);
		close(wtr);
		waitpid(pid, NULL, 0);
	}
	else{
		close(wtr);
		close(rdr);
		// set the data file descriptor to read.
		dup2(*datafd, 0);
		close(*datafd);
		execlp("more", "more", "-20", (char *)0);
		perror("exec more from server");
		exit(1);
	}
}

/* receive response from server arguments is the pointer to the control file descriptor
  the buffer, and a debug flag */
void receiveResponse(int *ctrlfd, char *buf, int debug) {
	if(debug)
		printf("Awaiting server response.\n");
	int n;
	for(int i = 0;i < BUF_SIZE; i++) {
		if((n = read(*ctrlfd, &buf[i],1)) < 0){
			perror("Could not receive message");
			break;
		}
		if(buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}
	}
	if(debug){
		printf("received server response '%s'\n", buf);
	}
}

/* Execls takes in the current working directory as an argument and execs ls
  and more via forking. */
void execls(int debug) {
	int pid;   // child id 1 for more
	int pid2;  // child id 2 for ls -l
	int fd[2];  // file descriptors use to pipe
	int rdr, wtr;  // for better readability
	pipe(fd);     // pipe stdin, stdout.
	rdr = fd[0]; wtr = fd[1];  
	if((pid = fork()) < 0) {
		perror("Client parent fork");
		return;
	}
	
	if(pid) { // fork if-else for more.
		if(debug)
			printf("Client parent: process %d to run ls locally\n", pid);

		if((pid2 = fork()) < 0) {
			perror("could not fork other child process");
			exit(1);
		}
		if(pid2) { // fork if-else for ls
			close(rdr);
			close(wtr);
			waitpid(pid2, NULL, 0);
		}
		else {  // ls command block
			dup2(rdr, 0);
			close(wtr);
			close(rdr);
			execlp("more", "more", "-20", (char *)0);
			perror("could not exec more locally");
			exit(1);
		}
	}
	else { // more command block
		dup2(wtr, 1);
		close(rdr);
		close(wtr);
		execlp("ls", "ls", "-l", (char *)0);
		perror("could not exec ls locally");
		exit(1);
	}

}
/* changecwd looks at the directiory the user specifies to change to */
int changecwd(char* p, int debug) {
	for(int i = 0; i < BUF_SIZE; i++){
		if(p[i] == '\n'){
			p[i] = '\0';
			break;
		}
	}
	struct stat d;
	if(lstat(p, &d) < 0) {
		perror("lstat did not succeed");
		return -1;
	}
	if(!S_ISDIR(d.st_mode)){
		fprintf(stderr, "local cd: path is not a directory\n");
		return -1;
	}
	if(access(p, F_OK) != 0 || access(p, R_OK) != 0 ||
		access(p, X_OK) != 0){
		perror("local cd: error");
		return -1;
	}
	if(chdir(p) != 0){
		perror("local cd: unable to change directory");
		return -1;
	}
	return 0;

}
/* commandMenu takes a pointer to the control file descriptor, this is the
   main driver function for the client commands. */
void commandMenu(int *ctrlfd, char *host, int debug) {
	char buffer[BUF_SIZE];
	char response[BUF_SIZE];
	char *cmd, *path;
	char toServer[BUF_SIZE];
	int datafd;
	struct sockaddr_in dataAddr;
	do {  // go through all the below commands until "exit\n" is called.
		path = NULL;
		cmd = NULL;
		printf("%s@MFTP:~$ ", host);
		fflush(stdout);
		fgets(buffer, BUF_SIZE, stdin);
		// parse the users command.
		cmd = strtok(buffer, " ");
		if(debug)
			printf("cmd = %s\n", buffer);
		path = strtok(NULL, " ");
		if(path != NULL) {
			if(debug) {
				printf("pathname = %s", path);
				fflush(stdout);
			}
		}
		// local ls
		if (strcmp(buffer, "ls\n") == 0) {
			execls(debug);
		}
		// local cd
		else if (strcmp(buffer, "cd") == 0) {
			if(changecwd(path, debug) == 0)
				printf("Changed local working directory to %s\n", path);
		}
		// server cd
		else if (strcmp(buffer, "rcd") == 0) {
			sprintf(toServer, "C%s", path);
			// strcpy(toServer, "C");
			// strcat(toServer, path);
			sendToServer(ctrlfd, toServer);
			receiveResponse(ctrlfd, response, debug);
			if(response[0] != 'A') {
				for(int i = 1; i < BUF_SIZE; i++){
					write(1, &response[i], 1);
					if(response[i] == '\0')
						break;
				}
				write(1, "\n", 1);
			}
			// all other server commands go here since they require
			// a data connection before proceeding.
			else {

				printf("Changed server working directory to %s\n", path);
			}
		}
		// exit case
		else if(strcmp(buffer, "exit\n") == 0){
			sendToServer(ctrlfd, "Q\n");
			receiveResponse(ctrlfd, response, debug);
			if(response[0] != 'A') {
				strcpy(buffer, "");
				for(int i = 1; i < BUF_SIZE; i++){
					write(1, &response[i], 1);
					if(response[i] == '\0')
						break;
				}
				write(1, "\n", 1);
			}
		}
		// else if for all data connections
		else if(strcmp(cmd, "rls\n") == 0 || strcmp(cmd, "get") == 0 ||
				strcmp(cmd, "show") == 0 || strcmp(cmd, "put") == 0) {
			// parse the users command.
			char cmdc = cmd[0];
			if(debug){
				printf("cmdc %c\n", cmdc);
			}
			// send the client a data connection request
			sendToServer(ctrlfd, "D\n");
			receiveResponse(ctrlfd, response, debug);
			// if error, print out the message
			if(response[0] != 'A') {
				for(int i = 1; i < BUF_SIZE; i++) {
					write(1, &response[i], 1);
					if(response[i] == '\0')
						break;
				}
				write(1, "\n", 1);
			}
			// else everything succeeded and we received a port.
			else {
				if(debug)
					printf("getting port number\n");
				char port[10];
				// get the port separated from the message.
				for(int i = 1; i < BUF_SIZE; i++){
					port[i-1] = response[i];
					if(port[i-1] == '\n') {
						port[i - 1] = '\0';
						break;
					}
				}
				if(debug)
					printf("Port number %s\n", port);
				// create the socket, if unsuccessful then do nothing.
				if(createSocket(&datafd) < 0){}
				else {
					if(debug)
						printf("Created socket\n");
					// make the data connection, if unsuccessful do nothing.
					if(makeDataConnection(host, &datafd, &dataAddr, port) < 0){}
					else { // switch statement based on first character of command.
						if(debug)
							printf("%s - made data connection\n", cmd);
						switch(cmdc){
							case 'r':
							if(debug)
								printf("in 'rls' area\n");
							sendToServer(ctrlfd, "L\n");
							receiveResponse(ctrlfd, response, debug);
							if(response[0] == 'A') {
								execMore(&datafd, debug);
							}
							break;
							case 's':
							sendToServer(ctrlfd, "G\n");
							receiveResponse(ctrlfd, response, debug);
							if(response[0] == 'A') {
								receiveResponse(ctrlfd, response, debug);
								if(response[0] == 'A')
									execMore(&datafd, debug);
								else {
									printf("Error response from server: ");
									fflush(stdout);
									for(int i = 1; i < BUF_SIZE; i++) {
										write(1, &response[i], 1);
										if(response[i] == '\0')
											break;
									}
									write(1, "\n", 1);
								}
							}
							else {
								printf("Error response from server: ");
									fflush(stdout);
									for(int i = 1; i < BUF_SIZE; i++) {
										write(1, &response[i], 1);
										if(response[i] == '\0')
											break;
									}
									write(1, "\n", 1);
							}
							break;
							case 'g':
							break;
							case 'p':
							break;
							default:
							break;
						}
						// close the data connection.
						close(datafd);
					}
				}
			}			
		}
		// unknown command response.
		else {
			// this is to properly format the command as if it didn't have a newline character.
			for(int i = 0; i < BUF_SIZE; i++){
				if(buffer[i] == '\n') {
					buffer[i] = '\0';
					break;
				}
			}
			printf("Unknown command '%s' - ignored\n", cmd);
		}
	} while(strcmp(buffer, "exit\n") != 0);
}
/* Main function */
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
		if(argc >= 2  && strcmp(argv[1], "-d") == 0) {
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
	// connect to server
	connectToServer(host, &controlfd, &servAddr);
	printf("Connected to server %s\n", host);
	commandMenu(&controlfd, host, debug);
	close(controlfd);

	return 0;
}
