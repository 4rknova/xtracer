/*

	This file is part of xtracer.

	xtracer.h
	Xtracer

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

/*
	Environmental options

	The command line accepts the following options.

	OPTION              DESCRIPTION

	RENDERING MODES

	-interactive        The images will be rendered in an sdl window.

	-mode               Available modes:
						local   -	Stand alone renderer without networking.
						master  -	Master node accepts incoming connections and coordinates rendering.
						slave %s-	Slave node connects to a master node and accepts rendering tasks.
									A host must be provided.

	-drv                Available drivers:
						sdl     -	Render to SDL window.
						img %s  -	Render to image file.
						asc     - 	Render to ascii ( outputs to console ).

	-depth  %i          The maximum recursion depth given as an integer %i.

	-buffer %ix%i       The screen buffer dimmensions given as an integer pair formatted as %ix%i.

	-version, -v, -ver  Outputs version info and exits. This argument cannot be combined with others.

	-port               This is only used in master or slave mode and is ignored in all other modes. 
						If it's not provided then default value is used.
*/

#ifndef XTRACER_H_INCLUDED
#define XTRACER_H_INCLUDED

#endif /* XTRACER_H_INCLUDED  */

