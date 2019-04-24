/*
Tyler Higgins
CS 360
mftp.h
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

#define BUF_SIZE 4096
#define PORT_NUM 49999

int createSocket(int *fd) {
	if((*fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}
	return 0;
}
