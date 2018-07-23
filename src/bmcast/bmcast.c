// Note: this include is a beta feature for design- and compile-time
#include "../liblinux_util/mscfix.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../liblinux_util/linux_util.h"

const char shortOpts[] = ":BM:SRTUbp:";
const struct option longOpts[] = {
      { "broadcast", no_argument, NULL, 'B' }
	, { "multicast", required_argument, NULL, 'M' }
	, { "sender", no_argument, NULL, 'S' }
	, { "receiver", no_argument, NULL, 'R' }
	, { "port", required_argument, NULL, 'p' }
	, { "bind-to", required_argument, NULL, 'b' }
	, { "proto-tcp", no_argument, NULL, 'T' }
	, { "proto-udp", no_argument, NULL, 'U' }
    , { NULL, no_argument, NULL, 0 }
};

typedef struct __bitfield_options {
	unsigned broadcast: 1; // false if multicast
	unsigned receiver: 1; // false if sender
	unsigned proto_udp: 1;
	unsigned __padding: 13;
} __attribute__((packed)) BFOptions;

typedef struct __rt_options {
	union __flags {
		uint16_t numeric_value;
		BFOptions l;
	} f;
	uint16_t port; // default port
	struct in_addr addrv4_if;
	struct in_addr addrv4_grp;
	struct in_addr addrv4_dest;
} RTOptions;

//#assert(sizeof(RTOptions) == 16)

void print_options(RTOptions *rto) {
	char optval[INET_ADDRSTRLEN * 3];
	inet_ntop(AF_INET, &(rto->addrv4_if), optval, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(rto->addrv4_dest), optval + INET_ADDRSTRLEN, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(rto->addrv4_grp), optval + INET_ADDRSTRLEN * 2, INET_ADDRSTRLEN);
	ALOGV("Mode: %s %s %s (mode no = %u), port %hu\n"
		, rto->f.l.broadcast ? "Broadcast" : "Multicast"
		, rto->f.l.proto_udp ? "UDP" : "TCP"
		, rto->f.l.receiver ? "Receiver" : "Sender"
		, rto->f.numeric_value & 0x7
		, rto->port
	);
	ALOGV("addr_if: "   ANSI_COLOR_BRIGHT_WHITE "%s" ANSI_CLRST " \t"
		  "addr_dest: " ANSI_COLOR_BRIGHT_WHITE "%s" ANSI_CLRST " \t"
		  "addr_grp: "  ANSI_COLOR_BRIGHT_WHITE "%s" ANSI_CLRST "\n"
		, optval, optval + INET_ADDRSTRLEN, optval + INET_ADDRSTRLEN * 2
	);
}

int perform_broadcast_sender(RTOptions *rto) {
	return -1;
}

int main(int argc, char **argv) {
	// defaults
	BFOptions bfo = { 
		  .broadcast = 1
		, .receiver = 0
		, .proto_udp = 1
		, .__padding = ((1 << 13) - 1)
	};
	RTOptions rto = {
		  .f.l = bfo
		, .port = 2018
		, .addrv4_if = INADDR_ANY
		, .addrv4_grp = inet_addr("224.0.0.1")
		, .addrv4_dest = INADDR_ANY
	};

	// parse options
	while(true) {
		int longind = -1;
		int e = getopt_long(argc, argv, shortOpts, longOpts, &longind);
		if(e == -1)
			break;

		//if (longind == -1)
		//	printf("%c = %s\n", e, optarg);
		//else
		//	printf("%c %s = %s\n", e, longOpts[longind].name, optarg);

		switch(e) {
			case 'B': rto.f.l.broadcast = true; break;
			case 'M': {
				lassert(inet_pton(AF_INET, optarg, &rto.addrv4_grp) == 1);
				rto.f.l.broadcast = false;
			} break;
			case 'S': rto.f.l.receiver = false; break;
			case 'R': rto.f.l.receiver = true; break;
			case 'T': rto.f.l.proto_udp = false; break;
			case 'U': rto.f.l.proto_udp = true; break;
			case 'p': {
				rto.port = (uint16_t)(atoi(optarg));
			} break;
			case 'b': break;

			case '?':
			default:
				ALOGW("Unknown command-line argument '%c' with value: %s\n", optopt, optarg);
			break;
		}
	}

	// TODO: https://linux.die.net/man/3/getifaddrs
	// Also, https://gist.github.com/edufelipe/6108057
	print_options(&rto);

	return EXIT_SUCCESS;
}
