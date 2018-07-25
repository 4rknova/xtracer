#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cpp-httplib/httplib.h>
#include "log.h"

#include <iostream>

#define MAXSTRL 255     /* Max string length */
#define IP      "255.255.255.255"
#define PORT    8080

namespace xtcore {
    namespace network {

int listen(bool *done)
{
    int sock;                         /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast Address */
    unsigned short broadcastPort;     /* Port */
    char recvString[MAXSTRL+1];       /* Buffer for received string */
    int recvStringLen;                /* Length of received string */

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) return 3;

    /* Construct bind structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
    broadcastAddr.sin_port = htons(PORT);               /* Broadcast port */

    /* Bind to the broadcast port */
    if (bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0) return 2;

    while(!*done) {
        /* Receive a single datagram from the server */
        if ((recvStringLen = recvfrom(sock, recvString, MAXSTRL, 0, NULL, 0)) < 0) return 1;
        recvString[recvStringLen] = '\0';

        printf("Received: %s\n", recvString);    /* Print the received string */
    }

    close(sock);
}

int broadcast()
{
    const char *msg = "xtracer instance";

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
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
        (void *) &broadcastPermission,
        sizeof(broadcastPermission)) < 0) {
        return 2;
	}

    /* Construct local address structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = inet_addr(IP);      /* Broadcast IP address */
    broadcastAddr.sin_port = htons(PORT);               /* Broadcast port */

    messageLen = strlen(msg);
    if (sendto(sock, msg, messageLen + 1, 0,
        (struct sockaddr *) &broadcastAddr,
        sizeof(broadcastAddr)) != messageLen) {
		return 3;
	}

    close(sock);

	return 0;
}

int wget(const char *url, const char *path)
{

    {
        httplib::Client cli("www.4rknova.com", 80);
        auto res = cli.Get("/main.html");
        if (res) {
            std::cout << res->status << std::endl;
            std::cout << res->get_header_value("Content-Type") << std::endl;
            std::cout << res->body << std::endl;
        }
    }
    {
        httplib::SSLClient cli("gist.githubusercontent.com", 443, 5);
        auto res = cli.Get("/noonat/1131091/raw/3ef186720b1bb6e704c46aebf57a42677d1638af/cube.obj");
        if (res) {
            std::cout << res->status << std::endl;
            std::cout << res->get_header_value("Content-Type") << std::endl;
            std::cout << res->body.size() << std::endl;
            std::cout << res->body << std::endl;
        }
    }
}

    } /* namespace network */
} /* namespace xtcore */
