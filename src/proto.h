/*

    This file is part of xtracer.

    proto.h
    Configuration protocol definitions

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

#ifndef XTRACER_CFGPROTO_HPP_INCLUDED
#define XTRACER_CFGPROTO_HPP_INCLUDED

#define XT_CFGPROTO_PROP_NAME			"name"
#define XT_CFGPROTO_PROP_DESCRIPTION	"description"

/*
	NODES
*/

#define XT_CFGPROTO_NODE_CAMERA         "camera"
#define XT_CFGPROTO_NODE_LIGHT          "light"
#define XT_CFGPROTO_NODE_MATERIAL       "material"
#define XT_CFGPROTO_NODE_GEOMETRY       "geometry"
#define XT_CFGPROTO_NODE_OBJECT			"object"

/*
	COMPOSITE PROPERTIES
*/

#define XT_CFGPROTO_PROP_POSITION 		"position"
#define XT_CFGPROTO_PROP_TARGET   		"target"
#define XT_CFGPROTO_PROP_UP       		"up"
#define XT_CFGPROTO_PROP_NORMAL			"normal"

#define XT_CFGPROTO_PROP_VECDATA		"vecdata"

#define XT_CFGPROTO_PROP_INTENSITY		"intensity"

#define XT_CFGPROTO_PROP_AMBIENT		"ambient"
#define XT_CFGPROTO_PROP_SPECULAR		"specular"
#define XT_CFGPROTO_PROP_DIFFUSE		"diffuse"

/*
	PROPERTIES
*/

#define XT_CFGPROTO_PROP_DEFAULT		"default"
#define XT_CFGPROTO_PROP_SOURCE			"source"

#define XT_CFGPROTO_PROP_SMOOTH			"smooth"

#define XT_CFGPROTO_PROP_APERTURE		"aperture"
#define XT_CFGPROTO_PROP_SHUTTER		"shutter"
#define XT_CFGPROTO_PROP_FLENGTH		"flength"

#define XT_CFGPROTO_PROP_TYPE			"type"

#define XT_CFGPROTO_PROP_RADIUS			"radius"

#define XT_CFGPROTO_PROP_DISTANCE		"distance"

#define XT_CFGPROTO_PROP_FOV            "fov"

#define XT_CFGPROTO_PROP_REFLECTANCE	"reflectance"
#define XT_CFGPROTO_PROP_TRANSPARENCY	"transparency"

#define XT_CFGPROTO_PROP_GEOMETRY		"geometry"
#define XT_CFGPROTO_PROP_MATERIAL       "material"

#define XT_CFGPROTO_PROP_KAMBN			"k_ambient"
#define XT_CFGPROTO_PROP_KDIFF			"k_diffuse"
#define XT_CFGPROTO_PROP_KSPEC			"k_specular"
#define XT_CFGPROTO_PROP_KSEXP			"k_specexp"

#define XT_CFGPROTO_PROP_IOR			"ior"

#define XT_CFGPROTO_PROP_COORD_X        "x"
#define XT_CFGPROTO_PROP_COORD_Y        "y"
#define XT_CFGPROTO_PROP_COORD_Z        "z"

#define XT_CFGPROTO_PROP_COLOR_R		"r"
#define XT_CFGPROTO_PROP_COLOR_G		"g"
#define XT_CFGPROTO_PROP_COLOR_B		"b"
#define XT_CFGPROTO_PROP_COLOR_A		"a"

/*
	CONSTANT VALUES
*/

#define XT_CFGPROTO_VAL_TRUE			"true"
#define XT_CFGPROTO_VAL_FALSE			"false"

#define XT_CFGPROTO_VAL_SPHERE          "sphere"
#define XT_CFGPROTO_VAL_PLANE			"plane"
#define XT_CFGPROTO_VAL_TRIANGLE		"triangle"
#define XT_CFGPROTO_VAL_MESH			"mesh"

#define XT_CFGPROTO_VAL_LAMBERT			"lambert"
#define XT_CFGPROTO_VAL_PHONG			"phong"
#define XT_CFGPROTO_VAL_BLINNPHONG		"blinn_phong"

#endif /* XTRACER_CFGPROTO_HPP_INCLUDED  */
