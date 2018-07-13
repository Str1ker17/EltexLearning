#ifdef _MSC_VER
#undef __cplusplus
#define __builtin_alloca(size) NULL
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../libncurses_util/linux_util.h"
// Normally, gcc(1) translates calls to alloca() with inlined code.
// This is not done when either the -ansi, -std=c89, -std=c99, or the
// -std=c11 option is given AND the header <alloca.h> is not included.
#include <alloca.h>

int main(int argc, char **argv) {
	if(argc < 2) {
		printf("[x] Specify FIFO (named pipe) to open\n");
		exit(EXIT_FAILURE);
	}
	struct stat *st = (struct stat*)alloca(sizeof(struct stat));
	__syscall(stat(argv[1], st));

	if((st->st_mode & __S_IFMT) != __S_IFIFO) {
		printf("[x] File '%s' is not a FIFO (named pipe)\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	int fd = open(argv[1], O_RDONLY);
	lassert(fd != -1);
	//lassert(dup2(STDOUT_FILENO, fd) == fd);

	setbuf(stdout, NULL);
	char *buf = (char*)alloca(BUFSIZ);
	while(true) {
		ssize_t red = read(fd, buf, BUFSIZ);
		if (red == -1) {
			logprint(ANSI_BACKGROUND_RED ANSI_COLOR_WHITE "[x] read() error" ANSI_COLOR_RESET "\n");
			break;
		}
		if(red == 0) {
			logprint(ANSI_BACKGROUND_YELLOW ANSI_COLOR_BLACK "[i] end-of-file" ANSI_COLOR_RESET "\n");
			break;
			//usleep(250000);
		}
		lassert(write(STDOUT_FILENO, buf, red) == red);
	}

	close(fd);
	return 0;
}
