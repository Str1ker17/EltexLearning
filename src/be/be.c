// Note: this include is a beta feature for design- and compile-time
#include "../liblinux_util/mscfix.h"

#include <stdlib.h>
#include <unistd.h>
#include <alloca.h>
#include <fcntl.h> // open()
#include <sys/stat.h> // stat(), mkfifo()
#include "../libeditor/editor.h"
#include "../liblinux_util/linux_util.h"

int main(int argc, char **argv) {
	int e;
	// If the first character of optstring is '-', then each
	// nonoption argv-element is handled as if it were the 
	// argument of an option with character code 1
	while((e = getopt(argc, argv, "-d")) != -1) {
		VERBOSE logprint("Got argument: %c = %s\n", e, optarg);
		switch(e) {
			case 'd':
				verbose = true;
			break;

			case 1:
				// compatibility mode
			break;

			case '?':
			default:
				VERBOSE logprint("[!] Unknown command-line argument with value: %s\n", optarg);
			break;
		}
	}

	int fd;
	VERBOSE {
		char *name = "/tmp/be-fpipe.err";
		struct stat *st = alloca(sizeof(struct stat));

		if (stat(name, st) == 0) {
			// exists
			if(!S_ISFIFO(st->st_mode)) {
				printf("[x] File '%s' exists and not a FIFO (named pipe)\n", name);
				exit(EXIT_FAILURE);
			}
		}
		else {
			__syscall(mkfifo(name, 0600));
		}

		printf("[DEBUG MODE] Connect bterm or compatible reader to '%s'\n", name);
		__syscall(fd = open(name, O_WRONLY));
	}
	else {
		fd = open("/dev/null", O_WRONLY);
	}

	lassert(dup2(fd, STDERR_FILENO) == STDERR_FILENO);
	return editor_main(argc, argv);
}
