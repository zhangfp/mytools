#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>
#include <net/if.h>
#include <stdio.h>


//TODO
/*
1.get ip by nic name
2.get gateway ip
3.
*/

int
get_hwaddr(int s, char *interface ,unsigned char *mac_addr)
{
	struct ifreq ifr;
	int ret;

	if (s <= 0)
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

int
get_ipv4_addr(int s, char *interface ,char *ipv4_addr)
{
	struct ifreq ifr;
	int ret;

	if (s <= 0)
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		
	if(s < 0)
	{
		return -1;
	}
	
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);

	ret = ioctl(s, SIOCGIFADDR, &ifr);
	if (ret == -1)
	{
		return -1;
	}

	if ( ifr.ifr_addr.sa_family != AF_INET)
	{
		return -1;
	}

	;

	if (NULL == inet_ntop(AF_INET, &( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr, 
		ipv4_addr, INET_ADDRSTRLEN))
	{
		return -1;
	}

	if (s > 0)
		close(s);
	
	return 0;
}


int
get_ipv6_addr(char *interface ,char *ipv6_addr)
{
#define IPV6_ADDR_GLOBAL        0x0000U
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
#define IPV6_ADDR_COMPATv4      0x0080U

#define PRIORITY_LOOPBACK   1
#define PRIORITY_LINKLOCAL  2
#define PRIORITY_SITELOCAL  3
#define PRIORITY_GLOBAL     4

	FILE *f;
    int ret, scope, prefix;
    unsigned char ipv6[16];
    char dname[IFNAMSIZ];
    char address[INET6_ADDRSTRLEN];
    char *scopestr;
    int priority = 0;

    f = fopen("/proc/net/if_inet6", "r");
    if (f == NULL) {
        return -1;
    }

    while (19 == fscanf(f,
                        "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %*x %x %x %*x %s",
                        &ipv6[0],
                        &ipv6[1],
                        &ipv6[2],
                        &ipv6[3],
                        &ipv6[4],
                        &ipv6[5],
                        &ipv6[6],
                        &ipv6[7],
                        &ipv6[8],
                        &ipv6[9],
                        &ipv6[10],
                        &ipv6[11],
                        &ipv6[12],
                        &ipv6[13],
                        &ipv6[14],
                        &ipv6[15],
                        &prefix,
                        &scope,
                        dname)) {

        if (strcmp(interface, dname) != 0) {
            continue;
        }

        if (inet_ntop(AF_INET6, ipv6, address, sizeof(address)) == NULL) {
            continue;
        }

        switch (scope) {
        case IPV6_ADDR_GLOBAL:
            scopestr = "Global";
            if (priority < PRIORITY_GLOBAL)
            {
            	memcpy(ipv6_addr, address, INET6_ADDRSTRLEN);
            	priority = PRIORITY_GLOBAL;
            }
            break;
        case IPV6_ADDR_LINKLOCAL:
            scopestr = "Link";
            if (priority < PRIORITY_LINKLOCAL)
            {
            	memcpy(ipv6_addr, address, INET6_ADDRSTRLEN);
            	priority = PRIORITY_LINKLOCAL;
            }
            break;
        case IPV6_ADDR_SITELOCAL:
            scopestr = "Site";
            if (priority < PRIORITY_SITELOCAL)
            {
            	memcpy(ipv6_addr, address, INET6_ADDRSTRLEN);
            	priority = PRIORITY_SITELOCAL;
            }
            break;
        case IPV6_ADDR_COMPATv4:
            scopestr = "Compat";
            break;
        case IPV6_ADDR_LOOPBACK:
            scopestr = "Host";
            if (priority < PRIORITY_LOOPBACK)
            {
            	memcpy(ipv6_addr, address, INET6_ADDRSTRLEN);
            	priority = PRIORITY_LOOPBACK;
            }
            break;
        default:
            scopestr = "Unknown";
        }

        //printf("IPv6 address: %s, prefix: %d, scope: %s\n", address, prefix, scopestr);
    }

    fclose(f);
}


int 
main(int argc, char *argv[])
{
	unsigned char mac_addr[16] = {0};
	char str_mac_addr[32] = {0};

	char interface[16] = {0};

	if (argc < 2)
	{
		printf("Usage: ./test ifname\n");
		return -1;
	}
	strcpy(interface, argv[1]);
	if (get_hwaddr(-1, interface, mac_addr) < 0)
	{
		printf("get_hwaddr() error.\n");
		return -1;
	}

	snprintf(str_mac_addr, 32, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3],mac_addr[4],mac_addr[5]);

	printf("get_hwaddr() success, mac_addr: %s\n", str_mac_addr);

	char ipv4_addr[32] = {0};
	if (0 > get_ipv4_addr(-1, interface ,ipv4_addr))
	{
		printf("get_ipv4_addr() error.\n");
		return -1;
	}

	printf("get_ipv4_addr() success, ipv4_addr: %s\n", ipv4_addr);


	char ipv6_addr[64] = {0};
	if (0 > get_ipv6_addr(interface ,ipv6_addr))
	{
		printf("get_ipv6_addr() error.\n");
		return -1;
	}

	printf("get_ipv6_addr() success, ipv6_addr: %s\n", ipv6_addr);

	printf("INET6_ADDRSTRLEN= %d\n", INET6_ADDRSTRLEN);
	
	return 0;
}


