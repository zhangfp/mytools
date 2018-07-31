/* system includes */
#include <asm/byteorder.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <unistd.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/sockios.h>
#include <stdarg.h>
#include <syslog.h>


#define NDISC_HOPLIMIT	255
#define	MAX_LOG_MSG	255


/*
 *	IPv6 Header
 */
struct ip6hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8			priority:4,
				version:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8			version:4,
				priority:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	__u8			flow_lbl[3];

	__be16			payload_len;
	__u8			nexthdr;
	__u8			hop_limit;

	struct	in6_addr	saddr;
	struct	in6_addr	daddr;
};


/* static vars */
static char *ndisc_buffer;
static int ndisc_fd = -1;

static char if_name[16] = {0};
static char vip[64] = {0};
static struct in6_addr vaddr;
static unsigned char hwaddr[6];
static int if_index = 0;

static void
vlog_message(const int facility, const char* format, va_list args)
{
#if !HAVE_VSYSLOG
	char buf[MAX_LOG_MSG+1];

	vsnprintf(buf, sizeof(buf), format, args);
#endif

#if HAVE_VSYSLOG
		vsyslog(facility, format, args);
#else
		syslog(facility, "%s", buf);
#endif
}


static void
log_message(const int facility, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vlog_message(facility, format, args);
	va_end(args);
}

static int
get_hwaddr(char *interface ,unsigned char *mac_addr)
{
	struct ifreq ifr;
	int ret,s;

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(s < 0)
	{
		return -1;
	}
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);

	ret = ioctl(s, SIOCGIFHWADDR, &ifr);
	if (ret == -1)
	{
		return -1;
	}
	memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);

	if (s > 0)
		close(s);
	
	return 0;
}


/*
 *	Neighbour Advertisement sending routine.
 */
static void
ndisc_send_na()
{
	struct sockaddr_ll sll;
	ssize_t len;
	//char addr_str[INET6_ADDRSTRLEN] = "";

	/* Build the dst device */
	memset(&sll, 0, sizeof (sll));
	sll.sll_family = AF_PACKET;
	memcpy(sll.sll_addr, hwaddr, ETH_ALEN);
	sll.sll_halen = ETH_ALEN;
	sll.sll_ifindex = /*(int)IF_INDEX(ipaddress->ifp)*/if_index;
/*
	if (__test_bit(LOG_DETAIL_BIT, &debug)) {
		inet_ntop(AF_INET6, &ipaddress->u.sin6_addr, addr_str, sizeof(addr_str));
		log_message(LOG_INFO, "Sending unsolicited Neighbour Advert on %s for %s",
			    IF_NAME(ipaddress->ifp), addr_str);

	}
*/
	log_message(LOG_INFO, "Sending unsolicited Neighbour Advert on %s for %s",
			    if_name, vip);
			    
	/* Send packet */
	len = sendto(ndisc_fd, ndisc_buffer,
		     ETHER_HDR_LEN + sizeof(struct ip6hdr) + sizeof(struct nd_neighbor_advert) +
		     sizeof(struct nd_opt_hdr) + ETH_ALEN, 0,
		     (struct sockaddr *) &sll, sizeof (sll));

	if (len < 0) {
		log_message(LOG_INFO, "Error sending ndisc unsolicited neighbour advert on %s for %s",
			    if_name, vip);
	}

}

/*
 *	ICMPv6 Checksuming.
 */
static __sum16
ndisc_icmp6_cksum(const struct ip6hdr *ip6, const struct icmp6_hdr *icp, uint32_t len)
{
	size_t i;
	register const uint16_t *sp;
	uint32_t sum;
	union {
		struct {
			struct in6_addr ph_src;
			struct in6_addr ph_dst;
			uint32_t	ph_len;
			uint8_t	ph_zero[3];
			uint8_t	ph_nxt;
		} ph;
		uint16_t pa[20];
	} phu;

	/* pseudo-header */
	memset(&phu, 0, sizeof(phu));
	memcpy(&phu.ph.ph_src, &ip6->saddr, sizeof(struct in6_addr));
	memcpy(&phu.ph.ph_dst, &ip6->daddr, sizeof(struct in6_addr));
	phu.ph.ph_len = htonl(len);
	phu.ph.ph_nxt = IPPROTO_ICMPV6;

	sum = 0;
	for (i = 0; i < sizeof(phu.pa) / sizeof(phu.pa[0]); i++)
		sum += phu.pa[i];

	sp = (const uint16_t *)icp;

	for (i = 1; i < len; i += 2)
		sum += *sp++;

	if (len & 1)
		sum += htons((*(const uint8_t *)sp) << 8);

	while (sum > 0xffff)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum & 0xffff;
}

/*
 *	Build an unsolicited Neighbour Advertisement.
 *	As explained in rfc4861.4.4, a node sends unsolicited
 *	Neighbor Advertisements in order to (unreliably) propagate
 *	new information quickly.
 */
void
ndisc_send_unsolicited_na_immediate()
{
	struct ether_header *eth = (struct ether_header *) ndisc_buffer;
	struct ip6hdr *ip6h = (struct ip6hdr *) ((char *)eth + ETHER_HDR_LEN);
	struct nd_neighbor_advert *ndh = (struct nd_neighbor_advert*) ((char *)ip6h + sizeof(struct ip6hdr));
	struct icmp6_hdr *icmp6h = &ndh->nd_na_hdr;
	struct nd_opt_hdr *nd_opt_h = (struct nd_opt_hdr *) ((char *)ndh + sizeof(struct nd_neighbor_advert));
	char *nd_opt_lladdr = (char *) ((char *)nd_opt_h + sizeof(struct nd_opt_hdr));
	//char *lladdr = (char *) IF_HWADDR(ipaddress->ifp);

	/* Ethernet header:
	 * Destination ethernet address MUST use specific address Mapping
	 * as specified in rfc2464.7 Address Mapping for
	 */
	memset(eth->ether_dhost, 0, ETH_ALEN);
	eth->ether_dhost[0] = eth->ether_dhost[1] = 0x33;
	eth->ether_dhost[5] = 1;
	memcpy(eth->ether_shost, /*lladdr*/hwaddr, ETH_ALEN);
	eth->ether_type = htons(ETHERTYPE_IPV6);

	/* IPv6 Header */
	ip6h->version = 6;
	ip6h->payload_len = htons(sizeof(struct nd_neighbor_advert) + sizeof(struct nd_opt_hdr) + ETH_ALEN);
	ip6h->nexthdr = IPPROTO_ICMPV6;
	ip6h->hop_limit = NDISC_HOPLIMIT;
	memcpy(&ip6h->saddr, &vaddr, sizeof(struct in6_addr));
	ip6h->daddr.s6_addr16[0] = htons(0xff02);
	ip6h->daddr.s6_addr16[7] = htons(1);

	/* ICMPv6 Header */
//	icmp6h->icmp6_type = ND_NEIGHBOR_ADVERT;
//	icmp6h->icmp6_router = ifp->gna_router;
	ndh->nd_na_type = ND_NEIGHBOR_ADVERT;
	//if (ifp->gna_router)
	//	ndh->nd_na_flags_reserved |= ND_NA_FLAG_ROUTER;

	/* Override flag is set to indicate that the advertisement
	 * should override an existing cache entry and update the
	 * cached link-layer address.
	 */
//	icmp6h->icmp6_override = 1;
	ndh->nd_na_flags_reserved |= ND_NA_FLAG_OVERRIDE;
	ndh->nd_na_target = vaddr;

	/* NDISC Option header */
	nd_opt_h->nd_opt_type = ND_OPT_TARGET_LINKADDR;
	nd_opt_h->nd_opt_len = 1;
	memcpy(nd_opt_lladdr, hwaddr, ETH_ALEN);

	/* Compute checksum */
	icmp6h->icmp6_cksum = ndisc_icmp6_cksum(ip6h, icmp6h,
						sizeof(struct nd_neighbor_advert) + sizeof(struct nd_opt_hdr) + ETH_ALEN);

	/* Send the neighbor advertisement message */
	ndisc_send_na();

	/* Cleanup room for next round */
	memset(ndisc_buffer, 0, ETHER_HDR_LEN + sizeof(struct ip6hdr) +
	       sizeof(struct nd_neighbor_advert) + sizeof(struct nd_opt_hdr) + ETH_ALEN);
	       
}

/*
 *	Neighbour Discovery init/close
 */
int
ndisc_init(void)
{
	if (ndisc_buffer)
	{
		log_message(LOG_INFO, "ndisc_buffer already exist.\n");
		return 0;
	}

	/* Initalize shared buffer */
	ndisc_buffer = (char *) malloc(ETHER_HDR_LEN + sizeof(struct ip6hdr) +
				       sizeof(struct nd_neighbor_advert) + sizeof(struct nd_opt_hdr) + ETH_ALEN);

	if (NULL == ndisc_buffer)
	{
		return -1;
	}
	/* Create the socket descriptor */
	ndisc_fd = socket(PF_PACKET, SOCK_RAW | SOCK_CLOEXEC, htons(ETH_P_IPV6));
//#if !HAVE_DECL_SOCK_CLOEXEC
//	set_sock_flags(ndisc_fd, F_SETFD, FD_CLOEXEC);
//#endif

	if (ndisc_fd > 0)
	{
		//log_message(LOG_INFO, "Registering gratuitous NDISC shared channel\n");
		return 0;
	}
	else
	{
		log_message(LOG_INFO, "Error while registering gratuitous NDISC shared channel\n");
		free(ndisc_buffer);
		return -1;
	}

}

void
ndisc_close(void)
{
	if (!ndisc_buffer)
		return;

	free(ndisc_buffer);
	ndisc_buffer = NULL;
	close(ndisc_fd);
	ndisc_fd = -1;
}

int main(int argc, char *argv[])
{
	int ret;

	if(argc != 3)
	{
		log_message(LOG_INFO, "USAGE: %s ethX vip\n",argv[0]);
		exit(-1);
	}	

	strncpy(if_name, argv[1], sizeof(if_name) - 1);
	strncpy(vip, argv[2], sizeof(vip) - 1);

	if (ndisc_init() < 0)
	{
		exit(-1);
	}
	
	ret = inet_pton(AF_INET6, vip, &vaddr);
  	if(ret<=0)
	{
		log_message(LOG_INFO, "inet_pton error");
		exit(-1);
	}

	ret = get_hwaddr(if_name, hwaddr);
	if(ret<0)
	{
		log_message(LOG_INFO, "get hwaddr failed\n");
		exit(-1);
	}
/*
	int i = 0;
	printf("hwaddr: ");
	for (i = 0;i < 6; i++)
	{
		if (i < 5)
		printf("%02x:", hwaddr[i]);
		else
		printf("%02x\n", hwaddr[i]);
	}
*/
	if_index = (int)if_nametoindex(if_name);

	if (if_index == 0)
	{
		log_message(LOG_INFO, "get interface index failed\n");
	}

//	printf("if_index: %d\n", if_index);

	ndisc_send_unsolicited_na_immediate();
	ndisc_close();
	return 0;
}

