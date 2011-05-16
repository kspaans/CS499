#ifndef _IP_H
#define _IP_H
typedef uint32_t in_addr_t;

struct in_addr {
	in_addr_t s_addr;
};

enum {
	IPPROTO_IP = 0,		/* Dummy protocol for TCP		*/
	IPPROTO_HOPOPTS = 0,		/* IPv6 Hop-by-Hop options		*/
	IPPROTO_ICMP = 1,		/* Internet Control Message Protocol	*/
	IPPROTO_IGMP = 2,		/* Internet Gateway Management Protocol */
	IPPROTO_IPIP = 4,		/* IPIP tunnels (older KA9Q tunnels use 94) */
	IPPROTO_TCP = 6,		/* Transmission Control Protocol	*/
	IPPROTO_EGP = 8,		/* Exterior Gateway Protocol		*/
	IPPROTO_PUP = 12,		/* PUP protocol				*/
	IPPROTO_UDP = 17,		/* User Datagram Protocol		*/
	IPPROTO_IDP = 22,		/* XNS IDP protocol			*/
	IPPROTO_IPV6 = 41,		/* IPv6 header				*/
	IPPROTO_ROUTING = 43,		/* IPv6 Routing header			*/
	IPPROTO_FRAGMENT = 44,	/* IPv6 fragmentation header		*/
	IPPROTO_ESP = 50,		/* encapsulating security payload	*/
	IPPROTO_AH = 51,		/* authentication header		*/
	IPPROTO_ICMPV6 = 58,		/* ICMPv6				*/
	IPPROTO_NONE = 59,		/* IPv6 no next header			*/
	IPPROTO_DSTOPTS = 60,		/* IPv6 Destination options		*/
	IPPROTO_RAW = 255,		/* Raw IP packets			*/
	IPPROTO_MAX
};

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

struct udphdr {
	u_short	uh_sport;		/* source port */
	u_short	uh_dport;		/* destination port */
	u_short	uh_ulen;		/* udp length */
	u_short	uh_sum;			/* udp checksum */
};

#endif /* _IP_H */
