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
#include <sys/stat.h>
#include <fcntl.h>
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
	sAddr->sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(*lfd, (struct sockaddr *)sAddr, sizeof(*sAddr)) < 0){
		fprintf(stderr, "Parent: fatel error binding socket to port 49999 -> %s\n", strerror(errno));
		exit(1);
	}

}
/* Creating the data connection arguments, a pointer to the data address the pointer to
   the data connection listner and a character string to hold an error message should an
   error occur. */
void setDataAddr(struct sockaddr_in *dAddr, int *dlfd, char m[]){
	memset(dAddr, 0, sizeof(*dAddr));
	dAddr->sin_family = AF_INET;
	dAddr->sin_port = 0;
	dAddr->sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(*dlfd, (struct sockaddr *)dAddr, sizeof(*dAddr)) < 0){
		sprintf(m, "E%s\n", strerror(errno));
		// strcpy(m, "E");
		// strcat(m, strerror(errno));
		// strcat(m, "\n");
		return;
	}
}
/* getHost takes the clients address information, a pointer to
   a character string, and the child pid as arguments.  This
   function attempts to retrieve a string representation of
   the client that is connected to the server. */
char *getHost(struct sockaddr_in *cAddr, int cid) {
	struct hostent *hostEntry;
	if((hostEntry = gethostbyaddr(&cAddr->sin_addr, sizeof(struct in_addr),
			AF_INET)) == NULL) {
		printf("Child %d: Translation of client hostname failed -> %s\n", cid, 
			strerror(errno));
		return NULL;
	}
	return hostEntry->h_name;

}
/* listDirectory takes a pointer to a data file descriptor, a character array
   to store the message to the client, and a debug flag.  This function
   pipe, forks and calls ls -l on the current server working directory.  It then
   transfers through the data file directory to be read by the client.*/
void listDirectory(int *datafd, char m[], int debug){
	int pid;
	int fd[2];
	pipe(fd);
	int rdr, wtr;
	rdr = fd[0]; wtr = fd[1];
	if((pid = fork()) < 0){
		perror("forking error");
		return;
	}
	if(pid) { // parent
		// don't need to read or write anything
		close(rdr);
		close(wtr);
		waitpid(pid, NULL, 0);
	}
	else{ // child
		close(wtr);
		close(rdr);
		// set up the data descriptor to write.
		dup2(*datafd, 1);
		close(*datafd);
		execlp("ls", "ls", "-l", (char *)0);
		sprintf(m, "E%s\n", strerror(errno));
		// strcpy(m, "E");
		// strcat(m, strerror(errno));
		// strcat(m, "\n");
		exit(1);
	}
	strcpy(m, "A");
	strcat(m, "\n");
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
		if(buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}
	}

}



/* writeCommand Writes to the client using the control file descriptor 
arguments is the pointer to the control file descriptor, the awknowledgment 
or error message, the child id, and the debug flag */
void writeCommand(int *ctrlfd, char *message, int cid, int debug) {
	int i = 0;
	if(debug && message[i] == 'A')
		printf("Child %d: sending positive awknowledgment\n", cid);
	else if(debug && message[i] == 'E')
		printf("Child %d: sending negative 'E' awknowledgement\n", cid);
	for(i = 0; i < BUF_SIZE; i++) {
		if(write(*ctrlfd, &message[i], 1) < 0) {
			fprintf(stderr, "Child %d: could not send message -> %s\n", cid, strerror(errno));
			break;
		}
		if(message[i] == '\n')
			break;
	}

}
/* putfile attempts to read from the data fd and write it to a file. */
void putFile(int *datafd, char path[], int *cfd, int cid, int debug) {
	char fullpath[BUF_SIZE];
	getcwd(fullpath, BUF_SIZE);
	strcat(fullpath, "/");
	strcat(fullpath, path);
	char m[BUF_SIZE];
	int fd;
	// check to see if file exists already.
	if(access(fullpath, F_OK) == 0) {
		sprintf(m,"EFile %s already exists\n", path);
		writeCommand(cfd, m, cid, debug);
		return;
	}
	if((fd = open(path, O_RDWR | O_CREAT, S_IRWXU)) < 0) {
		sprintf(m, "ECould not create file: %s\n", strerror(errno));
		writeCommand(cfd, m, cid, debug);
		return;
	}
	writeCommand(cfd, "A\n", cid, debug);
	int n;
	int SIZE = 512;
	char buffer[SIZE];
	while((n=read(*datafd, buffer, SIZE)) > 0) {
		write(fd, buffer, n);
	}
	close(fd);
	printf("Successfully transferred %s from client to %s\n", path, fullpath);

}
/* retrieveFile opens a file and transfers the file to the client using the data file descriptor,
   arguments are the pointer to the data file descriptor, the path to the file, and
   debug flag*/
int retrieveFile(int *datafd, char path[], int *cfd, int cid, int debug) {
	char fullpath[BUF_SIZE];
	getcwd(fullpath, BUF_SIZE);
	strcat(fullpath, "/");
	strcat(fullpath, path);
	char m[BUF_SIZE];

	if(debug)
		printf("fullpath is %s\n", fullpath);
	int fd;
	int SIZE = 512;
	char buffer[SIZE];
	if(access(fullpath, R_OK) != 0) {
		sprintf(m, "E%s\n", strerror(errno));
		writeCommand(cfd, m, cid, debug);
		return -1;
	}
	if(debug){
		printf("passed access\n");
	}
	if((fd = open(fullpath, O_RDONLY)) < 0) {
		sprintf(m, "E%s\n", strerror(errno));
		writeCommand(cfd, m, cid, debug);
		return -1;
	}
	if(debug)
		printf("opened file\n");
	int n;
	while((n=read(fd, buffer, SIZE)) > 0){
		write(*datafd, buffer, n);
	}
	if(n < 0){
		sprintf(m, "E%s\n", strerror(errno));
		writeCommand(cfd, m, cid, debug);
		return -1;
	}

	close(fd);
	writeCommand(cfd, "A\n", cid, debug);
	if(debug){
		printf("finished writing data\n");
	}
	return 0;

}

/* changecwd evaluates whether or not the directory can be read
   and executed by the user then changes the directory, arguemnts
   are the pointer to the pathname string, buffer to store the message
   that will be sent to the client, and debug flag */
void changecwd(char* p, char buf[], int debug) {
	char change[BUF_SIZE];
	getcwd(change, BUF_SIZE);
	strcat(change, "/");
	strcat(change, p);
	struct stat d;
	if(lstat(change, &d) < 0) {
		sprintf(buf, "E%s\n", strerror(errno));
		// strcpy(buf, "E");
		// strcat(buf, strerror(errno));
		// strcat(buf, "\n");
		return;
	}
	if(debug)
		printf("passed lstat\n");
	if(!S_ISDIR(d.st_mode)){
		sprintf(buf, "ENo such directory\n");
		return;
	}
	if(debug)
		printf("directory confirmed\n");
	if(access(p, F_OK) != 0 || access(p, R_OK) != 0 ||
		access(p, X_OK) != 0){
		sprintf(buf, "E%s\n", strerror(errno));
		return;
	}
	if(debug)
		printf("access is checked\n");
	if(chdir(change) != 0){
		sprintf(buf, "E%s\n", strerror(errno));
		return;
	}
	if(debug)
		printf("successfully changed the current working directory\n");
	strcpy(buf, "A\n");
	if(debug)
		printf("path change successful sending %s\n", buf);

}

/* The main driver function for the client, it takes in the child id, the address
   of the client, the the control file descriptor, and debug flag as arguments*/
void client(int cid, struct sockaddr_in cAddr, int ctrlfd, int debug) {
	char *hostname = NULL;
	char buffer[BUF_SIZE], cmd, path[BUF_SIZE];
	char sendMessage[BUF_SIZE];
	struct sockaddr_in dataAddr, clientDataAddr;
	int plen;
	int datalistenfd, datafd;
	if((hostname = getHost(&cAddr, cid)) == NULL) {
		hostname = inet_ntoa(cAddr.sin_addr);
		printf("Child %d: Client IP address %s\n", cid, inet_ntoa(cAddr.sin_addr));
	}
	printf("Child %d: Conenction accepted from host %s\n", cid, hostname);
	do{  // go through this procedure
		plen = 0;
		// receive command from the client
		receieveCommand(&ctrlfd, buffer, cid);
		if(debug)
			printf("command received from client: %s\n", buffer);
		cmd = buffer[0];
		for(int i = 1; i < BUF_SIZE; i++) {
			path[i-1] = buffer[i];
			plen++;
			if(path[i-1] == '\n') {
				path[i-1] = '\0';
				break;
			}
		}
		// switch based on the first character in the client messsage.
		switch(cmd){
			case 'C': // 'cd' from client
			changecwd(path, sendMessage, debug);
			if(debug)
				printf("message to send to client = %s\n", sendMessage);
			writeCommand(&ctrlfd, sendMessage, cid, debug);
			break;
			case 'D': // start data connection
			// create the listener socket for the data connection
			if(createSocket(&datalistenfd) < 0) {
				sprintf(sendMessage, "E%s\n", strerror(errno));
				writeCommand(&ctrlfd, sendMessage, cid, debug);
				break;
			}
			// set up the address of the data connection
			setDataAddr(&dataAddr, &datalistenfd, sendMessage);
			socklen_t l = sizeof(clientDataAddr);
			memset(&clientDataAddr, 0, sizeof(clientDataAddr));
			if(getsockname(datalistenfd, (struct sockaddr *) &clientDataAddr, &l) < 0) {
				sprintf(sendMessage, "E%s\n", strerror(errno));
				writeCommand(&ctrlfd, sendMessage, cid, debug);
				break;
			}
			// prepare to send the port to the client.
			int port = ntohs(clientDataAddr.sin_port);
			if(debug){
				printf("Child %d: port number to send to client --> %d\n", cid, port);
			}
			// send positive awknowledgement with the port number to the client
			sprintf(sendMessage, "A%d\n", port);
			writeCommand(&ctrlfd, sendMessage, cid, debug);
			// listen for the data connection.
			listen(datalistenfd, 1);
			if(debug)
				printf("Listening for the client to connect\n");
			// accept the connection
			socklen_t daddrl = sizeof(dataAddr);
			datafd = accept(datalistenfd, (struct sockaddr *) &clientDataAddr, &daddrl);
			break;
			case 'L': // 'rls' command from client
			writeCommand(&ctrlfd, sendMessage, cid, debug);
			listDirectory(&datafd, sendMessage, debug);	
			close(datafd);
			close(datalistenfd);
			break;
			case 'G': // 'show/get' command from client
			retrieveFile(&datafd, path, &ctrlfd, cid, debug);
			close(datafd);
			close(datalistenfd);
			break;
			case 'P':// 'put' command from client
			putFile(&datafd, path, &ctrlfd, cid, debug);
			close(datafd);
			close(datalistenfd);
			break;
			case 'Q': // 'exit' command from client
			if(debug)
				printf("Child %d: Quitting\n", cid);
			writeCommand(&ctrlfd, "A\n", cid, debug);
			printf("Child %d\n has exited\n", cid);
			break;
			default:
			writeCommand(&ctrlfd, "EUnable to process request\n", cid, debug);
			break;
		}
	}while(cmd != 'Q'); // if 'Q' is received, end this do-while loop.
	if(debug)
		printf("Child %d: exiting normally\n", cid);
}
/* Main function */
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
	if(createSocket(&listenfd) < 0) {
		exit(1);
	}
	if(debug)
		printf("Parent: socket created with descriptor %d\n", listenfd);
	struct sockaddr_in servAddr;
	setServAddr(&servAddr, &listenfd);
	if(debug)
		printf("Parent: socket bound to port %d\n", ntohs(servAddr.sin_port));
	listen(listenfd, BACKLOG);
	if(debug)
		printf("Parent: listening with connection queue of %d\n", BACKLOG);
	// Loop until accepting a conneciton.
	struct sockaddr_in clientAddr;
	int controlfd;
	int pid;
	socklen_t l = sizeof(struct sockaddr_in);
	for(;;) {
		// looks for zombie processes
		waitpid(-1, NULL, WNOHANG);
		// accept the connection
		controlfd = accept(listenfd, (struct sockaddr *) &clientAddr, &l);
		if(debug)
			printf("Parent: accepted client with file descriptor of %d\n", controlfd);
		// fork and split the server and child nodes
		if((pid = fork()) < 0) {
			perror("fork error");
			exit(1);
		}
		if(pid) { // Parent process
			// close the controlfd so it can be properly opened by another process
			close(controlfd);
			if(debug)
				printf("Parent: spawned child %d and awaiting another connection.\n", pid);
		}
		else {  // Child process
			int cid = getpid();
			if(debug)
				printf("Child %d: started\n", cid);
			// make a call to the driver function of the client
			client(cid, clientAddr, controlfd, debug);
			close(controlfd);
		}
		// Parent continues to loop, awaiting another connection.

	}
	close(listenfd);
	return 0;
}
