/*

    This file is part of xtracer.

	net.c
	Distributed rendering

	Copyright (C) 2010, 2011
	Papadopoulos Nikolaos

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General
	Public License along with this library; if not, write to the
	Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA 02110-1301 USA

*/

#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "err.h"

int producer (int port, const char *proto)
{
	struct protoent *ptrp;	/* pointer to a protocol table entry */
	struct sockaddr_in sad;	/* structure to hold server's address */
	struct sockaddr_in cad;	/* structure to hold client's address */
	int sd1, sd2;			/* socket descriptors */
	socklen_t alen;			/* address length */
	char buf[1000];			/* data buffer */

	long lRecvd, lEchoed;	/* for statistics */
	int iRecvdp, iEchoedp; 	/* for statistics */
	iRecvdp = 0;
	iEchoedp = 0;
	int visits = 0;         /* Active connections count */

	
	/* Setup the server socketaddr infrastructure */
	memset((char*)&sad, 0, sizeof(sad)); 	/* clear the socketaddr structure */
	sad.sin_family = AF_INET;				/* set the family to internet */
	sad.sin_addr.s_addr = INADDR_ANY;		/* set the local IP address */
	
	/* Check for illegal port value */
	if(port > 0)
	{
		sad.sin_port = htons((u_short)port);
	}
	else
	{
		fprintf(stderr, "Bad port number %i\n", port);
		return  XTRACER_STATUS_NET_INVALID_PORT;
	}

	printf("Listening on port: %d..\n", port);

	/* Determine which protocol to use */
	int bIsTCP = !strcmp(proto, "tcp");
	int bIsUDP = !strcmp(proto, "udp");

	if(!bIsTCP && !bIsUDP)
	{
		fprintf(stderr, "Invalid protocol specified %s.\n", proto);
		return  XTRACER_STATUS_NET_INVALID_PROTO;
	}

	printf("Using %s protocol..\n", proto);

	/* Map protocol name to protocol number */
	if(((int)(ptrp = getprotobyname(XT_PROTO_SRV_STRN))) == 0)
	{
		fprintf(stderr, "Cannot map protocol to protocol number\n");
		return XTRACER_STATUS_NET_FAILURE_PROTOMAP;
	}

	/* 
		Create a socket, depending on the protocol we are using.
	
		TCP: spcl_stream
		UDP: datagram
	*/

	if (bIsTCP)
	{
		sd1 = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);		/* TCP */
	}
	else
	{
		sd1 = socket(AF_INET, SOCK_DGRAM, ptrp->p_proto);		/* UDP */
	}
	
	/* Check for errors */
	if (sd1 < 0)
	{
		fprintf(stderr, "Socket creation failed\n");
		return XTRACER_STATUS_NET_FAILURE_SOCKET_C;
	}
	
	/* Bind a local access to the socket and detect errors. */
	if(bind(sd1, (struct sockaddr *)&sad, sizeof(sad)) < 0)
	{
	 	fprintf(stderr, "Cannot bind socket\n");
		return XTRACER_STATUS_NET_FAILURE_BIND;
	}

	/* Specify size of the request queue. Only call the listen function for tcp server */
	if(bIsTCP && listen(sd1, XT_PROTO_SRV_QUSZ) < 0)
	{
		fprintf(stderr, "Cannot listen\n");
		return XTRACER_STATUS_NET_FAILURE_LISTEN;
	}

	/* Server loop */
	int n; /* Temporary variable */

	while (bIsTCP)	/* TCP */
	{
		alen = sizeof(cad);
		if((sd2 = accept(sd1, (struct sockaddr *)&cad, &alen)) < 0)
		{
			fprintf(stderr, "Failed to accept\n");
			return XTRACER_STATUS_NET_FAILURE_ACCEPT;
		}
		iRecvdp = 0;
		iEchoedp = 0;
		visits++;
		printf("VISITS: %d RECVD: %d ECHOED: %d \n", visits, iRecvdp, iEchoedp);
		
		/* receive data from client */		
		n = recv(sd2, buf, sizeof(buf), 0);
		lRecvd=(long)n;
		lEchoed=0l;

		while(n > 0)
		{
			if(!strcmp(buf, "join"))
			{
				lEchoed += (long)send(sd2, "wlcm", n, 0);
			}
			else if(!strcmp(buf, "done"))
			{
				lEchoed += (long)send(sd2, "chill", n, 0);
			}

			iRecvdp++;
			iEchoedp++;
			printf("Client says: %s", buf);
			n = recv(sd2, buf, sizeof(buf), 0); /* Check if there is more */
			lRecvd+=(long)n;
		}

		/* Output statistics */
		printf("Stats\n");
		printf("received: %08ld bits / %08d packets\n", lRecvd, iRecvdp);
		printf("  echoed: %08ld bits / %08d packets\n", lEchoed, iEchoedp);

		shutdown(sd2, 2);
	}

	while(!bIsTCP)
	{
		alen = sizeof(cad);

		n = recvfrom(sd1, buf, sizeof(buf), 0, (struct sockaddr*)&cad, &alen);

		while(n > 0)
		{
			/* Check if there is more */
			printf("Client says: %s", buf);
			n = recvfrom(sd1, buf, sizeof(buf), 0, (struct sockaddr*)&cad, &alen);			

			/* Output statistics */
			printf("Stats\n");
			printf("received: %08d bits / 1 packets\n", n);
			printf("  echoed: %08d bits / 1 packets\n", n);
		}
	}

	/* Clean up */
	/* We can't close the main listening socket for udp connection until the very end. */
	if(!bIsTCP) shutdown(sd1, 2);

	return XTRACER_STATUS_OK;
}

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */
