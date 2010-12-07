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
#define XT_PROTO_SRV_STRN	"tcp"	/* Default protocol to use */
#define XT_PROTO_SRV_PORT	68555	/* Default protocol port */
#define XT_PROTO_SRV_QUSZ	10		/* Default size of the request queue */

int producer(int port, const char *proto);
int consumer(const char *host, int port, const char *proto);

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */

#endif /* XTRACER_NET_H_INCLUDED  */
