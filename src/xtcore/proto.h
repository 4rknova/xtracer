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
#define XTPROTO_LTRL_MANDELBULB      "mandelbulb"        /* string     */ /* Mandelbulb fractal */
#define XTPROTO_LTRL_JULIA           "julia"             /* string     */ /* Julia 3D fractal */
#define XTPROTO_LTRL_MOBIUS_STRIP   "mobius_strip"      /* string     */ /* Mobius strip */
#define XTPROTO_LTRL_KLEIN_BOTTLE   "klein_bottle"      /* string     */ /* Klein bottle */
#define XTPROTO_LTRL_HAIRBALL       "hairball"          /* string     */ /* Hairball */
#define XTPROTO_LTRL_SHELL_SPIRAL   "shell_spiral"      /* string     */ /* Shell spiral */
#define XTPROTO_LTRL_ROCK           "rock"              /* string     */ /* Rock */
#define XTPROTO_LTRL_CHAIN_LINK     "chain_link"        /* string     */ /* Chain links */
#define XTPROTO_LTRL_LATHE          "lathe"             /* string     */ /* Lathe */
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
#define XTPROTO_PROP_PROPERTIES     "properties"        /* string     */ /* properties */
#define XTPROTO_PROP_RESOLUTION     "resolution"        /* string     */ /* resolution */
#define XTPROTO_PROP_SCALARS        "scalars"           /* string     */ /* scalar properties */
#define XTPROTO_PROP_COLORS         "colors"            /* string     */ /* color properties */
#define XTPROTO_PROP_TEXTURES       "textures"          /* string     */ /* texture properties */
#define XTPROTO_PROP_PREFIX         "prefix"            /* string     */ /* A prefix string */
#define XTPROTO_PROP_TYPE			"type"				/* string	  */ /* Type */
#define XTPROTO_PROP_DEFAULT		"default"			/* string     */ /* Default value */
#define XTPROTO_PROP_POSITION		"position"			/* Vector3f   */ /* Translation */
#define XTPROTO_PROP_TEXCOORD		"texcoord"			/* Vector2f   */ /* Texture coordinates */
#define XTPROTO_PROP_SOURCE			"source"			/* string     */ /* External source file */
#define XTPROTO_PROP_FOV			"fov"				/* scalar_t   */ /* Field of view */
#define XTPROTO_PROP_APERTURE		"aperture"			/* scalar_t   */ /* Aperture of the lense */
#define XTPROTO_PROP_TARGET			"target"			/* Vector3f   */ /* Target position */
#define XTPROTO_PROP_FLENGTH		"flength"			/* scalar_t   */ /* Focal length */
#define XTPROTO_PROP_APERTURE_BLADES "aperture_blades"  /* integer    */ /* Aperture polygon blades (>=3 enables polygon bokeh) */
#define XTPROTO_PROP_APERTURE_ROTATION "aperture_rotation" /* scalar_t */ /* Aperture polygon rotation in degrees */
#define XTPROTO_PROP_ORIENTATION    "orientation"		/* Vector3f   */ /* Orientation */
#define XTPROTO_PROP_INTST			"intensity"			/* ColorRGBf  */ /* Color intensity */
#define XTPROTO_PROP_IAMBN			"ambient"			/* ColorRGBf  */ /* Ambient intensity */
#define XTPROTO_PROP_IDIFF			"diffuse"			/* ColorRGBf  */ /* Diffuse intensity */
#define XTPROTO_PROP_ISPEC			"specular"			/* ColorRGBf  */ /* Specular intensity */
#define XTPROTO_PROP_EMISSIVE       "emissive"          /* ColorRGBf  */ /* Emissive intensity */
#define XTPROTO_PROP_KAMBN			"k_ambient"			/* scalar_t   */ /* Ambient ratio */
#define XTPROTO_PROP_KDIFF			"k_diffuse"			/* scalar_t   */ /* Diffuse ratio */
#define XTPROTO_PROP_KSPEC			"k_specular"		/* scalar_t   */ /* Specular ratio */
#define XTPROTO_PROP_KEXPN			"exponent"	    	/* scalar_t   */ /* Specular exponent */
#define	XTPROTO_PROP_ROUGH			"roughness"			/* scalar_t   */ /* Roughness */
#define XTPROTO_PROP_REFLC			"reflectance"		/* scalar_t   */ /* Reflectance ratio */
#define XTPROTO_PROP_TRSPC			"transparency"		/* scalar_t   */ /* Transparency ratio */
#define XTPROTO_PROP_IOR			"ior"				/* scalar_t   */ /* Index of refraction */
#define XTPROTO_PROP_IPD			"ipd"				/* scalar_t   */ /* Inter-Pupillary Distance */
#define XTPROTO_PROP_FILTERING      "filtering"         /* string     */ /* Texture filtering type */
#define XTPROTO_LTRL_NEAREST        "nearest"           /* string     */ /* Texture filtering, nearest */
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
#define XTPROTO_LTRL_PLANE			"plane"				/* string     */ /* Plane */
#define XTPROTO_LTRL_TRIANGLE		"triangle"			/* string     */ /* Triangle */
#define XTPROTO_LTRL_POINT			"point"	    		/* string     */ /* Point */
#define XTPROTO_LTRL_SPHERE			"sphere"			/* string     */ /* Sphere */
#define XTPROTO_LTRL_HULL			"hull"  			/* string     */ /* Hull */
#define XTPROTO_LTRL_CONE           "cone"             /* string     */ /* Cone */
#define XTPROTO_LTRL_CYLINDER		"cylinder"			/* string     */ /* Cylinder */
#define XTPROTO_LTRL_MESH			"mesh"				/* string     */ /* Mesh */
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
#define XTPROTO_PROP_BASE_SIZE      "base_size"         /* scalar_t   */ /* Base size */
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
#define XTPROTO_PROP_OCTAVES        "octaves"           /* integer    */ /* Octave count */
#define XTPROTO_PROP_COUNT          "count"             /* integer    */ /* Count */
#define XTPROTO_PROP_MAJOR_RADIUS   "major_radius"      /* scalar_t   */ /* Major radius */
#define XTPROTO_PROP_MINOR_RADIUS   "minor_radius"      /* scalar_t   */ /* Minor radius */
#define XTPROTO_PROP_SPACING        "spacing"           /* scalar_t   */ /* Spacing */
#define XTPROTO_PROP_PROFILE        "profile"           /* group      */ /* Lathe profile */
#define XTPROTO_PROP_CAP_ENDS       "cap_ends"          /* bool       */ /* Cap ends */
#define XTPROTO_PROP_SPLINE         "spline"            /* group      */ /* Spline */
#define XTPROTO_PROP_END_A			"end_a"				/* scalar_t   */ /* End a */
#define XTPROTO_PROP_END_B			"end_b"				/* scalar_t   */ /* End b */
#define XTPROTO_PROP_VRTXDATA		"vecdata"			/* group      */ /* Vertex data */
#define XTPROTO_PROP_OBJ_GEO		"geometry"			/* asset_id_t */ /* Geometry id */
#define XTPROTO_PROP_OBJ_MAT		"material"			/* asset_id_t */ /* Material id */
#define XTPROTO_LTRL_TEXTURE  		"texture"			/* asset_id_t */ /* Texture id */
#define XTPROTO_NODE_ENVIRONMENT    "environment"       /* N/A        */ /* Environment */
#define XTPROTO_GRADIENT            "gradient"          /* N/A        */ /* Gradient */
#define XTPROTO_NODE_CAMERA			"camera"			/* N/A		  */ /* Resource node */
#define XTPROTO_NODE_MATERIAL		"material"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_TEXTURE		"texture"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_GEOMETRY		"geometry"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_OBJECT			"object"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_VARIANTS       "variants"          /* N/A        */ /* Scene variants node */
#define XTPROTO_NODE_VARIANT_SET    "set"               /* N/A        */ /* Variant set overlay */
#define XTPROTO_NODE_VARIANT_REMOVE "remove"            /* N/A        */ /* Variant remove overlay */
#define XTPROTO_LTRL_CAM_THINLENS   "thin-lens"         /* string     */ /* Perspective camera */
#define XTPROTO_LTRL_CAM_ODS        "ods"               /* string     */ /* Omni Directional Stereo camera */
#define XTPROTO_LTRL_CAM_ERP        "erp"               /* string     */ /* Equirectangular camera */
#define XTPROTO_LTRL_CAM_CUBEMAP    "cubemap"           /* string     */ /* Cubemap camera */
#define XTPROTO_TEXTURE             "texture"
#define XTPROTO_CUBEMAP             "cubemap"
#define XTPROTO_ERP                 "erp"
#define XTPROTO_COLOR               "color"
#define XTPROTO_GRAPHPAPER          "graphpaper"
#define XTPROTO_CHECKER             "checker"
#define XTPROTO_WEAVE               "weave"
#define XTPROTO_FBM_MARBLE          "fbm_marble"
#define XTPROTO_VORONOI_NORMAL      "voronoi_normal"
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

#endif /* XTCORE_PROTO_H_INCLUDED */
