CFLAGS = -Wall -Werror -Wextra -pedantic

all:
	${CC} ${CFLAGS} stopthread.c -o stopthread
