#include <stdio.h>
#include <unistd.h>
#include <string.h>    // strcat
#include <stdarg.h>    // getopt 
#include <stdlib.h>    // exit
#include <sys/socket.h>  // socket
#include <netinet/in.h>  // htons
#include <errno.h>    // errno?
#include <strings.h>  // bzero
#include <sys/select.h>  // select

#define MAXLINE 4096

/* global vars */
int is_client = 1;
int udp = 0;
int listenq = 5;
int readlen = 1024;
char* rbuf = NULL;
int debug = 0;

static void usage(const char*);
static void err_msg(const char*, ...);
static void err_sys(const char*, ...);
static void err_doit(int, const char*, va_list);
static int cliopen(char*, char*);
static int servopen(char*, char*);
static void buffers(int);
static void loop(int);
static void sockopts(int, int);

int main(int argc, char* argv[])
{
	int c;
	while((c = getopt(argc, argv, "sD")) != EOF) {
		switch(c) {
		case 's':
			is_client = 0;
			break;
		case 'D':
			debug = 1;
			break;
		case 'u':
			udp = 1;
			break;
		case '?':
			usage("");
			break;
		}
	}

	char* host, *port;

	if(is_client) {
		if(optind != argc - 2) 
			usage("missing <host> <port> options");
		host = argv[optind];
		port = argv[optind + 1];
	} else {
		if(optind == argc - 2) {
			host = argv[optind];
			port = argv[optind + 1];
		} else if(optind == argc - 1) {
			port = argv[optind];
		} else {
			usage("missing <port> option");
		}
	}
	int sock_fd;
	if(is_client) {
		sock_fd = cliopen(host, port);
	} else {
		sock_fd = servopen(host, port);
	}

	loop(sock_fd);

	return 0;
}

/*  创建客户端
 */
static int
cliopen(char* host, char* port) {
	int fd;
	struct sockaddr_in serv_addr;
	unsigned long addr_t;

	// init serv_addr
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(port));

	if((addr_t = inet_addr(host)) != 0xFFFFFFFF) {
		bcopy((char*)&addr_t, (char*)&serv_addr.sin_addr, sizeof(addr_t));
	} else {
		err_sys("call inet_addr() error");
	}

	if((fd = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0) {
		err_sys("call socket() error");
	}

	buffers(fd);
	sockopts(fd, 0);

	// try to connect
	if(connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		err_sys("call connect() error");
	}

	return fd;
}

/*  创建服务端
 */
static int
servopen(char* host, char* port) {
	int fd;
	struct sockaddr_in serv_addr, cli_addr;
	unsigned long addr_t;
	int new_fd;
	int cli_size;

	// init serv_addr
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(port));
	if(host) {
		if((addr_t = inet_addr(host)) == 0xFFFFFFFF)
			err_sys("call inet_addr() error");
		serv_addr.sin_addr.s_addr = addr_t;
	} else {
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	if((fd = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0) {
		err_sys("call socket() error");
	}

	if(bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		err_sys("call bind() error");

	buffers(fd);
	sockopts(fd, 0);
	listen(fd, listenq);

	for(;;) {
		cli_size = sizeof(cli_addr);
		if((new_fd = accept(fd, (struct sockaddr *)&cli_addr, &cli_size)) < 0)
			err_sys("call accept() error");
		return new_fd;
	}
	return -1;
}

static void
buffers(int sock_fd) {
	if(rbuf == NULL)
		rbuf = (char*)malloc(readlen);
}

static void
sockopts(int sock_fd, int doall) {
	if(debug) {
		int option, optlen;
		option = 1;
		if(setsockopt(sock_fd, SOL_SOCKET, SO_DEBUG, (char*)&option, sizeof(option)) < 0)
			err_sys("call setsockopt() error");
		option = 0;
		optlen = sizeof(option);
		if(getsockopt(sock_fd, SOL_SOCKET, SO_DEBUG, (char*)&option, &optlen) < 0)
			err_sys("call getsockopt() error");
		if(option == 0)
			err_sys("SO_DEBUG not set (%d)", option);
	}
}

static void
loop(int sock_fd) {
	fd_set rset;
	int max_fd = sock_fd + 1;
	int stdineof = 0;
	int nread;

	FD_ZERO(&rset);

	for(;;) {
		if(stdineof == 0) 
			FD_SET(STDIN_FILENO, &rset);
		FD_SET(sock_fd, &rset);
		if(select(max_fd, &rset, NULL, NULL, NULL) < 0)
			err_sys("call select() failed");
		if(FD_ISSET(STDIN_FILENO, &rset)) {
			if((nread = read(STDIN_FILENO, rbuf, readlen)) < 0) {
				err_sys("call read() error");
			} else if(nread == 0) {
				stdineof = 1;
				fputs("bye.\n", stderr);
				break;
			} else {
				if(write(sock_fd, rbuf, nread) != nread)
					err_sys("call write() error");
			}
		}
		if(FD_ISSET(sock_fd, &rset)) {
			if((nread = read(sock_fd, rbuf, readlen)) < 0) {
				err_sys("call read() error");
			} else if(nread == 0) {
				fputs("client close.\nbye.\n", stderr);
				break;
			} else {
				if(write(STDOUT_FILENO, rbuf, nread) != nread) {
					err_sys("call write() error");
				}
			}
		}
	}

	if(close(sock_fd) < 0)
		err_sys("call close() error");
}

static void
usage(const char* msg) {

	err_msg(
"usage: sock [options] <host> <post>            (for client; default)\n"
"       sock [options] -s [<ipAddr>] <port>     (for server)\n"
"options: -s operate as server instead of client\n"
"         -D use SO_DEBUG\n"
"         -u use UDP instead of TCP\n"
);

	err_msg(msg);
	exit(1);
}

static void 
err_msg(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(0, fmt, ap);
	va_end(ap);
	return;
}

static void
err_sys(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, fmt, ap);
	va_end(ap);
	exit(1);
}

static void
err_doit(int errnoflag, const char* fmt, va_list ap) {
	char buf[MAXLINE];
	vsprintf(buf, fmt, ap);
	int error_save = -1; //errno;
	if(errnoflag) 
		sprintf(buf + strlen(buf), " : %s", strerror(error_save));
	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
}
