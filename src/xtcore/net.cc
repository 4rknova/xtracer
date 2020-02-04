#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cpp-httplib/httplib.h>
#include "log.h"
#include "macro.h"

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

    return 0;
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
    UNUSED(url);
    UNUSED(path);
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
    return 0;
}

//===========================================================================

int setup_socket(void) {
  int sck = socket(AF_INET, SOCK_DGRAM, 0);

  int bcast = 1;
  if (0 > setsockopt(sck, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast))) {
    return -1;
  }

  struct timeval tv = {
    .tv_sec = 0,
    .tv_usec = 100 * 1000,
  };

  if (0 > setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
    fprintf(stderr, "Unable to 'setsockopt': %s\n", strerror(errno));
    return -2;
  }

  return sck;
}

void * get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Bind to the specified host and port. Host can be a broadcast address. */
int bind_socket(
    char * host, /* "192.168.1.255" */
    char * port  /* "5000"          */
){
    struct addrinfo * info;
    struct addrinfo hints = {
        .ai_family   = AF_INET,
        .ai_socktype = SOCK_DGRAM
    };
    int sck;
    int err;

    if (0 != (err = getaddrinfo(host, port, &hints, &info))) {
        return -1;
    }

    struct addrinfo * p;
    for (p = info; p != NULL; p = p->ai_next) {
        if (0 > (sck = socket(p->ai_family, p->ai_socktype, p->ai_protocol))) {
            continue;
        }

        if (0 > bind(sck, p->ai_addr, p->ai_addrlen)) {
            close(sck);
            continue;
        }

        break;
    }

    if (NULL == p) {
        return -2;
    }

    freeaddrinfo(info);

    int bcast = 1;
    if (0 > (err = setsockopt(sck, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)))) {
        return -3;
    }

    return sck;
}

/* Given a 'listening socket' and a 'local socket', loop and respond to
 * messages. */
int listen_loop(
    int listen_sck, /* The socket we wait for detection messages on. */
    int local_sck) /* The socket we respond to detection messages on. */
{
    uint8_t reply[] = "hello";
    uint8_t buffer[128] = {0};
    struct sockaddr_storage from;
    socklen_t from_len;

    while (1) {
        from_len = sizeof(from);
        int len = recvfrom(listen_sck, (void*)buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &from_len);

        if (0 < len) {
            printf("Recevied message '%s'\n", buffer);

            len = sendto(local_sck, (void*)reply, sizeof(reply), 0, (struct sockaddr *)&from, from_len);
            if (errno) {
                fprintf(stderr, "Unable to 'sendto': %s\n", strerror(errno));
                return -1;
            }

            printf("Replied with %d bytes.\n", len);
        }
    }

    return 0;
}

int listen()
{
    int listen_sck;
    int local_sck;

    char * bind_addr = "127.0.0.1";
    char * bind_port = "5000";


    if (0 > (local_sck = setup_socket())) {
        fprintf(stderr, "Failed to setup local socket: %d\n", local_sck);
        return -1;
    }

    if (0 > (listen_sck = bind_socket(bind_addr, bind_port))) {
        fprintf(stderr, "Failed to bind to %s:%s. Error: %d\n", bind_addr, bind_port, listen_sck);
        return -1;
    }

    return listen_loop(listen_sck, local_sck);
}

    } /* namespace network */
} /* namespace xtcore */
