/*

    This file is part of xtracer.

	net.h
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


/*

	Network interfaces for distributed rendering.

	The server listens to a socket and waits for incoming connection.
	
	- The client connects and sends the signal "JOIN"
	- The server sends the signal "WLCM" and puts the client in the idle list

	- When the server wants to assign a task to a slave
	- The server sends a scene description and pair of 2d coordinates
	- Once the data is sent the client responds with "over"
	- The server puts the client in the workers list

	- When the client renders a pixel it sends the signal "prod" and the pixel value
	- The server responds with "kthx"

	- When the client completes the current assignment it sends the signal "done"
	- The server responds with "chill" and puts the client in the idle list
	

*/

#ifndef XTRACER_NET_H_INCLUDED
#define XTRACER_NET_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Supervisor */
#define XT_PROTO_SRV_PORT	6555	/* Default protocol port */
#define XT_PROTO_SRV_QUSZ	10		/* Default size of the request queue */

enum XT_MODE_NET
{
	XTRACER_NET_LOCAL,      /* Start as stand alone, no networking */
	XTRACER_NET_MASTER,     /* Start as master node */
	XTRACER_NET_SLAVE       /* Start as slave node */
};

struct netdata_t
{
	int port;
	const char host[1000];
};

/* Set the networking mode */
void net_set_mode(int mode);
/* Retrieve last status */
int net_get_status();
/* Initiatiate networking */
void net_init(int port, const char *host);
/* DeInit networking */
void net_deinit();
/* Master node function */
void *net_master(struct netdata_t *data);
/* Slave node function */
void *net_slave(struct netdata_t *data);	

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */

#endif /* XTRACER_NET_H_INCLUDED  */
