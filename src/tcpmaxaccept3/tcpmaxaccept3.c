/*
 * TCP Max Accept v.3 (epoll)
 * Compile: gcc -std=gnu99 -O3 -ggdb -march=native -mtune=native -c -o tcpmaxaccept2.o
 * Link: gcc -o tcpmaxaccept3.exe linux_util.o vector.o -lpthread
 * Quick build: gcc -std=gnu99 -O3 -march=native -mtune=native -ggdb -o tcpmaxaccept3.exe \
 *				tcpmaxaccept3.c linux_util.c vector.c -lpthread
 */

#ifdef _MSC_VER
#undef __cplusplus
#define __builtin_alloca NULL
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/epoll.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "../liblinux_util/linux_util.h"
#include "../libvector/vector.h"

#define SERVER_PORT 2018
#define CUTSIZE 10
#define LISTEN_BACKLOG 1024 // we don't fear DDoS!
#define EPOLL_SLEEP 100
#define SPAWNER_SLEEP 350
#define SOCKETS_MAX 131071 // 65535 * 2 + 1
#define EPOLL_BATCH 16

volatile bool verbose = false;
const char text[] = "The quick brown fox jumps over the lazy dog.";
volatile size_t spawner_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
	int epoll_fd;
	unsigned short sv_port;
	struct sockaddr_in sv_addr;
	VECTOR *ports;
} SVPARAM;

typedef union {
	uint64_t v_vect;
	struct {
		int fd;
		unsigned short port;
		char _pad[2];
	} endpoint;
} FDEPUN;

int epoll_add(int epfd, int sockfd, int events, int extra32) {
	struct epoll_event epevent = { .events = events, .data.u64 = (((uint64_t)extra32) << 32) | sockfd };
	// does kernel copies the stucture pointed to?
	return epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &epevent);
}

// ipv4iface is reversed to network byte order
void reserve_max_ports(int32_t ipv4iface, VECTOR *vec_out) {
	assert(vec_out->size == 0);

	for(int port = 1; port <= 65535; ++port) {
		//if(port == SERVER_PORT)
		//	continue;
		int sock;
		__syscall(sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
		int val = 1;
		__syscall(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)));
		struct sockaddr_in sin = { .sin_addr = { ipv4iface }, .sin_family = AF_INET };
		socklen_t in_len = sizeof(sin);

		sin.sin_port = htons(((unsigned short)port & 0xffff));
		int bind_ret = bind(sock, (struct sockaddr*)&sin, in_len);
		if (bind_ret == 0) {
			FDEPUN fdport;
			fdport.endpoint.fd = sock;
			fdport.endpoint.port = ((unsigned short)port & 0xffff);
			assert(vector_push_back(vec_out, fdport.v_vect));
		}
		else {
			VERBOSE logprint("[:(] The port %hu is busy (%d = %s)\n"
				, (unsigned short)port, errno, strerror(errno));
			close(sock);
		}
	}
}

void *spawner_thread(void *argv) {
	SVPARAM param = *((SVPARAM*)argv);
	socklen_t addr_len = sizeof(param.sv_addr);

	bool leaving = false;
	while(true) {
		// wait for next events
		usleep(SPAWNER_SLEEP);

		FDEPUN fdport;
		fdport.v_vect = param.ports->array[spawner_count];
		int sock_peer = fdport.endpoint.fd;
		
		int val = 1;
		__syscall(setsockopt(sock_peer, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)));

		pthread_mutex_lock(&mutex);
		__syscall(connect(sock_peer, (struct sockaddr*)&param.sv_addr, addr_len));
		sysassert(send(sock_peer, text + rand() % sizeof(text) - CUTSIZE - 1, CUTSIZE, 0) == CUTSIZE);
		__syscall(epoll_add(param.epoll_fd, sock_peer, EPOLLIN | EPOLLERR, 0));
		
        ++spawner_count;
		if(spawner_count >= param.ports->size) {
			leaving = true;
		}
		pthread_mutex_unlock(&mutex);

		if(leaving)
			break;
	}

	logprint("[!] Spawner thread tired and leaving now\n");
	return NULL;
}

int main(int argc, char **argv) {
	int e;
	while((e = getopt(argc, argv, "v")) != -1) {
		switch(e) {
			case 'v': verbose = true; break;

			default:
				logprint(ANSI_BACKGROUND_YELLOW ANSI_COLOR_BLACK "[!]" ANSI_COLOR_RESET
					" Unknown command-line argument with value: %s\n", optarg);
			break;
		}
	}

	char buf[CUTSIZE];
	int sock;
	struct sockaddr_in s_addr = {
		  .sin_family = AF_INET
		, .sin_addr = { htonl(INADDR_LOOPBACK + 1) } // 127.0.0.2
		, .sin_port = htons(SERVER_PORT)
        , .sin_zero = (uint64_t)0
	};
	socklen_t addr_len = sizeof(s_addr);

	struct rlimit rlm;
	__syscall(getrlimit(RLIMIT_NOFILE, &rlm));
	if(rlm.rlim_cur < SOCKETS_MAX) {
		rlm.rlim_cur = SOCKETS_MAX;
		//rlm.rlim_max = RLIM_SAVED_CUR;
		if(setrlimit(RLIMIT_NOFILE, &rlm) == -1) {
			logprint(
				  ANSI_BACKGROUND_YELLOW ANSI_COLOR_BLACK "[!]" ANSI_COLOR_RESET
				  " Could not setrlimit() for RLIMIT_NOFILE; you can encounter open file limit\n"
				  "\t(%d = %s)\n"
				, errno, strerror(errno)
			);
		}
	}

	// main socket
	__syscall(sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

	size_t counter = 0;
	size_t counter_perc = 0;
	int val = 1; // allow reuse for quick rerun on errors
	__syscall(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)));
	__syscall(bind(sock, (struct sockaddr*)&s_addr, addr_len));
	__syscall(listen(sock, LISTEN_BACKLOG));

	// reserve sockets
	VECTOR vector_ports;
	assert(vector_init(&vector_ports, 65535));
	reserve_max_ports(htonl(INADDR_LOOPBACK + 1), &vector_ports);
	logprint("[i] Port reserver succeeded for %zu ports.\n", vector_ports.size);

	// наборчик событий от epoll
	int epfd;
	struct epoll_event epevent_list[EPOLL_BATCH];

	sigset_t set, set_prev;
	sigfillset(&set);

	// Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
	// Nowadays, this hint is no longer required (the kernel dynamically sizes the required
	// data structures without needing the hint), but size must still be greater than zero
	__syscall(epfd = epoll_create(1));
	__syscall(epoll_add(epfd, sock, EPOLLIN | EPOLLERR, 0));

	pthread_t spawner_tid;
	SVPARAM param = { .epoll_fd = epfd, .sv_addr = s_addr, .sv_port = SERVER_PORT, .ports = &vector_ports };
	if(pthread_create(&spawner_tid, NULL, spawner_thread, &param) == -1) {
		logprint(ANSI_BACKGROUND_RED ANSI_COLOR_BLACK "[x]" ANSI_COLOR_RESET
			" Could not pthread_create() to working thread; exiting.\n");
		syscall_error(ANSI_BACKGROUND_RED ANSI_COLOR_BLACK "[x]" ANSI_COLOR_RESET
			, __FILE__, __LINE__);
	}

	logprint("[i] Socket spawner created.\n");
	logprint("[i] Listening on port %d...\n", SERVER_PORT);

	while (true) {
		// pseudo-parallel behaviour
		struct sockaddr_in sockname, peername;
		socklen_t sockname_len = sizeof(sockname), peername_len = sizeof(peername);
		int epoll_ret;
		int sock_peer;
		__syscall(sigprocmask(SIG_BLOCK, &set, &set_prev));
		__syscall(epoll_ret = epoll_wait(epfd, epevent_list, EPOLL_BATCH, -1));
		__syscall(sigprocmask(SIG_SETMASK, &set_prev, NULL));

		if (epoll_ret == 0) {
			VERBOSE logprint("[-] No epoll events happened, sleep awhile\n");
			goto epoll_pause;
		}

		if(epoll_ret > 1) {
			VERBOSE logprint("\x1b[1;43m" ANSI_COLOR_BLACK "[i]" ANSI_COLOR_RESET
				" epoll events: %d\n", epoll_ret);
		}

		pthread_mutex_lock(&mutex);
		for(int i = 0; i < epoll_ret; i++) {
			struct epoll_event epevent = epevent_list[i]; // copy
			bool show_verbose_message = false;
			VERBOSE show_verbose_message = true;
			if(epevent.events & EPOLLERR) show_verbose_message = true;

			if(show_verbose_message) {
				__syscall(getsockname(epevent.data.fd, (struct sockaddr*)&sockname, &sockname_len));
				if (epevent.events & EPOLLERR)
					logprint(ANSI_BACKGROUND_RED ANSI_COLOR_WHITE "[x]" ANSI_COLOR_RESET);
				else
					logprint(ANSI_BACKGROUND_WHITE ANSI_COLOR_BLACK "[-]" ANSI_COLOR_RESET);
				
				logprint(
					  " Socket: %s:%hu <=> "
					, inet_ntoa(sockname.sin_addr)
					, htons(sockname.sin_port)
				);

				if (getpeername(epevent.data.fd, (struct sockaddr*)&peername, &peername_len) != -1) {
					logprint("%s:%hu", inet_ntoa(peername.sin_addr), htons(peername.sin_port));
				}
				else {
					logprint(ANSI_COLOR_YELLOW "LISTENING" ANSI_COLOR_RESET);
				}

				logprint("\tevent(s) = 0x%x\n", epevent.events);
			}
			sysassert(!(epevent.events & EPOLLERR));

			if (epevent.data.fd == sock) { 
				if(epevent.events & EPOLLIN) {
					// can accept
					__syscall(sock_peer = accept(sock, (struct sockaddr*)&peername, &peername_len));
					__syscall(epoll_add(epfd, sock_peer, EPOLLIN | EPOLLERR, 0));
					++counter;
					VERBOSE {
						logprint(ANSI_BACKGROUND_MAGENTA "[i]" ANSI_COLOR_RESET
						  " Accepted peer from %s:%hu (counter = %zu)\n"
							, inet_ntoa(peername.sin_addr)
							, htons(peername.sin_port)
							, counter
						);
					}
				}
			}

			else if((epevent.events & EPOLLIN) || (epevent.events & EPOLLOUT)) {
				if (epevent.events & EPOLLIN) {
					// can read?
					ssize_t rcvd;
					lassert((rcvd = recv(epevent.data.fd, buf, CUTSIZE, 0)) == CUTSIZE);
					VERBOSE {
						logprint(ANSI_BACKGROUND_CYAN ANSI_COLOR_BLACK "[i]" ANSI_COLOR_RESET
							  " Received data: " ANSI_COLOR_MAGENTA "%10s" ANSI_COLOR_RESET "\n"
							, buf
						);
					}

					// remove from epoll, to ease epoll's task
					__syscall(shutdown(epevent.data.fd, SHUT_RD));
					__syscall(epoll_ctl(epfd, EPOLL_CTL_DEL, epevent.data.fd, NULL));
				}
			}

			else {
				// unknown event
				logprint("[-] Event: %x, fd: %d, data x64: 0x%016lx\n",
					epevent.events,
					epevent.data.fd,
					epevent.data.u64);
			}
		}
		if((counter >> 5) != counter_perc || counter == vector_ports.size) {
			logprint("\033[1A" ANSI_BACKGROUND_GREEN ANSI_COLOR_BLACK "[i]" ANSI_COLOR_RESET
						" Child connections accepted, "
						"counter = " ANSI_COLOR_GREEN "%zu" ANSI_COLOR_RESET " "
						"spawner = " ANSI_COLOR_MAGENTA "%zu" ANSI_COLOR_RESET
						"\n"
				, counter, spawner_count
			);
			counter_perc = (counter >> 5);
		}
		pthread_mutex_unlock(&mutex);

		if(counter == vector_ports.size) {
			sysassert(pthread_join(spawner_tid, NULL) == 0);
			logprint("Press any key to continue... ");
			getchar();
			break;
		}

epoll_pause:
		usleep(EPOLL_SLEEP);
	}

	return EXIT_SUCCESS;
}
