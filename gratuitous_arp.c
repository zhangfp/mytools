#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <errno.h>


#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include <pcap.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>


#define	MAX_LOG_MSG	255
#define ETHERNET_MTU 1500
#define CAPTURE_TIMEOUT 1000

static char if_name[16] = {0};
static char vip[64] = {0};
static unsigned char hwaddr[6];
static struct in_addr vaddr;
static int dev_desc_fd = -1;
static pcap_t *dev_desc;
static char errbuf[PCAP_ERRBUF_SIZE];

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

int gratuitous_arp(const int dev_desc_fd)
{
    struct ether_header eh;
    static unsigned char arp[28] = {
            0x00, 0x01,   /* MAC address type */
            0x08, 0x00,   /* Protocol address type */
            0x06, 0x04,   /* MAC address size, protocol address size */
            0x00, 0x01,   /* OP (1=request, 2=reply) */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* Sender MAC */
            0x00, 0x00, 0x00, 0x00,               /* Sender IP */
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* Target MAC */
            0xff, 0xff, 0xff, 0xff                /* Target IP */
    };    
    unsigned char *pkt;
    int rc;

    arp[7] = 0x01;                                 /* request op */
    memcpy(&arp[8], hwaddr, sizeof hwaddr);        /* Sender MAC */
    memcpy(&arp[14], &vaddr.s_addr, (size_t) 4U);  /* Sender IP */
    memcpy(&arp[18], hwaddr, sizeof hwaddr);       /* Target MAC */
    memcpy(&arp[24], &vaddr.s_addr, (size_t) 4U);  /* Target IP */

    memset(&eh, 0, sizeof eh);
    memcpy(&eh.ether_shost, hwaddr, sizeof hwaddr);
    memset(&eh.ether_dhost, 0xff, ETHER_ADDR_LEN);
    eh.ether_type = htons(ETHERTYPE_ARP);

    if ((pkt = malloc((size_t)(sizeof eh + sizeof arp))) == NULL) 
    {
        log_message(LOG_INFO, "out of memory to send gratuitous ARP");
        return -1;
    }
    memcpy(pkt, &eh, sizeof eh);
    memcpy(pkt + sizeof eh, arp, sizeof arp);

    log_message(LOG_INFO, "Sending gratuitous ARP on %s for %s",
			    if_name, vip);
    do {
		rc = write(dev_desc_fd, pkt, sizeof eh + sizeof arp);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        printf("write() in garp: %s", strerror(errno));
        free(pkt);
        return -1;
    }
    free(pkt);
    
    return 0;
}

int get_hwaddr(char *interface ,unsigned char *mac_addr)
{
	struct ifreq ifr;
	int ret,s;

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(s < 0)
	{
		return -1;
	}
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

	ret = ioctl(s, SIOCGIFHWADDR, &ifr);
	if (ret == -1)
	{
		goto END;
	}
	
	memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
	
END:
	close(s);
	return ret;
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

	if ((dev_desc = pcap_open_live(if_name, ETHERNET_MTU, 0,
                                   CAPTURE_TIMEOUT, errbuf)) == NULL) 
	{
        log_message(LOG_INFO, "Unable to open interface [%s]: %s", if_name, errbuf);
        return -1;
    } 

	dev_desc_fd = pcap_fileno(dev_desc);

	ret = inet_pton(AF_INET,vip,&vaddr);
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

	ret = gratuitous_arp(dev_desc_fd);
	if(ret < 0)
	{
		log_message(LOG_INFO, "send gratuitous arp failed\n");
		exit(-1);
	}
	
	return 0;
}
