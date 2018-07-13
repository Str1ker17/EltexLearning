#ifdef _MSC_VER
#undef __cplusplus
#endif

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "copyfile.h"
#include "../libncurses_util/linux_util.h"

int copyfile(const char *src, const char *dst, int mode) {
	int fd_src = -1, fd_dst = -1;
	FILE *f_src = NULL, *f_dst = NULL;

	char buf[BUFSIZ];

	struct stat st;

	ssize_t red, written;
	size_t(*fread_f)(void*, size_t, size_t, FILE*) = &fread;
	size_t(*fwrite_f)(const void*, size_t, size_t, FILE*) = &fwrite;
	size_t chunk_size = 1, chunk_count = BUFSIZ;

	int mode_i = COPYFILE_MODE(mode);
	__syscall(lstat(src, &st));

	bool use_fd = (!!(mode & COPYFILE_USE_FD));
	int ret = 0;

	switch(mode_i) {
		case COPYFILE_SENDFILE:
			use_fd = true;
			__syscall(fd_src = open(src, O_RDONLY));
			__syscall(fd_dst = open(dst, O_WRONLY | O_CREAT, st.st_mode & 07777));
			lassert(sendfile(fd_dst, fd_src, NULL, st.st_size) == st.st_size);
		break;

		case COPYFILE_READWRITE_CHUNKS:
			chunk_size = BUFSIZ;
			chunk_count = 1;
		case COPYFILE_READWRITE_BYTES:
			if (use_fd) { // use open/read/write
				__syscall(fd_src = open(src, O_RDONLY));
				__syscall(fd_dst = open(dst, O_WRONLY | O_CREAT, st.st_mode & 07777));
				while(true) {
					red = read(fd_src, buf, BUFSIZ);
					if(red == -1) {
						ret = -1; // read error
						goto loop_break;
					}
					if(red == 0)
						break; // eof

					// read()/write() do not support chunks, only bytes
					lassert((written = write(fd_dst, buf, red)) == red);
				}
			}
			else { // use fopen/fread/fwrite
				lassert((f_src = fopen(src, "rb")) != NULL);
				lassert((f_dst = fopen(dst, "wb")) != NULL);

				if(mode & COPYFILE_USE_UNLOCKED) {
					flockfile(f_src);
					flockfile(f_dst);
					fread_f = fread_unlocked;
					fwrite_f = fwrite_unlocked;
				}

				while(true) {
					// it is hard to check integrity there
					red = fread_f(buf, chunk_size, chunk_count, f_src);
					if(red == 0) {
						if(feof_unlocked(f_src) != 0) 
							break; // eof
						// if feof == 0
						ret = -1; // i/o error
						goto loop_break;
					}
					written = fwrite_f(buf, chunk_size, red, f_dst);
					lassert(written == red);
				}

				if(mode & COPYFILE_USE_UNLOCKED) {
					funlockfile(f_src);
					funlockfile(f_dst);
				}

			}
		break;

		default: ret = -1; break;
	}

loop_break:
	if(use_fd) {
		close(fd_dst);
		close(fd_src);
	}

	else {
		fclose(f_dst);
		fclose(f_src);
	}

	return ret;
}
