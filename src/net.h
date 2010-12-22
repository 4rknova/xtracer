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

#ifndef XTRACER_NET_H_INCLUDED
#define XTRACER_NET_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
	COMMUNICATION PROTOCOL
	
	SPECIFICATION

		Communication is connectionless ( UDP ).
		A client can be in either IDLE or WORK mode

		All the message values are treated as big endians

		In every message:

			BB				BBBB			BBBB 				B...B
			2byte command	4byte id 		4byte data length	Nbyte data
							0 for server

		Handshake ( applies when a client is in idle mode  )
		- 	The client informs the server that he is in idle mode by sending an IDLE message and a benchmark value indicating
			his capabilities. If no response is received after the time out then the IDLE message is resent.
		- 	The server generates an id that will be used in conjuction with the client's ip address to identify the client.
			The server responds with AKNL <id>.
		-	Upon id receipt, the client sents back a CNFR <id> message to ensure the integrity of the id.
		-	The server receives the CNFR request, checks the registered client records and responds with either WLCM if the id is correct,
			or REST otherwise.
		-	Upon receipt of a REST 1 message, the client goes into idle mode. While the connection fails, the handshake process is repeated.

		Scene assignment
			By default all the rendering is to be performed by the server. The benchmark values are used to
			segment the screen to a set of areas.

		-	The server sends a SCEN message, a checksum and an ascii string to a client to assign a scene file.
		-	The client checks the message's integrity and responds with RCVD on success or FALR on failure.
		
		Task assignment
		-	The se

		Connection test
		-	The server sends a IDNT message to each client in his task list
		-	The client responds with a PRSN message and a status mode
				0: working
				1: idle
		-	The server then checks if the status is correct.
		-	If the server receives no answer or an invalid answer then he send a REST message to the client,
			drops the entry from the task list and reassigns it.

		Termination
		-	The server sends a REST 0 message. Awaits for response and removes the client from his list.
			At the end and after a certain timeout it reports any clients that did not respond and terminates as well.
		-	The clients reply with a KBYE message and terminate.
*/

/*
	CLIENT STATES
*/
#define XT_NET_SLAVE_STATE_IDLE	0
#define XT_NET_SLAVE_STATE_WORK	1

/*
	MESSAGES
	The message values are selected in such a way that permits them 
	to be efficiently decoded with bit masking.
*/
#define	XT_NET_MASTR_PRMSG_AKNL	0x005A
#define XT_NET_MASTR_PRMSG_WLCM	0x0051
#define XT_NET_MASTR_PRMSG_REST	0x00FF

#define XT_NET_MASTR_PRMSG_SCEN	0x00A1

#define	XT_NET_SLAVE_PRMSG_IDLE	0x805A
#define XT_NET_SLAVE_PRMSG_CNFR	0x8052

#define XT_NET_SLAVE_PRMSG_RCVD 0x80A1
#define XT_NET_SLAVE_PRMSG_FALR 0x80A0

#define XT_NET_SLAVE_PRMSG_KBYE	0x80FF

/*
	ENVIRONMENT SETUP 
*/
#define XT_NET_PROT_PORT		6555	/* Default protocol port */
#define XT_NET_PROT_MASTR_TIMER	5		/* Default time out in seconds ( master ) */
#define XT_NET_PROT_SLAVE_TIMER	3		/* Default time out in seconds ( slave ) */

/*
	NODE MODES
*/
enum XT_MODE_NET
{
	XTRACER_NET_LOCAL,      /* Start as stand alone, no networking */
	XTRACER_NET_MASTER,     /* Start as master node */
	XTRACER_NET_SLAVE       /* Start as slave node */
};

/* Default net mode */
#define XTRACER_DEFAULT_MODE_NET XTRACER_NET_LOCAL

/*
	Internal structure, used in network thread startup
*/
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
/* Master node worker */
void *net_master(struct netdata_t *data);
/* Slave node worker */
void *net_slave(struct netdata_t *data);	

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */

#endif /* XTRACER_NET_H_INCLUDED  */
