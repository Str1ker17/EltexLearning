#ifdef _MSC_VER
#undef __cplusplus
#define __builtin_alloca(size) NULL // is real MSVC: _alloca(size)
#endif

#include <stdlib.h>
#include <unistd.h>
#include <alloca.h>
#include <fcntl.h> // open()
#include <sys/stat.h> // stat(), mkfifo()
#include "../libeditor/libeditor.h"
#include "../libncurses_util/linux_util.h"

int main(int argc, char **argv) {

#ifdef DEBUG
	char *name = "/tmp/be-fpipe";
	struct stat *st = alloca(sizeof(struct stat));

	if (stat(name, st) == 0) {
		// exists
		if((st->st_mode & __S_IFMT) != __S_IFIFO) {
			printf("[x] File '%s' exists and not a FIFO (named pipe)\n", name);
			exit(EXIT_FAILURE);
		}
	}
	else {
		__syscall(mkfifo(name, 0600));
	}

	int fd = open(name, O_WRONLY);
	lassert(dup2(fd, STDERR_FILENO) == STDERR_FILENO);
#else
	int fd = open("/dev/null", O_WRONLY);
	lassert(dup2(fd, STDERR_FILENO) == STDERR_FILENO);
#endif

	return editor_main(argc, argv);
}
