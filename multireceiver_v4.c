#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>


int set_sock_nonblock(int sockfd)
{
	fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	char ifname[32] = {0};
	const char multi_ip[] = "224.11.12.19";
	const int  multi_port = 12306;
	struct sockaddr_in local_addr;
	struct sockaddr_in mcast_addr;

	memset(&local_addr, 0, sizeof(local_addr));
	memset(&mcast_addr, 0, sizeof(mcast_addr));
	
	int sockfd = -1;
	//parameter parse and check
	if (argc != 2)
	{
		printf("Usage: ./multireceiver ifname\n");
		return -1;
	}

	strncpy(ifname, argv[1], sizeof(ifname)-1);
	
	if (1 != inet_pton(AF_INET, multi_ip, &mcast_addr.sin_addr))
	{
		printf("inet_pton() error. %d:%s\n", errno, strerror(errno));
		return -1;
	}
	
	//create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		printf("socket() error, %d:%s\n", errno, strerror(errno));
		return -1;
	}

	set_sock_nonblock(sockfd);

	int reuse = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	if (ret < 0)
	{
		printf("setsockopt() error, %d:%s\n", errno, strerror(errno));
		close(sockfd);
		return -1;
	}
	
	//set socket option
	struct ip_mreqn imr;
	memset(&imr, 0, sizeof(imr));
	imr.imr_multiaddr = mcast_addr.sin_addr;
	imr.imr_ifindex = if_nametoindex(ifname);
	ret = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			 (char *) &imr, (socklen_t)sizeof(struct ip_mreqn));
			 
	if (ret < 0)
	{
		printf("setsockopt() error, %d:%s\n", errno, strerror(errno));
		close(sockfd);
		return -1;
	}
	
	ret = setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, (socklen_t)strlen(ifname) + 1);
	if (ret < 0) {
		printf("can't bind to device %s. errno=%d. (try to run it as root)",
			    ifname, errno);
		close(sockfd);
		return -1;
	}
/*
	int offset = 6;
	ret = setsockopt(sockfd, IPPROTO_IPV6, IPV6_CHECKSUM, &offset, sizeof(offset));
	if (ret < 0) {
		printf("cant set IPV6_CHECKSUM IP option. errno=%d (%m)", errno);
		close(sockfd);
		return -1;
	}
*/	

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(multi_port);
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET6, link_local_ip, &local_addr.sin6_addr);
	//inet_pton(AF_INET6, site_local_ip, &local_addr.sin6_addr);

	ret = bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if (ret < 0)
	{
		printf("bind() error. %d:%s\n", errno, strerror(errno));
		close(sockfd);
		return -1;
	}
	
	char data_buf[512] = {0};
	ssize_t recv_len = 0;

	printf("multi_ip: %s\n", multi_ip);
	printf("multi_port: %d\n", multi_port);
	printf("init success, starting receive data...\n");
	//receive data
	while(1)
	{
		recv_len = recv(sockfd, data_buf, sizeof(data_buf), 0);
		if (recv_len > 0)
		{
			printf("recv data: %s\n", data_buf);
		}
		else
		{
			printf("no receive data\n");
		}

		sleep(1);
		
	}
	
	return 0;
}


