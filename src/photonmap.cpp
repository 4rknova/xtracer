#include <cstdlib>
#include <nmath/precision.h>
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

	m_count_stored = 0;
	m_count_max = 0;
	m_prev_scale = 1;
}

void PhotonMap::calc_conversion_tables()
{
	for (unsigned int i = 0; i < 256; ++i) {
		float angle = (float)(i) * (1.0 / 256.0) * M_PI;

		m_sintheta[i] = nmath_sin(angle);
		m_costheta[i] = nmath_cos(angle);
		m_sinphi[i]   = nmath_sin(angle);
		m_cosphi[i]   = nmath_cos(angle);
	}
}

void PhotonMap::photon_dir(float *dir, const photon_t *p) const
{
	dir[0] = m_sintheta[p->theta] * m_cosphi[p->phi];
	dir[1] = m_sintheta[p->theta] * m_sinphi[p->phi];
	dir[2] = m_costheta[p->theta];
}

void PhotonMap::store(const float position[3], const float power[3], const float direction[3])
{
	if (m_count_stored >= m_count_max) {
		return;
	}

	m_count_stored++;
	photon_t *const node = &m_photons[m_count_stored];

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

void PhotonMap::irradiance_estimate(float irradiance[3],
	const float position[3], const float normal[3], 
	const float max_distance, const unsigned int count) const
{
	// Zero out the irradiance.
	irradiance[0] = irradiance[1] = irradiance[2] = 0.0;

	photon_cloud_t np;

	np.distance_squared = (float *) malloc(sizeof(float) * (count + 1));
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

	float pdir[3];

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

	const float density = (1.0f / M_PI) / (np.distance_squared[0]); // Density estimate.

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
	float dist1;

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
	float dist2 = 0;
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

void PhotonMap::scale_power(const float factor)
{
	for (unsigned int i = m_prev_scale; i < m_count_stored; ++i) {
		m_photons[i].power[0] *= factor;
		m_photons[i].power[1] *= factor;
		m_photons[i].power[2] *= factor;
	}

	m_prev_scale = m_count_stored + 1;
}

void PhotonMap::balance()
{
	if (m_count_stored > 1) {
		// allocate two temporary arrays for the
		// balancing procedure.
		photon_t **pa1 = (photon_t **) malloc(sizeof(photon_t*) * (m_count_stored + 1));
		photon_t **pa2 = (photon_t **) malloc(sizeof(photon_t*) * (m_count_stored + 1));

		for (unsigned int i = 0; i <= m_count_stored; i++) {
			pa2[i] = &m_photons[i];
		}

		balance_segment(pa1, pa2, 1, 1, m_count_stored);

		free(pa2);
		
		// Re-organise the balanced KD-Tree
		unsigned int d, j = 1, foo = 1;
		photon_t foo_photon = m_photons[j];

		for (unsigned int i = 1; i <= m_count_stored; i++) {
			d = pa1[j] - m_photons;
			pa1[j] = NULL;

			if (d != foo) {
				m_photons[j] = m_photons[d];
			}
			else {
				m_photons[j] = foo_photon;

				if (i < m_count_stored) {
					for (; foo <= m_count_stored; foo++) {
						if (pa1[foo] != NULL) {
							break;
						}
					}

					foo_photon = m_photons[foo];
					j = foo;
				}

				continue;
			}

			j = d;
		}

		free(pa1);
	}

	m_count_stored_half = m_count_stored / 2 - 1;
}

#define swap(ph, a , b) { photon_t *ph2 = ph[a]; ph[a] = ph[b]; ph[b] = ph2;}

void PhotonMap::median_split(photon_t **p, 
	const unsigned int start, const unsigned int end,
	const unsigned int median, const unsigned int axis)
{
	unsigned int left = start;
	unsigned int right = end;

	while (right > left) {
		const float v = p[right]->position[axis];

		unsigned int i = left - 1;
		unsigned int j = right;

		for(;;) {
			while (p[++i]->position[axis] < v);
			while (p[--j]->position[axis] > v && j > left);

			if (i >= j) {
				break;
			}

			swap(p, i, j);
		}

		swap(p, i, right);
		
		if (i >= median) {
			right = i - 1;
		}

		if (i <= median) {
			left = i + 1;
		}
	}
}

void PhotonMap::balance_segment(photon_t **pbal, photon_t **porg,
	 const unsigned int index,
	 const unsigned int start,
	 const unsigned int end)
{
	// Compute new median.
	unsigned int median = 1;

	while ((4 * median) <= (end - start + 1)) {
		median += median;
	}

	if ((3 * median) <= (end - start + 1)) {
		median += median;
		median += start - 1;
	} 
	else {
		median = end - median + 1;
	}

	// Determine which axis to split along.
	unsigned int axis = 2;
	if ((m_aabb_max[0] - m_aabb_min[0]) > (m_aabb_max[1] - m_aabb_min[1]) &&
		(m_aabb_max[0] - m_aabb_min[0]) > (m_aabb_max[2] - m_aabb_min[2])) {
		axis = 0;
	}
	else if ((m_aabb_max[1] - m_aabb_min[1]) > (m_aabb_max[2] - m_aabb_min[2])) {
		axis = 1;
	}

	// Partition photon block around the median.
	median_split(porg, start, end, median, axis);

	pbal[index] = porg[median];
	pbal[index]->plane = axis;

	// Recursively balance the left and right blocks.
	if (median > start) {
		// Balance left segment.
		if (start < median - 1) {
			const float tmp = m_aabb_max[axis];
			m_aabb_max[axis] = pbal[index]->position[axis];
			balance_segment(pbal, porg, 2 * index, start, median - 1);
			m_aabb_max[axis] = tmp;
		}
		else {
			pbal[2 * index] = porg[start];
		}
	}

	if (median < end) {
		// Balance right segment.
		if (median + 1 < end) {
			const float tmp = m_aabb_min[axis];
			m_aabb_min[axis] = pbal[index]->position[axis];
			balance_segment(pbal, porg, 2 * index + 1, median + 1, end);
			m_aabb_min[axis] = tmp;
		}
		else {
			pbal[2 * index + 1] = porg[end];
		}
	}
}
