/*
 * A clean way to represent IPv4/IPv6 addresses for routines which purport to
 * handle either, inspiration from mrtd
 */
typedef struct _ip_t {
	u_short family;		/* AF_INET | AF_INET6 */
	u_short bitlen;		/* bits */
	int ref_count;          /* reference count */
	union {
		struct in_addr sin;
		struct in6_addr sin6;
	} addr;
} ip_t;

