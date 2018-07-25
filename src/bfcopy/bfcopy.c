#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include "../libcopyfile/copyfile.h"
#include "../liblinux_util/linux_util.h"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

int main(int argc, char **argv) {
	if(argc < 3) {
		logprint("[x] Specify source and destination file\n");
		logprint("Usage: %s source destination [-v0123456]\n", argv[0]);
		logprint("\t-v\t- verbose output\n");
		logprint("Digits are for copy method selection:\n");
		logprint("\t-0\t- fd (default)\n");
		logprint("\t-1\t- sendfile\n");
		logprint("\t-2\t- stdio\n");
		logprint("\t-3\t- stdio unlocked\n");
		logprint("\t-4\t- stdio chunks\n");
		logprint("\t-5\t- stdio unlocked+chunks\n");
		logprint("\t-6\t- cp\n");
		return EXIT_FAILURE;
	}
	int mode = COPYFILE_FD;
	int e;
	while((e = getopt(argc, argv, "-v0123456")) != -1) {
		switch(e) {
			case 'v': verbose = true; break;

			case '0': mode = COPYFILE_FD; break;
			case '1': mode = COPYFILE_SENDFILE; break;
			case '2': mode = COPYFILE_STDIO; break;
			case '3': mode = COPYFILE_STDIO | COPYFILE_FLAG_STDIO_UNLOCKED; break;
			case '4': mode = COPYFILE_STDIO | COPYFILE_FLAG_STDIO_CHUNKS; break;
			case '5': 
				mode = COPYFILE_STDIO | COPYFILE_FLAG_STDIO_UNLOCKED | COPYFILE_FLAG_STDIO_CHUNKS; break;
			case '6': mode = COPYFILE_CP; break;

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
	if(COPYFILE_MODE(mode) == COPYFILE_STDIO) 
		copyfile_mode = STR_EXPAND(COPYFILE_STDIO);
	else if(COPYFILE_MODE(mode) == COPYFILE_FD) 
		copyfile_mode = STR_EXPAND(COPYFILE_FD);
	else if(COPYFILE_MODE(mode) == COPYFILE_SENDFILE) 
		copyfile_mode = STR_EXPAND(COPYFILE_SENDFILE);
	else if(COPYFILE_MODE(mode) == COPYFILE_CP) 
		copyfile_mode = STR_EXPAND(COPYFILE_CP);

	VERBOSE {
		logprint("[i] mode = %s, flags: UNLOCKED = %d, CHUNKS: %d\n"
			, copyfile_mode
			, (!!(mode & COPYFILE_FLAG_STDIO_UNLOCKED))
			, (!!(mode & COPYFILE_FLAG_STDIO_CHUNKS))
		);
		logprint("[i] Copy file '%s' to '%s'...\n", argv[1], argv[2]);
	}

	if (copyfile(argv[1], argv[2], mode, NULL) == 0) {
		VERBOSE logprint(ANSI_COLOR_GREEN "File copied!" ANSI_COLOR_RESET "\n");
	}
	else { // -1
		logprint(ANSI_COLOR_RED "copyfile() returned -1" ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
