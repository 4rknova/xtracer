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

/*
	NOTE:
	The photon map class is based on Henrik Wann Jensen's implementation
	as presented in his book: Realistic Image Synthesis Using Photon Mapping.
*/

#ifndef XTRACER_PHOTONMAP_HPP_INCLUDED
#define XTRACER_PHOTONMAP_HPP_INCLUDED

#include <nmath/precision.h>

using NMath::scalar_t;

/*
	Photon.
*/
struct photon_t
{
	scalar_t position[3];		/* Photon position */
	scalar_t power[3];			/* Photon power */
	unsigned char theta, phi;	/* Incoming direction in spherical coordinates */
	unsigned char plane;		/* KD-Tree spltting plane */
};

typedef struct photon_t photon_t;


struct photon_cloud_t
{
	unsigned int max;
	unsigned int count;
	unsigned int flag_heap;

	scalar_t position[3];

	scalar_t *distance_squared;

	const photon_t **index;
};

typedef struct photon_cloud_t photon_cloud_t;

/*
	Photon map.
*/
class PhotonMap
{
	public:
		PhotonMap();
		~PhotonMap();

		// RETURN CODES:
		//	0. Everything went well.
		//	1. Failed to allocate memory.
		unsigned int init(unsigned int max_count);
		void release();

		void photon_dir(scalar_t *dir, const photon_t *p) const;	// Get the photon direction.

		void store(const scalar_t position[3],						// Photon's position.
				   const scalar_t power[3],							// Photon's power.
				   const scalar_t direction[3]);					// Photon's incident direction.

		void irradiance_estimate(scalar_t irradiance[3],			// Returned irradiance.
								 const scalar_t position[3],		// Surface position.
								 const scalar_t normal[3],			// Surface normal.
								 const scalar_t max_distance,		// Maximum distance in which to look for photons.
								 const unsigned int count) const;	// Number of photons to use.

		void locate_photons(photon_cloud_t * const np, const unsigned int index) const;

		void scale_power(const scalar_t factor);

	private:
		void calc_conversion_tables();		// Calculate the direction conversion tables.

		photon_t *m_photons;				// List of photons.

		unsigned int m_count_max;			// Number of maximum photons.
		unsigned int m_count_stored;		// Number of photons currently stored.
		unsigned int m_count_stored_half;	// Half number of photons currently stored.
		unsigned int m_prev_scale;

		// AABB
		scalar_t m_aabb_min[3];
		scalar_t m_aabb_max[3];

		// Precalculated direction conversion tables;
		scalar_t m_sintheta[256];
		scalar_t m_costheta[256];
		scalar_t m_sinphi[256];
		scalar_t m_cosphi[256];
};

#endif /* XTRACER_PHOTONMAP_HPP_INCLUDED */
