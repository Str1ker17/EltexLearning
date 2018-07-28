// Note: this include is a beta feature for design- and compile-time
#include "../liblinux_util/mscfix.h"

// include first because of const definition
#include "copyfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../liblinux_util/linux_util.h"

// global variable
const size_t copyfile_chunk = 262144; // 256K
const size_t sendfile_chunk = 10485760; // 10M

int copyfile(const char *src, const char *dst, int mode, size_t *progress_out) {
	struct stat st;
	if(lstat(src, &st) == -1) // can we copy links as links?
		return -1;

	int fd_src = -1, fd_dst = -1;
	FILE *f_src = NULL, *f_dst = NULL;

	char *buf = NULL;

	ssize_t red, written, progress = 0;
	size_t(*fread_func)(void*, size_t, size_t, FILE*);
	size_t(*fwrite_func)(const void*, size_t, size_t, FILE*);
	int(*feof_func)(FILE*);
	size_t chunk_size = 1, chunk_count = copyfile_chunk;

	int mode_i = COPYFILE_MODE(mode);
	int ret = 0;

	bool use_fd = ((mode_i == COPYFILE_FD) || (mode_i == COPYFILE_SENDFILE));
	bool copyfile_internal = (mode_i & 0x80);

	if(use_fd) {
		// st.st_mode & 07777 заимствует атрибуты файла, по идее
		// послушаем Илюху и сделаем как он говорит
		__syscall(fd_src = open(src, O_RDONLY));
		__syscall(fd_dst = open(dst, O_WRONLY | O_CREAT, st.st_mode));
		__syscall(posix_fallocate(fd_dst, 0, st.st_size));
	}

	switch(mode_i) {
		case COPYFILE_SENDFILE: {
			while(true) {
				written = sendfile(fd_dst, fd_src, NULL, st.st_size - progress);
				if(written == -1) { // sendfile error
					ret = -1;
					goto loop_break;
				}
				if(written == 0) { // eof
					if(progress != st.st_size) { // file not copied successfully
						ret = -1;
					}
					break;
				}
				progress += written;
				// push current progress
				if(progress_out != NULL) *progress_out = progress;
			}
		} break;

		case COPYFILE_FD: {
			buf = (char*)malloc(copyfile_chunk);
			while(true) {
				red = read(fd_src, buf, copyfile_chunk);
				if(red == -1) {
					ret = -1; // read error
					goto loop_break;
				}
				if(red == 0) {
					break; // eof
				}

				// read()/write() do not support chunks, only bytes
				written = write(fd_dst, buf, red);
				if(written != red) {
					ret = -1;
					goto loop_break;
				}
				progress += written;
				// push current progress
				if(progress_out != NULL) *progress_out = progress;
			}
		} break;

		case COPYFILE_STDIO: {
			buf = (char*)malloc(copyfile_chunk);
			// use fopen/fread/fwrite
			lassert((f_src = fopen(src, "rb")) != NULL);
			lassert((f_dst = fopen(dst, "wb")) != NULL);

			if(mode & COPYFILE_FLAG_STDIO_CHUNKS) {
				chunk_size = copyfile_chunk;
				chunk_count = 1;
			}

			if (mode & COPYFILE_FLAG_STDIO_UNLOCKED) {
				flockfile(f_src);
				flockfile(f_dst);
				fread_func = fread_unlocked;
				fwrite_func = fwrite_unlocked;
				feof_func = feof_unlocked;
			}
			else {
				fread_func = fread;
				fwrite_func = fwrite;
				feof_func = feof;
			}

			while(true) {
				// it is hard to check integrity there
				red = fread_func(buf, chunk_size, chunk_count, f_src);
				if(red == 0) {
					if(feof_func(f_src) != 0) {
						break; // eof
					}
					// if feof == 0
					ret = -1; // i/o error
					goto loop_break;
				}
				written = fwrite_func(buf, chunk_size, red, f_dst);
				if(written != red) {
					ret = -1;
					goto loop_break;
				}
				progress += written;
				// push current progress
				if(progress_out != NULL) *progress_out = progress;
			}

			if(mode & COPYFILE_FLAG_STDIO_UNLOCKED) {
				funlockfile(f_src);
				funlockfile(f_dst);
			}
		} break;

		case COPYFILE_CP: {
			pid_t pid = fork();
			switch(pid) {
				case -1: ret = -1; goto return_break;
				case 0: {
					execlp("cp", "cp", src, dst, NULL);
					exit(254); // this is executed only if exec failed
				} break;
				default: {
					int stat_loc;
					waitpid(pid, &stat_loc, 0);
					if (((stat_loc >> 8) & 0xff) == 0) {
						// cp exited with code 0
						if(progress_out != NULL) *progress_out = st.st_size; // is it safe?
					}
					else {
						ret = -1;
					}
				} break;
			}
		} break;

		default: ret = -1; break;
	}

loop_break:
	if(copyfile_internal) {
		if(use_fd) {
			close(fd_dst);
			close(fd_src);
		}

		else {
			fclose(f_dst);
			fclose(f_src);
		}

		if(buf != NULL)
			free(buf);
	}

return_break:
	return ret;
}
