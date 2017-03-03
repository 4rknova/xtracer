#ifndef XTRACER_PROTO_H_INCLUDED
#define XTRACER_PROTO_H_INCLUDED

/*
**	SYMBOLIC NAME					LITERAL				TYPE			 DESCRIPTION
*/
#define XTPROTO_FORMAT_VEC3         "vec3(%f,%f,%f)"    /* string     */ /* Format for vec3 */
#define XTPROTO_FORMAT_COL3         "col3(%f,%f,%f)"    /* string     */ /* Format for col3 */
#define XTPROTO_FORMAT_TEX2         "tex2(%f,%f)"       /* string     */ /* Format for tex2 */
#define XTPROTO_FORMAT_EXTERNAL     "ext(%[a-zA-Z)"     /* string     */ /* format for external file */
#define XTPROTO_FORMAT_GENERATE     "gen(%[a-zA-Z])"    /* string     */ /* Format for generate function */

#define XTPROTO_PROP_TITLE			"title"				/* string     */ /* Scene title */
#define XTPROTO_PROP_DESCR			"descr"				/* string     */ /* Scene description */
#define XTPROTO_PROP_VERSN			"versn"				/* string     */ /* Scene version */

#define XTPROTO_LTRL_ICOSAHEDRON    "icosahedron"       /* string     */ /* Icosahedron */

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
#define XTPROTO_PROP_SCALARS        "scalars"           /* string     */ /* scalar properties */
#define XTPROTO_PROP_COLORS         "colors"            /* string     */ /* color properties */
#define XTPROTO_PROP_TEXTURES       "textures"          /* string     */ /* texture properties */

#define XTPROTO_PROP_TYPE			"type"				/* string	  */ /* Type */
#define XTPROTO_PROP_DEFAULT		"default"			/* string     */ /* Default value */
#define XTPROTO_PROP_POSITION		"position"			/* Vector3f   */ /* Translation */
#define XTPROTO_PROP_TEXCOORD		"texcoord"			/* Vector2f   */ /* Texture coordinates */
#define XTPROTO_PROP_SOURCE			"source"			/* string     */ /* External source file */
#define XTPROTO_PROP_FOV			"fov"				/* scalar_t   */ /* Field of view */
#define XTPROTO_PROP_APERTURE		"aperture"			/* scalar_t   */ /* Aperture of the lense */
#define XTPROTO_PROP_TARGET			"target"			/* Vector3f   */ /* Target position */
#define XTPROTO_PROP_FLENGTH		"flength"			/* scalar_t   */ /* Focal length */
#define XTPROTO_PROP_ORIENTATION    "orientation"		/* Vector3f   */ /* Orientation */
#define XTPROTO_PROP_INTST			"intensity"			/* ColorRGBf  */ /* Color intensity */
#define XTPROTO_PROP_IAMBN			"ambient"			/* ColorRGBf  */ /* Ambient intensity */
#define XTPROTO_PROP_IDIFF			"diffuse"			/* ColorRGBf  */ /* Diffuse intensity */
#define XTPROTO_PROP_ISPEC			"specular"			/* ColorRGBf  */ /* Specular intensity */
#define XTPROTO_PROP_EMISSIVE       "emissive"          /* ColorRGBf  */ /* Emissive intensity */
#define XTPROTO_PROP_KAMBN			"k_ambient"			/* scalar_t   */ /* Ambient ratio */
#define XTPROTO_PROP_KDIFF			"k_diffuse"			/* scalar_t   */ /* Diffuse ratio */
#define XTPROTO_PROP_KSPEC			"k_specular"		/* scalar_t   */ /* Specular ratio */
#define XTPROTO_PROP_KEXPN			"k_exponent"		/* scalar_t   */ /* Specular exponent */
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
#define XTPROTO_LTRL_PLANE			"plane"				/* string     */ /* Plane */
#define XTPROTO_LTRL_TRIANGLE		"triangle"			/* string     */ /* Triangle */
#define XTPROTO_LTRL_POINT			"point"	    		/* string     */ /* Point */
#define XTPROTO_LTRL_SPHERE			"sphere"			/* string     */ /* Sphere */
#define XTPROTO_LTRL_HULL			"hull"  			/* string     */ /* Hull */
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
#define XTPROTO_PROP_END_A			"end_a"				/* scalar_t   */ /* End a */
#define XTPROTO_PROP_END_B			"end_b"				/* scalar_t   */ /* End b */
#define XTPROTO_PROP_VRTXDATA		"vecdata"			/* group      */ /* Vertex data */
#define XTPROTO_PROP_OBJ_GEO		"geometry"			/* asset_id_t */ /* Geometry id */
#define XTPROTO_PROP_OBJ_MAT		"material"			/* asset_id_t */ /* Material id */

#define XTPROTO_LTRL_TEXTURE  		"texture"			/* asset_id_t */ /* Texture id */

#define XTPROTO_NODE_CUBEMAP    	"cubemap"			/* N/A        */ /* Cubemap */
#define XTPROTO_NODE_CAMERA			"camera"			/* N/A		  */ /* Resource node */
#define XTPROTO_NODE_MATERIAL		"material"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_TEXTURE		"texture"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_GEOMETRY		"geometry"			/* N/A        */ /* Resource node */
#define XTPROTO_NODE_OBJECT			"object"			/* N/A        */ /* Resource node */

#define XTPROTO_LTRL_CAM_THINLENS   "thin-lens"         /* string     */ /* Perspective camera */
#define XTPROTO_LTRL_CAM_ODS        "ods"               /* string     */ /* Omni Directional Stereo camera */
#define XTPROTO_LTRL_CAM_ERP        "erp"               /* string     */ /* Equirectangular camera */

#define XTPROTO_TEXTURE         "texture"
#define XTPROTO_CUBEMAP         "cubemap"
#define XTPROTO_COLOR           "color"
#define XTPROTO_PROPERTIES      "properties"
#define XTPROTO_SAMPLERS        "samplers"
#define XTPROTO_SCALARS         "scalars"
#define XTPROTO_VALUE           "value"
#define XTPROTO_DIRECTIONAL_UVS "directional_uvs"
#define XTPROTO_FLIP_NORMALS    "flip_normals"
#define XTPROTO_MODIFIERS       "modifiers"
#define XTPROTO_EXTRUDE         "extrude"
#define XTPROTO_MULTIPLIER      "multiplier"

#endif /* XTRACER_PROTO_H_INCLUDED */
