#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include "../libcopyfile/copyfile.h"
#include "../libncurses_util/linux_util.h"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

bool verbose = false;

int main(int argc, char **argv) {
	if(argc < 3) {
		logprint("[x] Specify source and destination file\n");
		logprint("Usage: %s source destination [-udscv]\n", argv[0]);
		return EXIT_FAILURE;
	}
	int mode = 0;
	int e;
	while((e = getopt(argc, argv, "-udscv")) != -1) {
		switch(e) {
			case 'u': mode |= COPYFILE_USE_UNLOCKED; break;
			case 'd': mode |= COPYFILE_USE_FD; break;
			case 's': mode = (mode & ~0xff) | COPYFILE_SENDFILE; break;
			case 'c': mode = (mode & ~0xff) | COPYFILE_READWRITE_CHUNKS; break;
			case 'v': verbose = true; break;

			case 1:
				// compatibility mode
			break;

			case '?':
			default:
				logprint("[!] Unknown command-line argument with value: %s\n", optarg);
			break;
		}
	}

	char *copyfile_mode = NULL;
	if(COPYFILE_MODE(mode) == COPYFILE_READWRITE_BYTES) 
		copyfile_mode = STR_EXPAND(COPYFILE_READWRITE_BYTES);
	else if(COPYFILE_MODE(mode) == COPYFILE_READWRITE_CHUNKS) 
		copyfile_mode = STR_EXPAND(COPYFILE_READWRITE_CHUNKS);
	else if(COPYFILE_MODE(mode) == COPYFILE_SENDFILE) 
		copyfile_mode = STR_EXPAND(COPYFILE_SENDFILE);

	VERBOSE {
		logprint("[i] mode = %s, flags: UNLOCKED = %d, USE_FD: %d\n"
			, copyfile_mode, (!!(mode & COPYFILE_USE_UNLOCKED)), (!!(mode & COPYFILE_USE_FD)));
		logprint("[i] Copy file '%s' to '%s'...\n", argv[1], argv[2]);
	}

	if (copyfile(argv[1], argv[2], mode) == 0) {
		VERBOSE logprint(ANSI_COLOR_GREEN "File copied!" ANSI_COLOR_RESET "\n");
	}
	else { // -1
		logprint(ANSI_COLOR_RED "copyfile() returned -1" ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
