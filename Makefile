
CC = gcc

PROG = sock

OBJS = main.o

all: ${PROG}

${PROG} : ${OBJS}
	${CC} -o $@ ${OBJS}

clean:
	rm ${PROG} ${OBJS}
