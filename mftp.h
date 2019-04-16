#ifndef MFTP_H
#define MFTP_H
/* Specifies which port to listen for for the */
#define PORT_NUM 49999
/* Buffer size */
#define BUF_SIZE 4096

/* Create the socket to communitcate with the client/server
   Argument, the file descriptor for the socket. */
void createSocket(int *sfd) {
		if((*sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Server Error");
		exit(1);
	}
}


#endif