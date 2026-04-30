#ifndef XTCORE_PROTO_H_INCLUDED
#define XTCORE_PROTO_H_INCLUDED

/*
**	    SYMBOLIC NAME			    LITERAL				TYPE			 DESCRIPTION
**      ----------------------------------------------------------------------------------------------------
*/
#define XTPROTO_FORMAT_VEC3         "vec3(%f,%f,%f)"    /* string     */ /* Format for vec3 */
#define XTPROTO_FORMAT_COL3         "col3(%f,%f,%f)"    /* string     */ /* Format for col3 */
#define XTPROTO_FORMAT_TEX2         "tex2(%f,%f)"       /* string     */ /* Format for tex2 */
#define XTPROTO_FORMAT_EXTERNAL     "ext(%[a-zA-Z)"     /* string     */ /* format for external file */
#define XTPROTO_FORMAT_GENERATE     "gen(%[a-zA-Z_])"   /* string     */ /* Format for generate function */

#define XTPROTO_CONFIG              "config"            /* N/A        */ /* Config */
#define XTPROTO_PROP_TITLE			"title"				/* string     */ /* Scene title */
#define XTPROTO_PROP_DESCR			"description"	    /* string     */ /* Scene description */
#define XTPROTO_PROP_VERSN			"version"			/* string     */ /* Scene version */
#define XTPROTO_PROP_DEFAULT_CAMERA "default_camera"    /* string     */ /* Scene default camera */
#define XTPROTO_LTRL_ICOSAHEDRON    "icosahedron"       /* string     */ /* Icosahedron */
#define XTPROTO_LTRL_PLANE          "plane"             /* string     */ /* Plane */
#define XTPROTO_LTRL_TETRAHEDRON    "tetrahedron"       /* string     */ /* Tetrahedron */
#define XTPROTO_LTRL_HEXAHEDRON     "hexahedron"        /* string     */ /* Hexahedron (cube) */
#define XTPROTO_LTRL_CUBE           "cube"              /* string     */ /* Cube */
#define XTPROTO_LTRL_OCTAHEDRON     "octahedron"        /* string     */ /* Octahedron */
#define XTPROTO_LTRL_DODECAHEDRON   "dodecahedron"      /* string     */ /* Dodecahedron */
#define XTPROTO_LTRL_PYRAMID        "pyramid"           /* string     */ /* Pyramid */
#define XTPROTO_LTRL_RING           "ring"              /* string     */ /* Ring (torus) */
#define XTPROTO_LTRL_TORUS          "torus"             /* string     */ /* Torus (alias for ring) */
#define XTPROTO_LTRL_ROUNDED_RING   "rounded_ring"      /* string     */ /* Rounded ring */
#define XTPROTO_LTRL_SNOWFLAKE      "snowflake"         /* string     */ /* Koch snowflake */
#define XTPROTO_LTRL_CAPSULE        "capsule"           /* string     */ /* Capsule */
#define XTPROTO_LTRL_CAPPED_CYLINDER "capped_cylinder"  /* string     */ /* Capped cylinder */
#define XTPROTO_LTRL_TRUNCATED_CONE "truncated_cone"    /* string     */ /* Truncated cone */
#define XTPROTO_LTRL_TORUS_KNOT     "torus_knot"        /* string     */ /* Torus knot */
#define XTPROTO_LTRL_ICOSPHERE      "icosphere"         /* string     */ /* Icosphere */
#define XTPROTO_LTRL_GEODESIC_DOME  "geodesic_dome"     /* string     */ /* Geodesic dome */
#define XTPROTO_LTRL_ICOSA_CAGE     "icosa_cage"        /* string     */ /* Icosahedral cage */
#define XTPROTO_LTRL_MENGER_SPONGE  "menger_sponge"     /* string     */ /* Menger sponge */
#define XTPROTO_LTRL_MENGER_SPONGE_IMPLICIT "menger_sponge_implicit" /* string */ /* Implicit Menger sponge */
#define XTPROTO_LTRL_SIERPINSKI_TETRAHEDRON "sierpinski_tetrahedron" /* string */ /* Sierpinski tetrahedron */
#define XTPROTO_LTRL_SIERPINSKI_TETRAHEDRON_IMPLICIT "sierpinski_tetrahedron_implicit" /* string */ /* Implicit Sierpinski tetrahedron */
#define XTPROTO_LTRL_MANDELBULB		"mandelbulb"		/* string     */ /* Mandelbulb fractal */
#define XTPROTO_LTRL_JULIA			"julia"				/* string     */ /* Julia 3D fractal */
#define XTPROTO_LTRL_MOBIUS_STRIP	"mobius_strip"		/* string     */ /* Mobius strip */
#define XTPROTO_LTRL_KLEIN_BOTTLE	"klein_bottle"		/* string     */ /* Klein bottle */
#define XTPROTO_LTRL_HAIRBALL		"hairball"			/* string     */ /* Hairball */
#define XTPROTO_LTRL_SHELL_SPIRAL	"shell_spiral"		/* string     */ /* Shell spiral */
#define XTPROTO_LTRL_ROCK			"rock"				/* string     */ /* Rock */
#define XTPROTO_LTRL_CHAIN_LINK		"chain_link"		/* string     */ /* Chain links */
#define XTPROTO_LTRL_TUBE_CURVE     "tube_curve"        /* string     */ /* Tube swept along a curve */
#define XTPROTO_LTRL_LATHE			"lathe"				/* string     */ /* Lathe */
#define XTPROTO_LTRL_TERRAIN        "terrain"           /* string     */ /* Terrain */
#define XTPROTO_LTRL_DRAPED_CLOTH_STRIP "draped_cloth_strip" /* string */ /* Draped cloth strip */
#define XTPROTO_LTRL_SVG            "svg"               /* string     */ /* SVG silhouette mesh */
#define XTPROTO_PROP_CRD_X			"x"					/* scalar_t   */ /* Vector's x coordinate */
#define XTPROTO_PROP_CRD_Y			"y"					/* scalar_t   */ /* Vector's y coordinate */
#define XTPROTO_PROP_CRD_Z			"z"					/* scalar_t   */ /* Vector's z coordinate */
#define XTPROTO_PROP_CRD_U			"u"					/* scalar_t   */ /* TexCoord's u coordinate */
#define XTPROTO_PROP_CRD_V			"v"					/* scalar_t   */ /* TexCoord's v coordinate */
#define XTPROTO_PROP_COL_R			"r"					/* scalar_t   */ /* Color's red component */
#define XTPROTO_PROP_COL_G			"g"					/* scalar_t   */ /* Color's green component */
#define XTPROTO_PROP_COL_B			"b"					/* scalar_t   */ /* Color's blue component */
#define XTPROTO_PROP_COL_A			"a"					/* scalar_t   */ /* Color's alpha component */
#define XTPROTO_PROP_VRT_0			"v0"				/* vector     */ /* Vector coordinates */
#define XTPROTO_PROP_VRT_1			"v1"				/* vector     */ /* Vector coordinates */
#define XTPROTO_PROP_VRT_2			"v2"				/* vector     */ /* Vector coordinates */
#define XTPROTO_PROP_PROPERTIES		"properties"		/* string     */ /* properties */
#define XTPROTO_PROP_MEDIUM			"medium"			/* string     */ /* interior medium asset reference */
#define XTPROTO_PROP_EXTERIOR_MEDIUM	"exterior_medium"	/* string     */ /* exterior medium asset reference */
#define XTPROTO_PROP_RESOLUTION		"resolution"		/* string     */ /* resolution */
#define XTPROTO_PROP_SCALARS		"scalars"			/* string     */ /* scalar properties */
#define XTPROTO_PROP_COLORS			"colors"			/* string     */ /* color properties */
#define XTPROTO_PROP_TEXTURES		"textures"			/* string     */ /* texture properties */
#define XTPROTO_PROP_PREFIX			"prefix"			/* string     */ /* A prefix string */
#define XTPROTO_PROP_TEXTURE_DIR	"texture_dir"		/* string     */ /* Override directory for imported textures */
#define XTPROTO_PROP_IGNORE			"ignore"			/* string     */ /* Comma-separated list of material names to skip during import */
#define XTPROTO_PROP_TYPE			"type"				/* string     */ /* Type */
#define XTPROTO_PROP_DEFAULT		"default"			/* string     */ /* Default value */
#define XTPROTO_PROP_POSITION		"position"			/* Vector3f   */ /* Translation */
#define XTPROTO_PROP_TEXCOORD		"texcoord"			/* Vector2f   */ /* Texture coordinates */
#define XTPROTO_PROP_SOURCE			"source"			/* string     */ /* External source file */
#define XTPROTO_PROP_SVG_SOURCE     "svg_source"        /* string     */ /* SVG source file for gen(svg) */
#define XTPROTO_PROP_FOV			"fov"				/* scalar_t   */ /* Field of view */
#define XTPROTO_PROP_APERTURE		"aperture"			/* scalar_t   */ /* Aperture of the lense */
#define XTPROTO_PROP_TARGET			"target"			/* Vector3f   */ /* Target position */
#define XTPROTO_PROP_FLENGTH		"flength"			/* scalar_t   */ /* Focal length */
#define XTPROTO_PROP_APERTURE_BLADES "aperture_blades"  /* integer    */ /* Aperture polygon blades (>=3 enables polygon bokeh) */
#define XTPROTO_PROP_APERTURE_ROTATION "aperture_rotation" /* scalar_t */ /* Aperture polygon rotation in degrees */
#define XTPROTO_PROP_ORIENTATION	"orientation"		/* Vector3f   */ /* Orientation */
#define XTPROTO_PROP_INTST			"intensity"			/* ColorRGBf  */ /* Color intensity */
#define XTPROTO_PROP_IAMBN			"ambient"			/* ColorRGBf  */ /* Ambient intensity */
#define XTPROTO_PROP_IDIFF			"diffuse"			/* ColorRGBf  */ /* Diffuse intensity */
#define XTPROTO_PROP_ISPEC			"specular"			/* ColorRGBf  */ /* Specular intensity */
#define XTPROTO_PROP_EMISSIVE		"emissive"			/* ColorRGBf  */ /* Emissive intensity */
#define XTPROTO_PROP_KAMBN			"k_ambient"			/* scalar_t   */ /* Ambient ratio */
#define XTPROTO_PROP_KDIFF			"k_diffuse"			/* scalar_t   */ /* Diffuse ratio */
#define XTPROTO_PROP_KSPEC			"k_specular"		/* scalar_t   */ /* Specular ratio */
#define XTPROTO_PROP_KEXPN			"exponent"	    	/* scalar_t   */ /* Specular exponent */
#define	XTPROTO_PROP_ROUGH			"roughness"			/* scalar_t   */ /* Roughness */
#define XTPROTO_PROP_REFLC			"reflectance"		/* scalar_t   */ /* Reflectance ratio */
#define XTPROTO_PROP_TRSPC			"transparency"		/* scalar_t   */ /* Transparency ratio */
#define XTPROTO_PROP_IOR			"ior"				/* scalar_t   */ /* Index of refraction */
#define XTPROTO_PROP_SIGMA_A        "sigma_a"           /* ColorRGBf  */ /* Absorption coefficient */
#define XTPROTO_PROP_SIGMA_S		"sigma_s"			/* ColorRGBf  */ /* Scattering coefficient */
#define XTPROTO_PROP_G				"g"					/* scalar_t   */ /* Anisotropy */
#define XTPROTO_PROP_EMISSION		"emission"			/* ColorRGBf  */ /* Medium emission */
#define XTPROTO_PROP_DENSITY        "density"           /* scalar_t   */ /* Density multiplier */
#define XTPROTO_PROP_NOISE_SCALE    "noise_scale"       /* scalar_t   */ /* Noise frequency scale */
#define XTPROTO_PROP_NOISE_MIN      "noise_min"         /* scalar_t   */ /* Minimum density factor */
#define XTPROTO_PROP_NOISE_MAX      "noise_max"         /* scalar_t   */ /* Maximum density factor */
#define XTPROTO_PROP_LACUNARITY     "lacunarity"        /* scalar_t   */ /* fBm lacunarity */
#define XTPROTO_PROP_GAIN           "gain"              /* scalar_t   */ /* fBm gain */
#define XTPROTO_PROP_IPD			"ipd"				/* scalar_t   */ /* Inter-Pupillary Distance */
#define XTPROTO_PROP_FILTERING      "filtering"         /* string     */ /* Texture filtering type */
#define XTPROTO_LTRL_NEAREST        "nearest"           /* string     */ /* Texture filtering, nearest */
#define XTPROTO_LTRL_LINEAR         "linear"            /* string     */ /* Texture filtering, linear */
#define XTPROTO_LTRL_BILINEAR       "bilinear"          /* string     */ /* Texture filtering, bilinear */
#define XTPROTO_LTRL_POSX           "posx"              /* string     */ /* Cubemap face */
#define XTPROTO_LTRL_POSY           "posy"              /* string     */ /* Cubemap face */
#define XTPROTO_LTRL_POSZ           "posz"              /* string     */ /* Cubemap face */
#define XTPROTO_LTRL_NEGX           "negx"              /* string     */ /* Cubemap face */
#define XTPROTO_LTRL_NEGY           "negy"              /* string     */ /* Cubemap face */
#define XTPROTO_LTRL_NEGZ           "negz"              /* string     */ /* Cubemap face */
#define XTPROTO_LTRL_EMISSIVE		"emissive"  		/* string     */ /* BRDF: Emissive */
#define XTPROTO_LTRL_LAMBERT		"lambert"			/* string     */ /* BRDF: Lambert */
#define XTPROTO_LTRL_PHONG			"phong"				/* string     */ /* BRDF: Phong */
#define XTPROTO_LTRL_BLINNPHONG		"blinn_phong"		/* string     */ /* BRDF: Blinn Phong */
#define XTPROTO_LTRL_EMISSIVE		"emissive"	     	/* string     */ /* BRDF: Emissive */
#define XTPROTO_LTRL_DIELECTRIC     "dielectric"        /* string     */ /* BRDF: Dielectric */
#define XTPROTO_LTRL_PRINCIPLED     "principled"        /* string     */ /* BRDF: GGX principled */
#define XTPROTO_LTRL_ROUGH_DIELECTRIC "rough_dielectric" /* string    */ /* BRDF: Rough dielectric */
#define XTPROTO_LTRL_THIN_DIELECTRIC "thin_dielectric"  /* string     */ /* BRDF: Thin dielectric sheet */
#define XTPROTO_LTRL_SUBSURFACE    "subsurface"         /* string     */ /* BRDF: Pragmatic subsurface */
#define XTPROTO_LTRL_SHEEN        "sheen"              /* string     */ /* BRDF: Cloth-like sheen */
#define XTPROTO_LTRL_THIN_TRANSLUCENT "thin_translucent" /* string    */ /* BRDF: Thin translucent sheet */
#define XTPROTO_LTRL_BOUNDARY       "boundary"          /* string     */ /* Interface-only boundary */
#define XTPROTO_LTRL_HOMOGENEOUS    "homogeneous"       /* string     */ /* Homogeneous medium */
#define XTPROTO_LTRL_HETEROGENEOUS_NOISE "heterogeneous_noise" /* string */ /* Procedural heterogeneous medium */
#define XTPROTO_LTRL_PLANE			"plane"				/* string     */ /* Plane */
#define XTPROTO_LTRL_TRIANGLE		"triangle"			/* string     */ /* Triangle */
#define XTPROTO_LTRL_POINT			"point"	    		/* string     */ /* Point */
#define XTPROTO_LTRL_SPHERE			"sphere"			/* string     */ /* Sphere */
#define XTPROTO_LTRL_HULL			"hull"  			/* string     */ /* Hull */
#define XTPROTO_LTRL_CONE           "cone"              /* string     */ /* Cone */
#define XTPROTO_LTRL_CYLINDER		"cylinder"		 	/* string     */ /* Cylinder */
#define XTPROTO_LTRL_MESH			"mesh"				/* string     */ /* Mesh */
#define XTPROTO_LTRL_CSG            "csg"               /* string     */ /* Constructive solid geometry */
#define XTPROTO_PROP_UP				"up"	            /* Vector3f   */ /* Up vector */
#define XTPROTO_PROP_RIGHT			"right"	            /* Vector3f   */ /* Right vector */
#define XTPROTO_PROP_NORMAL			"normal"			/* Vector3f   */ /* Normal vector */
#define XTPROTO_PROP_DISTANCE		"distance"			/* scalar_t   */ /* Distance */
#define XTPROTO_PROP_USCALE			"u_scale"			/* scalar_t   */ /* U scaling */
#define XTPROTO_PROP_VSCALE			"v_scale"			/* scalar_t   */ /* V scaling */
#define XTPROTO_PROP_DIMENSIONS		"dimensions"		/* Vector3f   */ /* Dimensions for 3d shape */
#define XTPROTO_PROP_TRANSLATION	"translation"		/* Vector3f   */ /* Translation */
#define XTPROTO_PROP_ROTATION		"rotation"			/* Vector3f   */ /* Rotation */
#define XTPROTO_PROP_SCALE			"scale"				/* Vector3f   */ /* Scale */
#define XTPROTO_PROP_RADIUS			"radius"			/* scalar_t   */ /* Radius */
#define XTPROTO_PROP_WIDTH          "width"             /* scalar_t   */ /* Width */
#define XTPROTO_PROP_BASE_SIZE      "base_size"         /* scalar_t   */ /* Base size */
#define XTPROTO_PROP_SMOOTH_NORMALS "smooth_normals"    /* bool       */ /* Use smooth (vertex) normals instead of flat (face) normals */
#define XTPROTO_PROP_SEED           "seed"              /* integer    */ /* Seed */
#define XTPROTO_PROP_CELLS          "cells"             /* integer    */ /* Cell count */
#define XTPROTO_PROP_MAX_DEVIATION  "max_deviation"     /* scalar_t   */ /* Max normal deviation (degrees) */
#define XTPROTO_PROP_FIBERS         "fibers"            /* integer    */ /* Fiber count */
#define XTPROTO_PROP_TURNS          "turns"             /* scalar_t   */ /* Turn count */
#define XTPROTO_PROP_GROWTH         "growth"            /* scalar_t   */ /* Growth */
#define XTPROTO_PROP_TUBE_RADIUS    "tube_radius"       /* scalar_t   */ /* Tube radius */
#define XTPROTO_PROP_HEIGHT			"height"			/* scalar_t   */ /* Height */
#define XTPROTO_PROP_POWER          "power"             /* scalar_t   */ /* Fractal power */
#define XTPROTO_PROP_BAILOUT        "bailout"           /* scalar_t   */ /* Fractal bailout */
#define XTPROTO_PROP_JULIA_C        "julia_c"           /* Vector3f   */ /* Julia constant */
#define XTPROTO_PROP_THICKNESS      "thickness"         /* scalar_t   */ /* Ring thickness (outer-inner radius) */
#define XTPROTO_PROP_HEIGHT_RESOLUTION "height_resolution" /* scalar_t */ /* Height resolution */
#define XTPROTO_PROP_PROFILE_RESOLUTION "profile_resolution" /* integer */ /* Profile resolution */
#define XTPROTO_PROP_CAP_TOP        "cap_top"           /* string     */ /* Top cap profile */
#define XTPROTO_PROP_CAP_BOTTOM     "cap_bottom"        /* string     */ /* Bottom cap profile */
#define XTPROTO_PROP_CAP_ROUNDNESS  "cap_roundness"     /* scalar_t   */ /* Rounded cap radius */
#define XTPROTO_PROP_CAP_RESOLUTION "cap_resolution"    /* integer    */ /* Rounded cap tessellation */
#define XTPROTO_PROP_OCTAVES        "octaves"           /* integer    */ /* Octave count */
#define XTPROTO_PROP_COUNT          "count"             /* integer    */ /* Count */
#define XTPROTO_PROP_MAJOR_RADIUS   "major_radius"      /* scalar_t   */ /* Major radius */
#define XTPROTO_PROP_MINOR_RADIUS   "minor_radius"      /* scalar_t   */ /* Minor radius */
#define XTPROTO_PROP_SPACING        "spacing"           /* scalar_t   */ /* Spacing */
#define XTPROTO_PROP_PROFILE        "profile"           /* group      */ /* Lathe profile */
#define XTPROTO_PROP_CAP_ENDS       "cap_ends"          /* bool       */ /* Cap ends */
#define XTPROTO_PROP_SPLINE         "spline"            /* group      */ /* Spline */
#define XTPROTO_PROP_CLOSED         "closed"            /* bool       */ /* Closed curve */
#define XTPROTO_PROP_END_A			"end_a"				/* scalar_t   */ /* End a */
#define XTPROTO_PROP_END_B			"end_b"				/* scalar_t   */ /* End b */
#define XTPROTO_LTRL_FLAT           "flat"              /* string     */ /* Flat profile */
#define XTPROTO_LTRL_ROUND          "round"             /* string     */ /* Rounded profile */
#define XTPROTO_PROP_VRTXDATA		"vecdata"			/* group      */ /* Vertex data */
#define XTPROTO_PROP_OP				"op"				/* string     */ /* CSG boolean operator */
#define XTPROTO_PROP_SMOOTHNESS		"smoothness"		/* scalar_t   */ /* CSG soft union smoothing */
#define XTPROTO_PROP_LEFT			"left"				/* group      */ /* CSG left node */
#define XTPROTO_LTRL_UNION			"union"				/* string     */ /* CSG union operator */
#define XTPROTO_LTRL_SOFT_UNION		"soft_union"		/* string     */ /* CSG soft union operator */
#define XTPROTO_LTRL_SMOOTH_UNION	"smooth_union"		/* string     */ /* CSG soft union alias */
#define XTPROTO_LTRL_INTERSECTION	"intersection"		/* string     */ /* CSG intersection operator */
#define XTPROTO_LTRL_DIFFERENCE		"difference"		/* string     */ /* CSG difference operator */
#define XTPROTO_PROP_OBJ_GEO		"geometry"			/* asset_id_t */ /* Geometry id */
#define XTPROTO_PROP_OBJ_MAT		"material"			/* asset_id_t */ /* Material id */
#define XTPROTO_LTRL_TEXTURE  		"texture"			/* asset_id_t */ /* Texture id */
#define XTPROTO_NODE_ENVIRONMENT	"environment"		/* N/A        */ /* Environment */
#define XTPROTO_GRADIENT			"gradient"			/* N/A        */ /* Gradient */
#define XTPROTO_NODE_CAMERA			"camera"			/* N/A		  */ /* Resource node */
#define XTPROTO_NODE_MATERIAL		"material"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_MEDIUM			"medium"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_TEXTURE		"texture"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_GEOMETRY		"geometry"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_OBJECT			"object"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_VARIANTS		"variants"			/* N/A        */ /* Scene variants node */
#define XTPROTO_NODE_VARIANT_SET	"set"				/* N/A        */ /* Variant set overlay */
#define XTPROTO_NODE_VARIANT_REMOVE	"remove"			/* N/A        */ /* Variant remove overlay */
#define XTPROTO_LTRL_CAM_THINLENS	"thin-lens"			/* string     */ /* Perspective camera */
#define XTPROTO_LTRL_CAM_ODS		"ods"				/* string     */ /* Omni Directional Stereo camera */
#define XTPROTO_LTRL_CAM_ERP		"erp"				/* string     */ /* Equirectangular camera */
#define XTPROTO_LTRL_CAM_CUBEMAP    "cubemap"           /* string     */ /* Cubemap camera */
#define XTPROTO_LTRL_CAM_TILTSHIFT  "tilt-shift"        /* string     */ /* Tilt-shift camera */
#define XTPROTO_PROP_TILT           "tilt"              /* scalar_t   */ /* Focal-plane tilt angle in degrees */
#define XTPROTO_PROP_SHIFT_X        "shift_x"           /* scalar_t   */ /* Horizontal lens shift (normalised sensor units) */
#define XTPROTO_PROP_SHIFT_Y        "shift_y"           /* scalar_t   */ /* Vertical lens shift (normalised sensor units) */
#define XTPROTO_TEXTURE             "texture"
#define XTPROTO_CUBEMAP             "cubemap"
#define XTPROTO_ERP                 "erp"
#define XTPROTO_COLOR               "color"
#define XTPROTO_RAYLEIGH_SKY        "rayleigh_sky"
#define XTPROTO_GRAPHPAPER          "graphpaper"
#define XTPROTO_SCALEGRID           "scalegrid"
#define XTPROTO_CHECKER             "checker"
#define XTPROTO_WEAVE               "weave"
#define XTPROTO_FBM_MARBLE          "fbm_marble"
#define XTPROTO_VORONOI_NORMAL      "voronoi_normal"
#define XTPROTO_SCENERY_HEIGHTFIELD "scenery_heightfield"
#define XTPROTO_FBM_WOOD            "fbm_wood"
#define XTPROTO_CURL_NOISE          "curl_noise"
#define XTPROTO_SCRATCHES           "scratches"
#define XTPROTO_EDGE_WEAR           "edge_wear"
#define XTPROTO_BRICK               "brick"
#define XTPROTO_DOTS                "dots"
#define XTPROTO_PREETHAM_SKY        "preetham_sky"
#define XTPROTO_HOSEK_WILKIE_SKY    "hosek_wilkie_sky"
#define XTPROTO_STARS               "stars"
#define XTPROTO_BLEND               "blend"
#define XTPROTO_MIX_MASKED          "mix_masked"
#define XTPROTO_TRIPLANAR           "triplanar"
#define XTPROTO_PROPERTIES          "properties"
#define XTPROTO_SAMPLERS            "samplers"
#define XTPROTO_SCALARS             "scalars"
#define XTPROTO_VALUE               "value"
#define XTPROTO_FLIP_NORMALS        "flip_normals"
#define XTPROTO_FLIP_X              "flip_x"
#define XTPROTO_FLIP_Y              "flip_y"
#define XTPROTO_MODIFIERS           "modifiers"
#define XTPROTO_EXTRUDE             "extrude"
#define XTPROTO_MULTIPLIER          "multiplier"
#define XTPROTO_PROP_HEIGHT_SAMPLER "height_sampler"

/* New procedural mesh generator literals */
#define XTPROTO_LTRL_GEAR            "gear"              /* string     */ /* Gear */
#define XTPROTO_LTRL_SPRING          "spring"            /* string     */ /* Coil spring */
#define XTPROTO_LTRL_TREE            "tree"              /* string     */ /* Recursive tree */
#define XTPROTO_LTRL_CRYSTAL         "crystal"           /* string     */ /* Crystal cluster */
#define XTPROTO_LTRL_DISC            "disc"              /* string     */ /* Flat disc / annulus */
#define XTPROTO_LTRL_CORAL           "coral"             /* string     */ /* Coral colony */
#define XTPROTO_LTRL_HEMISPHERE      "hemisphere"        /* string     */ /* Hemisphere dome */
#define XTPROTO_LTRL_STAR            "star"              /* string     */ /* Extruded star */
#define XTPROTO_LTRL_SUPERELLIPSOID  "superellipsoid"    /* string     */ /* Superellipsoid */
#define XTPROTO_LTRL_CITY            "city"              /* string     */ /* Procedural city */
#define XTPROTO_LTRL_LOWPOLY_TERRAIN "lowpoly_terrain"   /* string     */ /* Flat-shaded low-poly terrain */
#define XTPROTO_LTRL_DISPLACED_SPHERE "displaced_sphere" /* string     */ /* Icosphere with radial texture displacement */

/* New property tokens */
#define XTPROTO_PROP_TOOTH_COUNT     "tooth_count"       /* integer    */ /* Number of gear teeth */
#define XTPROTO_PROP_TOOTH_DEPTH     "tooth_depth"       /* scalar_t   */ /* Depth of gear tooth */
#define XTPROTO_PROP_INNER_RADIUS    "inner_radius"      /* scalar_t   */ /* Inner radius */
#define XTPROTO_PROP_OUTER_RADIUS    "outer_radius"      /* scalar_t   */ /* Outer radius */
#define XTPROTO_PROP_COILS           "coils"             /* scalar_t   */ /* Number of coils */
#define XTPROTO_PROP_WIRE_RADIUS     "wire_radius"       /* scalar_t   */ /* Wire / tube radius */
#define XTPROTO_PROP_SPRING_RADIUS   "spring_radius"     /* scalar_t   */ /* Spring coil radius */
#define XTPROTO_PROP_BRANCH_COUNT    "branch_count"      /* integer    */ /* Branches per node */
#define XTPROTO_PROP_BRANCH_ANGLE    "branch_angle"      /* scalar_t   */ /* Branch angle (radians) */
#define XTPROTO_PROP_DEPTH           "depth"             /* integer    */ /* Recursion depth */
#define XTPROTO_PROP_TRUNK_HEIGHT    "trunk_height"      /* scalar_t   */ /* Trunk / main stem height */
#define XTPROTO_PROP_TRUNK_RADIUS    "trunk_radius"      /* scalar_t   */ /* Trunk base radius */
#define XTPROTO_PROP_BRANCH_RADIUS   "branch_radius"     /* scalar_t   */ /* Branch base radius */
#define XTPROTO_PROP_TIP_HEIGHT      "tip_height"        /* scalar_t   */ /* Crystal tip height */
#define XTPROTO_PROP_POINTS          "points"            /* integer    */ /* Star point count */
#define XTPROTO_PROP_E1              "e1"                /* scalar_t   */ /* Superellipsoid N-S exponent */
#define XTPROTO_PROP_E2              "e2"                /* scalar_t   */ /* Superellipsoid E-W exponent */
#define XTPROTO_PROP_BLOCKS_X              "blocks_x"              /* integer    */ /* City blocks along X */
#define XTPROTO_PROP_BLOCKS_Z              "blocks_z"              /* integer    */ /* City blocks along Z */
#define XTPROTO_PROP_BLOCK_SIZE            "block_size"            /* scalar_t   */ /* City block size */
#define XTPROTO_PROP_ROAD_WIDTH            "road_width"            /* scalar_t   */ /* Road width */
#define XTPROTO_PROP_BUILDING_HEIGHT_MIN   "building_height_min"   /* scalar_t   */ /* Minimum building height */
#define XTPROTO_PROP_BUILDING_HEIGHT_MAX   "building_height_max"   /* scalar_t   */ /* Maximum building height */
#define XTPROTO_PROP_LOT_PADDING           "lot_padding"           /* scalar_t   */ /* Padding inside each lot */
#define XTPROTO_PROP_BUILDINGS_PER_BLOCK_X "buildings_per_block_x" /* integer    */ /* Building columns per block */
#define XTPROTO_PROP_BUILDINGS_PER_BLOCK_Z "buildings_per_block_z" /* integer    */ /* Building rows per block */
#define XTPROTO_PROP_FLOOR_HEIGHT          "floor_height"          /* scalar_t   */ /* Height per building floor */
#define XTPROTO_PROP_BAY_WIDTH             "bay_width"             /* scalar_t   */ /* Target window bay width */
#define XTPROTO_PROP_WINDOW_WIDTH_RATIO    "window_width_ratio"    /* scalar_t   */ /* Window width / bay width */
#define XTPROTO_PROP_WINDOW_HEIGHT_RATIO   "window_height_ratio"   /* scalar_t   */ /* Window height / floor height */
#define XTPROTO_PROP_WINDOW_INSET          "window_inset"          /* scalar_t   */ /* Window recess depth */
#define XTPROTO_PROP_PAVEMENT_HEIGHT       "pavement_height"       /* scalar_t   */ /* Raised kerb height */
#define XTPROTO_PROP_PAVEMENT_WIDTH        "pavement_width"        /* scalar_t   */ /* Pavement strip width into road */
#define XTPROTO_PROP_DISPLACEMENT_SCALE    "displacement_scale"    /* scalar_t   */ /* Max radial displacement in world-space units (radius-independent) */
#define XTPROTO_PROP_DISPLACEMENT_SMOOTH   "displacement_smooth"   /* int        */ /* Laplacian smoothing passes on sampled heights before displacement */

/* Meshgroup geometry */
#define XTPROTO_LTRL_MESHGROUP       "meshgroup"         /* string     */ /* Multi-file mesh group */
#define XTPROTO_PROP_SOURCES         "sources"           /* group      */ /* File path list for meshgroup (keys ignored, values are paths) */
#define XTPROTO_PROP_GLOB            "glob"              /* string     */ /* Glob pattern for meshgroup file discovery */

#endif /* XTCORE_PROTO_H_INCLUDED */
