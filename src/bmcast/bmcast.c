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
#include <ifaddrs.h>
#include <linux/if_link.h>
#include "../liblinux_util/linux_util.h"
#include <time.h>

#define TEXT_STRMAX 80
#define DGRAM_MAX 65535

const char text[][TEXT_STRMAX] = {
	"I liked it better when it didn't cost innocence",
	"Lost myself in accomplishment",
	"And all sight of all the things that I should see",
	"And I say",
	"Now the judge and the jury they sit and stare",
	"On my own without you here",
	"Just here me out",
	"And take a look around",
	"I'm letting go but all I see is evidence",
	"Of what I know everything around just don't make sense",
	"I'm letting go but all I feel is consequence",
	"Of what I know everything around just don't make sense",
	"I liked it better when I thought that I was free",
	"One on one not two on me",
	"This blind justice it seems to have a court",
	"And I say",
	"I feel it coming and I take it on myself",
	"Before i succumb to someone else",
	"Change the ending now",
	"And let myself break free",
	"I'm letting go but all I see is evidence",
	"Of what I know everything around just don't make sense",
	"I'm letting go but all feel is consequence",
	"Of what I know everything around just don't make sense"
};
const size_t text_lines = sizeof(text) / TEXT_STRMAX;

const char shortOpts[] = ":BM:SRTUp:b:";
const struct option longOpts[] = {
      { "broadcast", no_argument, NULL, 'B' }
	, { "multicast", required_argument, NULL, 'M' }
	, { "sender", no_argument, NULL, 'S' }
	, { "receiver", no_argument, NULL, 'R' }
	, { "proto-tcp", no_argument, NULL, 'T' }
	, { "proto-udp", no_argument, NULL, 'U' }
	, { "port", required_argument, NULL, 'p' }
	, { "bind-to", required_argument, NULL, 'b' }
	, { "verbose", no_argument, NULL, 'v' }
	, { "help", no_argument, NULL, 'h' }
    , { NULL, no_argument, NULL, 0 }
};

#define FOREACH_FRUIT(FRUIT)                 \
        FRUIT(BMCAST_MULTICAST_TCP_SENDER)   \
        FRUIT(BMCAST_MULTICAST_TCP_RECEIVER) \
        FRUIT(BMCAST_MULTICAST_UDP_SENDER)   \
        FRUIT(BMCAST_MULTICAST_UDP_RECEIVER) \
		FRUIT(BMCAST_BROADCAST_TCP_SENDER)   \
		FRUIT(BMCAST_BROADCAST_TCP_RECEIVER) \
		FRUIT(BMCAST_BROADCAST_UDP_SENDER)   \
		FRUIT(BMCAST_BROADCAST_UDP_RECEIVER) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum __bitf_rt_mode {
	FOREACH_FRUIT(GENERATE_ENUM)
} BFRTMode;

static const char *__bitf_rt_mode_name[] = {
	FOREACH_FRUIT(GENERATE_STRING)
};

typedef struct __bitfield_options {
	// WARNING: we are on Little-Endian architecture
	unsigned receiver: 1; // false if sender
	unsigned proto_udp: 1; // false if tcp
	unsigned broadcast: 1; // false if multicast

	unsigned _if_accessible: 1;
	unsigned _if_bcast_flag: 1;
	unsigned __padding: 11;
} __attribute__((packed)) BFOptions;

typedef struct __rt_options {
	union {
		uint16_t numeric_value;
		BFOptions l;
	} f;
	uint16_t port; // default port
	int sock;
	struct in_addr addrv4_if;
	struct in_addr addrv4_mask;
	struct in_addr addrv4_grp;
} RTOptions;

#define bflags f.l

#ifdef _MSC_VER
#define __builtin_popcount(val) 0
#endif

void print_help() {
	ALOGV("B/M cast multisoftware\n");
	for(size_t i = 0; i < sizeof(longOpts) / sizeof(struct option); i++) {
		if(longOpts[i].val != '\0') logprint("-%c\t", longOpts[i].val);
		if(longOpts[i].name != NULL) logprint("--%s\t", longOpts[i].name);
		logprint("\n");
	}
}

struct ifaddrs *autoselect_iface(struct ifaddrs *ifaces) {
	size_t pos = 0;
	struct ifaddrs *ret = NULL;
	struct ifaddrs *ifap = ifaces;
	while(ifap != NULL) {
		if(    ifap->ifa_addr == NULL
			|| ifap->ifa_addr->sa_family != AF_INET
			|| !(ifap->ifa_flags & IFLA_BROADCAST)
		)
			goto next_iface;

		if(((struct sockaddr_in*)(ifap->ifa_addr))->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
			ret = ifap;
			break;
		}

next_iface:
		ifap = ifap->ifa_next;
		++pos;
	}
	return ret;
}

void print_options(RTOptions *rto) {
	char optval[INET_ADDRSTRLEN * 3];
	inet_ntop(AF_INET, &(rto->addrv4_if), optval, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(rto->addrv4_mask), optval + INET_ADDRSTRLEN, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(rto->addrv4_grp), optval + INET_ADDRSTRLEN * 2, INET_ADDRSTRLEN);
	ALOGV("Mode: %s %s %s (mode no = %u), port %hu\n"
		, rto->bflags.broadcast ? "Broadcast" : "Multicast"
		, rto->bflags.proto_udp ? "UDP" : "TCP"
		, rto->bflags.receiver ? "Receiver" : "Sender"
		, rto->f.numeric_value & 0x7
		, ntohs(rto->port)
	);
	char *if_color = (rto->f.l._if_accessible ? ANSI_COLOR_BRIGHT_WHITE : ANSI_COLOR_RED);
	ALOGV("addr_if: "            "%s"          "%s" ANSI_CLRST " \t"
		  "netmask: "  ANSI_COLOR_BRIGHT_WHITE "%s" ANSI_CLRST " \t"
		  "addr_grp: " ANSI_COLOR_BRIGHT_WHITE "%s" ANSI_CLRST "\n"
		, if_color
		, optval, optval + INET_ADDRSTRLEN, optval + INET_ADDRSTRLEN * 2
	);
}

ssize_t perform_broadcast_udp_sender(RTOptions *rto) {
	// TODO: check settings first!
	__syscall(rto->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
	int val = 1;
	__syscall(setsockopt(rto->sock, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)));
	struct sockaddr_in sin = {
		  .sin_addr = ((rto->addrv4_if.s_addr) | ~(rto->addrv4_mask.s_addr))
		, .sin_port = htons(rto->port)
		, .sin_family = AF_INET
	};
	const char *ptr = text[rand() % text_lines];
	ssize_t sent;
	__syscall(sent = sendto(rto->sock, ptr, strlen(ptr) + 1, 0, (struct sockaddr*)&sin, sizeof(sin)));

	return sent;
}

ssize_t perform_broadcast_udp_receiver(RTOptions *rto) {
	// TODO: check settings first!
	__syscall(rto->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
	struct sockaddr_in sin = {
		  .sin_addr = ((rto->addrv4_if.s_addr) | ~(rto->addrv4_mask.s_addr))
		, .sin_port = htons(rto->port)
		, .sin_family = AF_INET
	};
	__syscall(bind(rto->sock, (struct sockaddr*)&sin, sizeof(sin)));

	char buf[DGRAM_MAX];
	char optval[INET_ADDRSTRLEN];
	ssize_t rcvd;
	while(true) {
		socklen_t addrlen = sizeof(sin);
		__syscall(rcvd = recvfrom(rto->sock, buf, DGRAM_MAX, 0, (struct sockaddr*)&sin, &addrlen));
		inet_ntop(AF_INET, &(sin.sin_addr), optval, INET_ADDRSTRLEN);
		ALOGI("Received from %s:%hu, %u bytes len:\n", optval, ntohs(sin.sin_port), rcvd);
		logprint("\t%s\n", buf);
	}

	return 0;
}

int main(int argc, char **argv) {
	// defaults
	BFOptions bfo = { 
		  .broadcast = 1
		, .proto_udp = 1
		, .receiver = 0
		, ._if_accessible = 0
		, ._if_bcast_flag = 0
		, .__padding = ((1 << 11) - 1)
	};
	RTOptions rto = {
		  .f.l = bfo
		, .port = htons(2018)
		, .addrv4_if = INADDR_ANY
		, .addrv4_grp = htonl(0xE0000001) // 224.0.0.1
		, .addrv4_mask = ~INADDR_ANY
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
			case 'B': rto.bflags.broadcast = true; break;
			case 'M': {
				lassert(inet_pton(AF_INET, optarg, &rto.addrv4_grp) == 1);
				rto.bflags.broadcast = false;
			} break;
			case 'S': rto.bflags.receiver = false; break;
			case 'R': rto.bflags.receiver = true; break;
			case 'T': rto.bflags.proto_udp = false; break;
			case 'U': rto.bflags.proto_udp = true; break;
			case 'p': {
				rto.port = htons((uint16_t)(atoi(optarg)));
			} break;
			case 'b': {
				lassert(inet_pton(AF_INET, optarg, &rto.addrv4_if) == 1);
			} break;
			case 'v': verbose = true; break;
			case 'h': {
				print_help();
				exit(EXIT_FAILURE);
			} break;

			case '?':
			default:
				ALOGW("Unknown command-line argument '%c' with value: %s\n", optopt, optarg);
			break;
		}
	}

	srand(time(NULL));
	// TODO: https://linux.die.net/man/3/getifaddrs
	// Also, https://gist.github.com/edufelipe/6108057
	bool addrv4_if_accessible = false;
	char addrstr[INET_ADDRSTRLEN];
	struct in_addr addrv4;
	struct ifaddrs *ifap_head;
	__syscall(getifaddrs(&ifap_head));
	struct ifaddrs *ifap = ifap_head;
	while(ifap != NULL) {
		char *fam = "UNKNOWN";

		if(ifap->ifa_addr != NULL && ifap->ifa_addr->sa_family == AF_INET) {
			// ipv4 only
			switch(ifap->ifa_addr->sa_family) {
				case AF_PACKET: fam = "AF_PACKET"; break;
				case AF_INET: fam = "AF_INET"; break;
				case AF_INET6: fam = "AF_INET6"; break;
				default: break;
			}
			ALOGD("iface: %s\t family: %s\n", ifap->ifa_name, fam);

			if(ifap->ifa_addr->sa_family == AF_INET) {
				addrv4 = ((struct sockaddr_in*)ifap->ifa_addr)->sin_addr;
				lassert(
					inet_ntop(AF_INET, &addrv4, addrstr, INET_ADDRSTRLEN)
				!= NULL);
				logprint("\taddr: %s", addrstr);
				
				lassert(
					inet_ntop(AF_INET
						, &(((struct sockaddr_in*)ifap->ifa_netmask)->sin_addr.s_addr)
						, addrstr, INET_ADDRSTRLEN
					)
				!= NULL);
				logprint("\tmask: %s", addrstr);
				
				if(ifap->ifa_flags & IFLA_BROADCAST) {
					// allow only ifaces with broadcast flag
					if(addrv4.s_addr == rto.addrv4_if.s_addr)
						addrv4_if_accessible = true;

					lassert(
						inet_ntop(AF_INET
							, &(((struct sockaddr_in*)ifap->ifa_ifu.ifu_broadaddr)->sin_addr.s_addr)
							, addrstr, INET_ADDRSTRLEN
						)
					!= NULL);
					logprint("\tbcast: %s", addrstr);
				}

				logprint("\n");
			}
		}

		ifap = ifap->ifa_next;
		if(ifap == ifap_head)
			break;
	}

	if(rto.addrv4_if.s_addr == htonl(INADDR_ANY)) {
		ifap = autoselect_iface(ifap_head);
		if(ifap != NULL) {
			rto.addrv4_if = ((struct sockaddr_in*)ifap->ifa_addr)->sin_addr;
			rto.addrv4_mask = ((struct sockaddr_in*)ifap->ifa_netmask)->sin_addr;
			lassert(inet_ntop(AF_INET, &rto.addrv4_if, addrstr, INET_ADDRSTRLEN) != NULL);
			rto.f.l._if_accessible = true;
			ALOGI("Autoselected interface: %s/%d\n", addrstr, __builtin_popcount(rto.addrv4_mask.s_addr));
		}
	}

	print_options(&rto);

	BFRTMode fl_mode = rto.f.numeric_value & 0x7;
	switch(fl_mode) {
		//case BMCAST_MULTICAST_TCP_SENDER: break;
		//case BMCAST_MULTICAST_TCP_RECEIVER: break;
		//case BMCAST_MULTICAST_UDP_SENDER: break;
		//case BMCAST_MULTICAST_UDP_RECEIVER: break;
		case BMCAST_BROADCAST_UDP_SENDER: perform_broadcast_udp_sender(&rto); break;
		case BMCAST_BROADCAST_UDP_RECEIVER: perform_broadcast_udp_receiver(&rto); break;
		default:
			ALOGE("Unsupported mode %s\n", __bitf_rt_mode_name[fl_mode]);
			exit(EXIT_FAILURE);
		break;
	}

	return EXIT_SUCCESS;
}
