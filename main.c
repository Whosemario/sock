#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define MAXLINE 4096

int is_client = 1;

static void usage(const char*);
static void err_msg(const char*, ...);
static void err_doit(const char*, va_list);

int main(int argc, char* const argv[])
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
	return 0;
}

static void
usage(const char* msg) {

	err_msg(
"usage: sock [options] <host> <post>            (for client; default)\n"
"       sock [options] -s [<ipAddr>] <port>     (for server)\n");

	err_msg(msg);
	exit(1);
}

static void 
err_msg(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(fmt, ap);
	va_end(ap);
	return;
}

static void
err_doit(const char* fmt, va_list ap) {
	char buf[MAXLINE];
	vsprintf(buf, fmt, ap);
	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
}
