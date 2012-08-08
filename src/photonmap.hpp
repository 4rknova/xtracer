/*

    This file is part of xtracer.

    photonmap.hpp
    Photon map

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef XTRACER_PHOTONMAP_HPP_INCLUDED
#define XTRACER_PHOTONMAP_HPP_INCLUDED

#include <nmath/precision.h>

/*
	Photon.
*/
struct photon_t
{
	scalar_t position[3];		/* Photon position */
	scalar_t power[3];			/* Photon power */
	unsigned char theta, phi	/* Incoming direction in spherical coordinates */
	unsigned char plane;		/* KD-Tree spltting plane */
};

typedef struct photon_t photon_t;

/*
	Photon probe.
*/
struct photon_probe_t
{
};

typedef struct photon_probe_t photon_probe_t;

/*
	Photon map.
*/
class PhotonMap
{
	public:
		PhotonMap(

};

#endif /* XTRACER_PHOTONMAP_HPP_INCLUDED */
