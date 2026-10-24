#include "common.h"
#include <sys/types.h>

void die(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

void *ecalloc(size_t n, size_t s) {
	void *p = calloc(n, s);
	if (!p)
		die("calloc failed");
	return p;
}

void spawn(const char *cmd) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return;
	}
	if (pid == 0) {
		setsid();	
		pid = fork();
		if (pid < 0) {
			perror("fork");
			_exit(1);
		}
		if (pid == 0) {
			execlp(cmd, cmd, NULL);
			perror(cmd);
			_exit(1);
		}
		_exit(0);
	}
}
