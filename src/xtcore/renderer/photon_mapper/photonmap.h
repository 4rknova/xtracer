/*
	NOTE:
	The photon map class is based on Henrik Wann Jensen's implementation
	as presented in his book: Realistic Image Synthesis Using Photon Mapping.
*/

#ifndef XTCORE_PHOTONMAP_H_INCLUDED
#define XTCORE_PHOTONMAP_H_INCLUDED


/*
	Photon.
*/
struct photon_t
{
	float position[3];		/* Photon position */
	float power[3];			/* Photon power */
	unsigned char theta, phi;	/* Incoming direction in spherical coordinates */
	unsigned char plane;		/* KD-Tree spltting plane */
};

typedef struct photon_t photon_t;


struct photon_cloud_t
{
	unsigned int max;
	unsigned int count;
	unsigned int flag_heap;

	float position[3];

	float *distance_squared;

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

		void photon_dir(float *dir, const photon_t *p) const;	// Get the photon direction.

		void store(const float position[3],						// Photon's position.
				   const float power[3],						// Photon's power.
				   const float direction[3]);					// Photon's incident direction.

		void irradiance_estimate(float irradiance[3],			// Returned irradiance.
								 const float position[3],		// Surface position.
								 const float normal[3],			// Surface normal.
								 const float max_distance,		// Maximum distance in which to look for photons.
								 const unsigned int count) const;	// Number of photons to use.

		void locate_photons(photon_cloud_t * const np, const unsigned int index) const;

		void scale_power(const float factor);

		// Create a left-balanced KD-Tree from the available flat photon array.
		void balance();

		// Split the photon array into two seperate pieces around the median, with all
		// photons below the median in the lower half and all photons above the median
		// in the upper half. The comparison criteria is the axis.
		// Based on Sedgewick's method in "Algorithms in C++"
		void median_split(photon_t **p,								// Photon array.
						  const unsigned int start,					// Start of photon block in the array.
						  const unsigned int end,					// End of photon block in the array.
						  const unsigned int median,				// Desired median number.
						  const unsigned int axis);					// Axis to split along.

		void balance_segment(photon_t **pbal,
							 photon_t **porg,
							 const unsigned int index,
							 const unsigned int start,
							 const unsigned int end);

	private:
		void calc_conversion_tables();		// Calculate the direction conversion tables.

		photon_t *m_photons;				// List of photons.

		unsigned int m_count_max;			// Number of maximum photons.
		unsigned int m_count_stored;		// Number of photons currently stored.
		unsigned int m_count_stored_half;	// Half number of photons currently stored.
		unsigned int m_prev_scale;

		// AABB
		float m_aabb_min[3];
		float m_aabb_max[3];

		// Precalculated direction conversion tables;
		float m_sintheta[256];
		float m_costheta[256];
		float m_sinphi[256];
		float m_cosphi[256];
};

#endif /* XTCORE_PHOTONMAP_H_INCLUDED */
