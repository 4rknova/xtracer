[![logo](https://cdn.rawgit.com/4rknova/xtracer/develop/res/logo.svg)]()

Copyright 2010 - 2017 (c) Nikos Papadopoulos [nikpapas@gmail.com]

[![License](http://img.shields.io/:license-lgpl-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0-standalone.html)
[![Build Status](https://travis-ci.org/4rknova/xtracer.svg?branch=develop)](https://travis-ci.org/4rknova/xtracer)
[![Build Status](https://travis-ci.org/4rknova/xtracer.svg?branch=master)](https://travis-ci.org/4rknova/xtracer)
[![Beerpay](https://beerpay.io/4rknova/xtracer/badge.svg?style=beer-square)](https://beerpay.io/4rknova/xtracer)
[![Beerpay](https://beerpay.io/4rknova/xtracer/make-wish.svg?style=flat-square)](https://beerpay.io/4rknova/xtracer?focus=wish)

XTracer is a raytracer written in c and c++.

Samples
-------
[![xtracer sample render](http://www.4rknova.com/xtracer/image/samples/0001.png)]()
[![xtracer sample render](http://www.4rknova.com/xtracer/image/samples/gi_2.png)]()
[![xtracer sample render](http://www.4rknova.com/xtracer/image/samples/gi_4.png)]()
[![xtracer sample render](http://www.4rknova.com/xtracer/image/samples/gi_5.png)]()

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
compatible c and c++ compilers. 

The source code is distributed along with a makefile configuration script for
unix based systems with a GNU toolchain installed. To view all the available
configuration options type: ./configure --help. Once the configuration is done,
you can build the binary by typing: make -j. To install the binary in your system
type: make install. To uninstall type: make uninstall

How to use
----------
You can find sample scenes in the scene directory. Note that some of the 
scenes require textures or meshes that are not included in the repository.
