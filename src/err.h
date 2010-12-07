/*

    This file is part of xtracer.

   	err.h
    Xtracer error codes

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

#define XTRACER_STATUS_OK						0x00
#define XTRACER_STATUS_INVALID_CLI_ARGUMENT		0x01

/* Net errors */
#define XTRACER_STATUS_NET_INVALID_PORT			0xF0
#define XTRACER_STATUS_NET_INVALID_PROTO		0xF1
#define XTRACER_STATUS_NET_FAILURE_PROTOMAP		0xF2
#define XTRACER_STATUS_NET_FAILURE_SOCKET_C		0xF3
#define XTRACER_STATUS_NET_FAILURE_BIND			0xF4
#define XTRACER_STATUS_NET_FAILURE_LISTEN		0xF5
#define XTRACER_STATUS_NET_FAILURE_ACCEPT		0xF6

