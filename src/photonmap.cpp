#include <cstdlib>
#include "photonmap.hpp"

PhotonMap::PhotonMap()
	: m_photons(NULL),
	  m_count_max(0),
	  m_count_stored(0),
	  m_count_stored_half(0),
	  m_prev_scale(1)
{
	// Calculate the direction conversion tables.
	calc_conversion_tables();
}

PhotonMap::~PhotonMap()
{
	release();
}

unsigned int PhotonMap::init(unsigned int max_count)
{
	// Release data.
	release();

	m_photons = (photon_t*) malloc(sizeof(photon_t) * (max_count + 1));

	if (m_photons == NULL) {
		return 1;
	}

	m_count_max = max_count;

	// Reset the bounding box.
	m_aabb_min[0] = m_aabb_min[1] = m_aabb_min[2] =  INFINITY;
	m_aabb_max[0] = m_aabb_max[1] = m_aabb_max[2] = -INFINITY;

	return 0;
}

void PhotonMap::release()
{
	free(m_photons);
}

void PhotonMap::calc_conversion_tables()
{
	for (unsigned int i = 0; i < 256; ++i) {
		scalar_t angle = (scalar_t)(i) * (1.0 / 256.0) * M_PI;

		m_sintheta[i] = nmath_sin(angle);
		m_costheta[i] = nmath_cos(angle);
		m_sinphi[i]   = nmath_sin(angle);
		m_cosphi[i]   = nmath_cos(angle);
	}
}

void PhotonMap::photon_dir(scalar_t *dir, const photon_t *p) const
{
	dir[0] = m_sintheta[p->theta] * m_cosphi[p->phi];
	dir[1] = m_sintheta[p->theta] * m_sinphi[p->phi];
	dir[2] = m_costheta[p->theta];
}

void PhotonMap::store(const scalar_t position[3], const scalar_t power[3], const scalar_t direction[3])
{
	if ( m_count_stored > m_count_max) {
		return;
	}

	m_count_stored++;
	photon_t * const node = &m_photons[m_count_stored];

	for (unsigned int i = 0; i < 3; i++) {
		node->position[i] = position[i];

		if (node->position[i] < m_aabb_min[i]) {
			m_aabb_min[i] = node->position[i];
		}

		if (node->position[i] > m_aabb_max[i]) {
			m_aabb_max[i] = node->position[i];
		}

		node->power[i] = power[i];
	}

	int theta = int (nmath_acos(direction[2]) * (256.0 / M_PI));

	if (theta > 255) {
		node->theta = 255;
	}
	else {
		node->theta = (unsigned char) theta;
	}

	int phi = int (nmath_atan2(direction[1], direction[0]) *  (256.0 / (2.0 * M_PI)));

	if (phi > 255) {
		node->phi = 255;
	}
	else if (phi < 0) {
		node->phi = (unsigned char) (phi + 256);
	}
	else {
		node->phi = (unsigned char) phi;
	}
}

void PhotonMap::irradiance_estimate(scalar_t irradiance[3],
	const scalar_t position[3], const scalar_t normal[3], 
	const scalar_t max_distance, const unsigned int count) const
{
	// Zero out the irradiance.
	irradiance[0] = irradiance[1] = irradiance[2] = 0.0;

	photon_cloud_t np;

	np.distance_squared = (scalar_t *) malloc(sizeof(scalar_t) * (count + 1));
	np.index = (const photon_t **) malloc(sizeof(photon_t *) * (count + 1));
	np.position[0] = position[0];
	np.position[1] = position[1];
	np.position[2] = position[2];
	np.max = count;
	np.count = 0;
	np.flag_heap = 0;
	np.distance_squared[0] = max_distance * max_distance;

	// locate the nearest photons.
	locate_photons(&np, 1);

	// if less than 8 photons return.
	if (np.count < 8) {
		return;
	}

	scalar_t pdir[3];

	// Sum irradiance from all photons.
	for (unsigned int i = 1; i <= np.count; ++i) {
		const photon_t *p = np.index[i];

		// The following check is not needed if the scene does not contain any
		// thin surfaces (such as planes).
		photon_dir(pdir, p);

		if ((pdir[0] * normal[0] + pdir[1] * normal[1] + pdir[2] * normal[2]) < 0.0f) {
			irradiance[0] += p->power[0];
			irradiance[1] += p->power[1];
			irradiance[2] += p->power[2];
		}
	}

	const scalar_t density = (1.0f / M_PI) / (np.distance_squared[0]); // Density estimate.

	irradiance[0] *= density;
	irradiance[1] *= density;
	irradiance[2] *= density;

	// Release allocated memory.
	free(np.distance_squared);
	free(np.index);
}

void PhotonMap::locate_photons(photon_cloud_t * const np, const unsigned int index) const
{
	const photon_t *p = &m_photons[index];
	scalar_t dist1;

	if (index < m_count_stored_half) {
		dist1 = np->position[p->plane] - p->position[p->plane];

		if (dist1 > 0.0) {
			// Search the right plane first.
			locate_photons(np,  2 * index + 1);

			if (dist1 * dist1 < np->distance_squared[0]) {
				locate_photons(np, 2 * index);
			}
		}
		else {
			// Search the left plane first.
			locate_photons(np,  2 * index);

			if (dist1 * dist1 < np->distance_squared[0]) {
				locate_photons(np, 2 * index + 1);
			}
		}
	}	

	// Compute square distance between current photon and np->position.
	scalar_t dist2 = 0;
	dist1 = p->position[0] - np->position[0];
	dist2 += dist1 * dist1;
	dist1 = p->position[1] - np->position[1];
	dist2 += dist1 * dist1;
	dist1 = p->position[2] - np->position[2];
	dist2 += dist1 * dist1;

	if (dist2 < np->distance_squared[0]) {
		// The photon satisfies the distance criteria.
		if (np->count < np->max) {
			// The photon satisfies the count criteria.
			np->count++;
			np->distance_squared[np->count] = dist2;
			np->index[np->count] = p;
		}
		else {
			unsigned int j, parent;

			if (np->flag_heap == 0) {
				// Build heap.
				float dst_squared;
				const photon_t *phot;
				unsigned int  half_count = np->count >> 1;

				for (unsigned int k = half_count; k >= 1; --k) {
					parent = k;
					phot = np->index[k];
					dst_squared = np->distance_squared[k];

					while (parent <= half_count) {
						j = parent + parent;

						if (j < np->count && np->distance_squared[j] < np->distance_squared[j + 1]) {
							j++;
						}

						if (dst_squared >= np->distance_squared[j]) {
							break;
						}

						np->distance_squared[parent] = np->distance_squared[j];
						np->index[parent] = np->index[j];
						parent = j;
					}
					
					np->distance_squared[parent] = dst_squared;
					np->index[parent] = phot;
				}

				np->flag_heap = 1;
			}

			// Insert new photon into max heap.
			// delete largest element, insert new, and reorder the heap.
			parent = 1;
			j = 2;

			while (j <= np->count) {
				if (j < np->count && np->distance_squared[j] < np->distance_squared[j+1]) {
					j++;
				}

				if (dist2 > np->distance_squared[j]) {
					break;
				}

				np->distance_squared[parent] = np->distance_squared[j];
				np->index[parent] = np->index[j];
				parent = j;
				j += j;
			}

			np->index[parent] = p;
			np->distance_squared[parent] = dist2;

			np->distance_squared[0] = np->distance_squared[1];
		}
	}
}

void PhotonMap::scale_power(const scalar_t factor)
{
	for (unsigned int i = m_prev_scale; i < m_count_stored; ++i) {
		m_photons[i].power[0] *= factor;
		m_photons[i].power[1] *= factor;
		m_photons[i].power[2] *= factor;
	}

	m_prev_scale = m_count_stored + 1;
}
