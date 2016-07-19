#include <stdio.h>
#include <unistd.h>
#include <string.h>    // strcat
#include <stdarg.h>    // getopt 
#include <stdlib.h>    // exit
#include <sys/socket.h>  // socket
#include <netinet/in.h>  // htons
#include <errno.h>    // errno?
#include <strings.h>  // bzero

#define MAXLINE 4096

/* global vars */
int is_client = 1;
int udp = 0;
int listenq = 5;

static void usage(const char*);
static void err_msg(const char*, ...);
static void err_sys(const char*, ...);
static void err_doit(int, const char*, va_list);
static int cliopen(char*, char*);
static int servopen(char*, char*);

int main(int argc, char* argv[])
{
	int c;
	while((c = getopt(argc, argv, "s")) != EOF) {
		switch(c) {
		case 's':
			is_client = 0;
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

	listen(fd, listenq);

	for(;;) {
		if((new_fd = accept(fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr))) < 0)
			err_sys("call accept() error");
		return new_fd;
	}
	return -1;
}

static void
usage(const char* msg) {

	err_msg(
"usage: sock [options] <host> <post>            (for client; default)\n"
"       sock [options] -s [<ipAddr>] <port>     (for server)\n"
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
	int error_save = errno;
	if(errnoflag) 
		sprintf(buf + strlen(buf), " : %s", strerror(error_save));
	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
}
