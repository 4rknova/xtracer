#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define IP   "255.255.255.255"
#define PORT 8080

namespace xtcore {
    namespace network {

int broadcast(const char *message)
{
    int sock;                         /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast address */
    int broadcastPermission;          /* Socket opt to set permission to broadcast */
    unsigned int messageLen;    	  /* Length of string to broadcast */

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        return 1;
	}

    /* Set socket to allow broadcast */
    broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,
          sizeof(broadcastPermission)) < 0) {
        return 2;
	}

    /* Construct local address structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = inet_addr(IP);      /* Broadcast IP address */
    broadcastAddr.sin_port = htons(PORT);               /* Broadcast port */

    messageLen = strlen(message);

    /* Broadcast message in datagram to clients every 3 seconds*/
    if (sendto(sock, message, messageLen, 0, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) != messageLen) {
		return 3;
	}

	return 0;
}

    } /* namespace network */
} /* namespace xtcore */
