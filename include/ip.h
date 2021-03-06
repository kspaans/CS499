#ifndef _IP_H
#define _IP_H
#include <types.h>

enum {
	IPPROTO_IP = 0,			/* Dummy protocol for TCP		*/
	IPPROTO_HOPOPTS = 0,	/* IPv6 Hop-by-Hop options		*/
	IPPROTO_ICMP = 1,		/* Internet Control Message Protocol	*/
	IPPROTO_IGMP = 2,		/* Internet Gateway Management Protocol */
	IPPROTO_IPIP = 4,		/* IPIP tunnels (older KA9Q tunnels use 94) */
	IPPROTO_TCP = 6,		/* Transmission Control Protocol	*/
	IPPROTO_EGP = 8,		/* Exterior Gateway Protocol		*/
	IPPROTO_PUP = 12,		/* PUP protocol				*/
	IPPROTO_UDP = 17,		/* User Datagram Protocol		*/
	IPPROTO_IDP = 22,		/* XNS IDP protocol			*/
	IPPROTO_IPV6 = 41,		/* IPv6 header				*/
	IPPROTO_ROUTING = 43,	/* IPv6 Routing header			*/
	IPPROTO_FRAGMENT = 44,	/* IPv6 fragmentation header		*/
	IPPROTO_ESP = 50,		/* encapsulating security payload	*/
	IPPROTO_AH = 51,		/* authentication header		*/
	IPPROTO_ICMPV6 = 58,	/* ICMPv6				*/
	IPPROTO_NONE = 59,		/* IPv6 no next header			*/
	IPPROTO_DSTOPTS = 60,	/* IPv6 Destination options		*/
	IPPROTO_RAW = 255,		/* Raw IP packets			*/
	IPPROTO_MAX
};

#define SWAP16(x) \
	((((uint16_t)((x) & 0x00FF)) << 8) | \
	 (((uint16_t)((x) & 0xFF00)) >> 8))

#define SWAP32(x) __builtin_bswap32(x)

/* standard endian swap functions */
#define htonl(x) SWAP32(x)
#define htons(x) SWAP16(x)
#define ntohl(x) SWAP32(x)
#define ntohs(x) SWAP16(x)

struct ip {
#define IPVERSION       4
#define IP_MAKE_VHL(v, hl)      ((v) << 4 | (hl))
#define IP_VHL_HL(vhl)          ((vhl) & 0x0f)
#define IP_VHL_V(vhl)           ((vhl) >> 4)
#define IP_VHL_BORING           0x45
	uint8_t  ip_vhl;                 /* version << 4 | header length >> 2 */
	uint8_t  ip_tos;                 /* type of service */
	uint16_t ip_len;                 /* total length */
	uint16_t ip_id;                  /* identification */
	uint16_t ip_off;                 /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* don't fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
	uint8_t  ip_ttl;                 /* time to live */
	uint8_t  ip_p;                   /* protocol */
	uint16_t ip_sum;                 /* checksum */
	struct  in_addr ip_src,ip_dst;  /* source and dest address */
};

struct icmphdr {
#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQ 8
	uint8_t icmp_type;
#define ICMP_CODE_ECHO 0
	uint8_t icmp_code;
	uint16_t icmp_sum;
	uint32_t icmp_data;
};

struct udphdr {
	uint16_t	uh_sport;		/* source port */
	uint16_t	uh_dport;		/* destination port */
	uint16_t	uh_ulen;		/* udp length */
	uint16_t	uh_sum;			/* udp checksum */
};

#define UDPMTU 1400 // maximum size of udp payload

#define IP(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))
#define MAC(a,b,c,d,e,f) { .addr = { (a), (b), (c), (d), (e), (f) }}

#endif /* _IP_H */
