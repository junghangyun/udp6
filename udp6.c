#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>


static void DoRxOperation(uint16_t port);
static void DoTxOperation(uint8_t *destip, uint16_t port);

uint8_t buf[5] = { 0x00, 0x01, 0x02, 0x03, 0x04 };

static void usage(char *cmd)
{
    printf("Usage:\n");
    printf("  %s rx <portnum>\n", cmd);
    printf("  %s tx <destip> <dsetport>\n", cmd);
}

int main(int argc, char *argv[])
{
    uint16_t port;
    uint8_t destip[16];

    if(!memcmp(argv[1], "rx", 2)) {

        if(argc < 3) {
            usage(argv[0]);
            return -1;
        }
        port = (uint16_t)strtoul(argv[2], NULL, 10);

        DoRxOperation(port);

    } else if(!memcmp(argv[1], "tx", 2)) {

        if (argc < 4) {
            usage(argv[0]);
            return -1;
        }
		inet_pton(AF_INET6, argv[2], destip);
        port = (uint16_t)strtoul(argv[3], NULL, 10);

        DoTxOperation(destip, port);

    } else {
    
        usage(argv[0]);
        return -1;

    }
}


static void DoRxOperation(uint16_t port) 
{
    int sock, ret, i;
    uint32_t udp_rxcnt = 0;
    struct timeval tv;
    struct sockaddr_in6 addr;
    socklen_t addrlen = sizeof(addr);
    uint8_t ipstr[50];
    
    printf("Starting rx operation - port: %u\n", port);

    /* 
     * 소켓 오픈 
     */
	sock = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("Fail to socket() ");
		return;
	}

	/* 소켓 타임아웃 설정 - 쓰레드 종료를 파악하기 위해 */
	// tv.tv_sec = 3;
	// tv.tv_usec = 0;
	// if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))
	// 		< 0) {
	// 	perror("Fail to setsockopt(RCVTIMEO) ");
	// 	close(sock);
	// 	return;
	// }

	/* 소켓 바인드 */
	addrlen = sizeof(struct sockaddr_in6);
	memset(&addr, 0, addrlen);
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_any;
	addr.sin6_port = htons(port);
	ret = bind(sock, (struct sockaddr *)&addr, addrlen);
	if (ret < 0) {
		perror("Fail to bind socket ");
		close(sock);
		return;
    }
    
    /* UDP 패킷을 수신한다. */
	do {
		ret = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addrlen);
		if (ret < 0) {
			perror("Fail to receive UDP packet ");
			continue;
		} else {
            udp_rxcnt++;
            inet_ntop(AF_INET6, addr.sin6_addr.s6_addr, ipstr, sizeof(ipstr));
			printf("%d-th UDP packet is received from %s (Len: %d)\n",
                        udp_rxcnt, ipstr, ret);
            for(i = 0; i < ret; i++) {
                if((i!=0) && (i%16==0)) {
                    printf("\n");
                }
                printf("%02X ", buf[i]);
            }
            printf("\n");
	    }
	} while(1);

}

static void DoTxOperation(uint8_t *destip, uint16_t port)
{
	int sock, ret;
    uint8_t destipstr[50];
    uint32_t udp_txcnt = 0;
	struct sockaddr_in6 remote;
    struct ifreq ifr;

    inet_ntop(AF_INET6, destip, destipstr, sizeof(destipstr));
    printf("Starting tx operation - destip : %s, port: %u\n", 
        destipstr, port);
        
	/* 소켓 오픈 */
	sock = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("Fail to socket() ");
		return;
	}

	/* 목적지 정보 설정 */
	memset(&remote, 0, sizeof(struct sockaddr_in6));
	remote.sin6_family = AF_INET6;
	remote.sin6_port = htons(port);
	memcpy(remote.sin6_addr.s6_addr, destip, 16);

	/* 소켓을 wave0 인터페이스에 bind() */
    memset(ifr.ifr_name, 0, IFNAMSIZ);
	snprintf(ifr.ifr_name, IFNAMSIZ, "wave0");
	ret = ioctl(sock, SIOGIFINDEX, &ifr);
	if (ret < 0) {
		perror("Fail to ioctl(SIOGIFINDEX) ");
		close(sock);
		return;
	}
	printf("Success to ioctl(sock) - ifindex = %d\n", ifr.ifr_ifindex);
	if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
		perror("setsockopt(txsock) failed to bind to interface ");
		close(sock);
		return;
	}
	printf("Success to setsockopt(SO_BINDTODEVICE)\n");

	do {

		/* UDP 패킷을 전송한다. */
		ret = sendto(sock, buf, sizeof(buf), 0,
				(struct sockaddr *) &remote, sizeof(remote));
	    if(ret != sizeof(buf)) {
			printf("Fail to send %d-bytes UDP packet - %s.\n",
					sizeof(buf), strerror(errno));
	    } else {
            udp_txcnt++;
            printf("Success to send %d-th UDP packet (Len: %d)\n",
						udp_txcnt, sizeof(buf));
        }
        
        sleep(1);

	} while(1);
	/*----------------------------------------------------------------------------------*/

}
