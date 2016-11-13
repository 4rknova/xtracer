![XTracer](https://cdn.rawgit.com/4rknova/xtracer/develop/res/logo.svg)

[![License](http://img.shields.io/:license-lgpl-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0-standalone.html)
[![Build Status](https://travis-ci.org/4rknova/xtracer.svg?branch=develop)](https://travis-ci.org/4rknova/xtracer)
[![Build Status](https://travis-ci.org/4rknova/xtracer.svg?branch=master)](https://travis-ci.org/4rknova/xtracer)

Copyright 2010 - 2016 (c)
Papadopoulos Nikos <nikpapas@gmail.com>

XTracer is a raytracer written in c and c++.

**Version 0.7** served as my thesis project for the BSc of computer
science at the University of Piraeus. It was developed under the
supervision of Prof. T. Panayiotopoulos as part of the research work
of the Knowledge Engineering Lab.

**Version 1.1** was developed as part of my dissertation for the MSc
of computer graphics programming at the University of Hull.

**Version 1.2** is the current version. All external dependencies have
been integrated.

The source code is distributed under the LGPL license version 3 or later.

Features
--------

Category      | Features
--------------|-----------
Primitives    | Plane, Triangle, Sphere, Mesh
Materials     | Lambert, Phong, Blinn-Phong
Lights        | Point, Sphere, Box, Triangle
Cameras       | Pinhole, Thin lense
Shading       | Reflection (glossy & perfect mirror), Refraction, Depth of field, Soft shadows, Texture mapping, Fresnel
Acceleration  | Threads, Octrees, KD-trees
Anti-aliasing | Supersampling
Rendering     | HDR, Whitted/Distributed raytracing, Photon mapping (diffuse term)

Compilation / Installation
--------------------------

XTracer should be able to compile and run on any system equipped with openmp
compatible c and c++ compilers. It has been tested without any problems under
the following platforms:

* Linux
* FreeBSD
* MacOSX
* MS Windows XP, 7

The source code is distributed along with a makefile configuration script for
unix based systems with a GNU toolchain installed. To view all the available
configuration options type: ./configure --help. Once the configuration is done,
you can build the binary by typing: make. To install the binary in your system
type: make install. To uninstall type: make uninstall

For windows users, a visual studio 2010 solution is available in the ide
directory.

The external dependencies are as follows:

### 0.x - 1.1.2.0

Library  | Source
---------|------------------------------------------
openmp   | Should be provided by your compiler.
libnmath | https://github.com/4rknova/libnmath
libnmesh | https://github.com/4rknova/libnmesh
linimg	 | https://github.com/4rknova/libnimg
libncf	 | https://github.com/4rknova/libncf

NOTE: You might need older versions of the libraries.

### 1.2.0.0 or later

Library  | Source
---------|------------------------------------------
openmp   | Should be provided by your compiler.

How to use
----------
You can find sample scenes in the scene directory. Note that some of the 
scenes require textures or meshes that are not included in the repository.

Contributions
-------------
I would like to thank the following people for their help and support.

### version 0.7

Name                     | Contributions
-------------------------|------------------------------------------------
Prof. T. Panayiotopoulos | Supervising professor at University of Piraeus.
Tsiombikas John          | Technical advisory.
Stea Maria Elenh         | Technical advisory.
Hardalias Apostolhs      | Windows testing, cpu time donation.
Kokkalis Nikos           | FreeBSD testing.
Ntzoufras Kwstas         | MacOSX testing, cpu time donation.
Potamianos Grhgorhs      | cpu time donation.
Costas Droggos           | cpu time donation.
Kouzouphs Antwnhs        | cpu time donation.

### version 1.0

Name                     | Contributions
-------------------------|------------------------------------------------
Dr Qingde Li             | Supervising professor at University of Hull
Tsiombikas John          | Technical advisory.
