CC = cc
FLAG = -Wall
SERV = mftpserve
CLIENT = mftp
SERVO = mftpserve.o
CLIO = mftp.o

all:
	${CC} ${FLAG} -c mftpserve.c
	${CC} ${FLAG} -c mftp.c
	${CC} ${FLAG} -o ${SERV} ${SERVO}
	${CC} ${FLAG} -o ${CLIENT} ${CLIO}

client:
	${CC} ${FLAG} -c mftp.c
	${CC} ${FLAG} -o ${CLIENT} ${CLIO}

server:
	${CC} ${FLAG} -c mftpserve.c
	${CC} ${FLAG} -o ${SERVER} ${SERVEO}

clean:
	rm -f ${CLIENT} ${SERVER} ${CLIO} ${SERVO}
