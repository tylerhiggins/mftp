CC = cc 
FLAGS = -Wall
OBJC = mftp.o
OBJS = mftpserve.o
CLIENT = mftp
SERVER = mftpserve
ARGS = localhost

all:
	${CC} ${FLAGS} -c mftpserve.c
	${CC} ${FLAGS} -c mftp.c
	${CC} ${FLAGS} -o ${SERVER} ${OBJS}
	${CC} ${FLAGS} -o ${CLIENT} ${OBJC}

server:
	${CC} ${FLAGS} -c mftpserve.c
	${CC} ${FLAGS} -o ${SERVER} ${OBJS}

client:
	${CC} ${FLAGS} -c mftp.c
	${CC} ${FLAGS} -o ${CLIENT} ${OBJC}

mftpserve.o: mftpserve.c
	${CC} ${FLAGS} -c mftpserve.c

mftp.o: mftp.c
	${CC} ${FLAGS} -c mftp.c

mftpserve.o: mftpserve.c mftp.h

mftp.o: mftp.c mftp.h

runserv:
	./${SERVER}

runclient:
	./${CLIENT} ${ARGS}

clean:
	rm -fr ${CLIENT} ${SERVER} ${OBJC} ${OBJS}
