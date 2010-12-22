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
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <pthread.h>

#include "err.h"

/*
	GLOBAL VARIABLES
*/

pthread_t g_net_thread;    				/* Thread id */
int g_net_mode = XTRACER_NET_LOCAL;		/* Net mode */
int g_net_status = XTRACER_STATUS_OK;	/* Used to track errors */
int g_net_hang_up = 0;					/* Flag used to terminate connections */

void net_set_mode(int mode)
{
	g_net_mode = mode;
}

int net_get_status()
{
	return g_net_status;
}

void net_init(int port, const char *host)
{
	if (g_net_mode != XTRACER_NET_LOCAL)
	{
		struct netdata_t data;
		data.port = port;

		/* Zero the data strings */

		memset(&data.host, 0, 1000);

		int hostln = strlen(host);
		if(hostln < 1000)
		{   
			memcpy(&data.host, host, hostln);
		}   
		else
		{
			fprintf(stderr, "Invalid host\n");
		}

		printf("Initiating networking [udp %s:%i]..\n", data.host, data.port);

		switch(g_net_mode)
		{       
			case XTRACER_NET_MASTER:
				printf("Starting up master node...\n");
				pthread_create(&g_net_thread, 0, (void *)net_master, &data);
				break;
			case XTRACER_NET_SLAVE:
				printf("Starting up slave node...\n");
				pthread_create(&g_net_thread, 0, (void *)net_slave, &data);
		}
	} 
}

void net_deinit()
{
	printf("Shutting down networking..\n");
	g_net_hang_up = 1;
	if(g_net_mode != XTRACER_NET_LOCAL)
	{
		pthread_join(g_net_thread, NULL);
	}
}

#define XT_NET_BUFFER_SZ 1000

void *net_master(struct netdata_t *data)
{
	/* Message buffer */
	char buffer[XT_NET_BUFFER_SZ];

	/* Create socket */
	int socketId = socket(AF_INET, SOCK_DGRAM, 0) ;

	/* Check for errors */
	if(socketId == -1)
	{
		fprintf(stderr, "Failed to create socket\n");
		g_net_status = XTRACER_STATUS_NET_FAILURE_SOCKET_C;
		return NULL;
	}

	/* Make the socket non-blocking */
	int x = fcntl(socketId, F_GETFL, 0);						/* Get socket flags */
	fcntl(socketId, F_SETFL, x | MSG_DONTWAIT | O_NONBLOCK);	/* Add non-blocking flag */
    
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr *serverAddrCast = (struct sockaddr *) &serverAddr;
	struct sockaddr *clientAddrCast = (struct sockaddr *) &clientAddr;
	
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(data->port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	/* Associate process with port */
	bind(socketId, serverAddrCast, sizeof(struct sockaddr));

	while (!g_net_hang_up)
	{
		char response[XT_NET_BUFFER_SZ];

		/* Reset the buffers */
		memset(buffer, 0, XT_NET_BUFFER_SZ);
		memset(response, 0, XT_NET_BUFFER_SZ);


		/* Receive */
		unsigned int size = sizeof(clientAddr);
		int mlen = recvfrom(socketId, buffer, XT_NET_BUFFER_SZ, 0, clientAddrCast, &size);

		if(mlen > 0) printf("<- %s %i: %s\n", (char *)inet_ntoa(clientAddr.sin_addr),  mlen, buffer);
		
		if(!strcmp(buffer, "JOIN"))
		{
			char *msg="WLCM";
			memcpy(response, msg, strlen(msg));
			printf("[%s] requested to join\n", (char *)inet_ntoa(clientAddr.sin_addr));
			/* put client in workers list as idle */
		}
		else if(!strcmp(buffer, "done"))
		{

		}
		
		/* Print the response if any, and reply */
		int outl = strlen(response);
		if(outl)
		{
			printf("-> %s %i: %s\n", (char *)inet_ntoa(clientAddr.sin_addr), outl, response);
			sendto(socketId, response, XT_NET_BUFFER_SZ, 0, clientAddrCast, size);
		}
	}
	
	/* Terminate */
	printf("Closing connection..\n");
	pthread_exit(0);
	g_net_status = XTRACER_STATUS_OK;
}

void *net_slave(struct netdata_t *data)
{
	/* Message buffer */
	char buffer[XT_NET_BUFFER_SZ];
	
	/* Create socket */
	int socketId = socket(AF_INET, SOCK_DGRAM, 0) ;

	/* Check for errors */
	if(socketId == -1)
	{
		fprintf(stderr, "Failed to create socket\n");
		g_net_status = XTRACER_STATUS_NET_FAILURE_SOCKET_C;
		return NULL;
	}

	/* Make the socket non-blocking */
	int x = fcntl(socketId, F_GETFL, 0);                        /* Get socket flags */
	fcntl(socketId, F_SETFL, x | MSG_DONTWAIT | O_NONBLOCK);    /* Add non-blocking flag */

	/* Server address */
	struct sockaddr_in serverAddr;
	struct sockaddr *serverAddrCast = (struct sockaddr *) &serverAddr;
	
	/* Zero the server address and then set it up */
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(data->port);

	/* Resolved server address */
	struct hostent *hp = gethostbyname(data->host);
	struct in_addr **host = (struct in_addr **)hp->h_addr_list;
	serverAddr.sin_addr = **host;

	/* Print the resolved addresses */
	{
		struct in_addr  **pptr;
		pptr = (struct in_addr **)hp->h_addr_list;
		while( *pptr != NULL )
		{
			printf("Resolved master node's hostname: %s = %s\n", data->host, inet_ntoa(**(pptr++)));  
		}
	}
	
	unsigned int size = sizeof(serverAddr);

	int iIsIdle = 1;

	while (!g_net_hang_up)
	{
		char response[XT_NET_BUFFER_SZ];
		
		/* Reset the buffers */
		memset(buffer, 0, XT_NET_BUFFER_SZ);
		memset(response, 0, XT_NET_BUFFER_SZ);

		/* Receive */
		int mlen = recvfrom(socketId, buffer, XT_NET_BUFFER_SZ, 0, serverAddrCast, &size);
		if(mlen > 0) printf("<- %s %i: %s\n", inet_ntoa(serverAddr.sin_addr),  mlen, buffer);
		
		if(iIsIdle)
		{
			char *msg="JOIN";
			memcpy(response, msg, strlen(msg));
			iIsIdle = 0;
		}
		
		int outl = strlen(response);
		if(outl)
		{
			printf("-> %s %i: %s\n", (char *)inet_ntoa(serverAddr.sin_addr),  outl, response);
			int res = sendto(socketId, response, outl, 0, serverAddrCast, size);
			
			if(res == -1)
			{
				fprintf(stderr, "Message was not send");
			}
		}
	}


	pthread_exit(0);
	g_net_status = XTRACER_STATUS_OK;
}

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */

