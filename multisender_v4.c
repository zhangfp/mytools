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
	
	int sockfd = -1;
	//parameter parse and check
	if (argc != 2)
	{
		printf("Usage: ./multisender ifname\n");
		return -1;
	}

	strcpy(ifname, argv[1]);
	
	//create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		printf("socket() error, %d:%s\n", errno, strerror(errno));
		return -1;
	}

	int val = 0;
	socklen_t len = sizeof(val);
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &val, len) < 0)
	{
		printf("vrrp set send socket buffer size error %d", errno);
		close(sockfd);
		return -1;
	}

	struct ip_mreqn imr;
	memset(&imr, 0, sizeof(imr));
	imr.imr_ifindex = (int)if_nametoindex(ifname);
	ret = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &imr, sizeof(imr));
	if (ret < 0) {
		printf("cant set IP_MULTICAST_IF IP option. errno=%d (%m)\n", errno);
		close(sockfd);
		return -1;
	}

	int loopv4 = 0;
	ret = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopv4, sizeof(loopv4));

	if (ret < 0) {
		printf("cant set IP_MULTICAST_LOOP IP option. errno=%d (%m)", errno);
		close(sockfd);
		return -1;
	}

	int ttl = 5;
	ret = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	if (ret < 0) {
		printf("cant set IP_MULTICAST_TTL IP option. errno=%d (%m)", errno);
		close(sockfd);
		return -1;
	}
	
	char data_buf[] = "hello world.";
	struct sockaddr_in mcast_addr;
	memset(&mcast_addr, 0, sizeof(struct sockaddr_in));
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons(multi_port);

	if (1 != inet_pton(AF_INET, multi_ip, &mcast_addr.sin_addr))
	{
		printf("inet_pton() error. %d:%s\n", errno, strerror(errno));
		return -1;
	}

	printf("multi_ip: %s\n", multi_ip);
	printf("multi_port: %d\n", multi_port);
	printf("init success, starting send data...\n");
	//send data
	while(1)
	{
		if (sendto(sockfd, data_buf, strlen(data_buf), 0, (struct sockaddr *)&mcast_addr, sizeof(mcast_addr)) < 0)
		{
			printf("sendto() error. %d:%s\n", errno, strerror(errno));
			return -1;
		}
		else
		{
			printf("send msg: %s\n", data_buf);
		}
		
		sleep(1);
	}
	
	return 0;
}



