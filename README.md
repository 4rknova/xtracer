XTRACER
-------
Copyright 2010 (c) Nikos Papadopoulos [nikpapas@gmail.com]
[![Build Status](https://img.shields.io/travis/4rknova/xtracer/master.svg?label=master)](https://travis-ci.org/4rknova/xtracer)
[![Build Status](https://img.shields.io/travis/4rknova/xtracer/develop.svg?label=develop)](https://travis-ci.org/4rknova/xtracer)

XTracer is a renderer written in c and c++.

Features
--------
* Renderers
    * Distributed ray-tracing
    * Ray traced depth
    * Ray traced Stencil
* Primitives
    * Plane
    * Triangle
    * Sphere
    * Mesh
* Materials
    * Lambert
    * Phong
    * Blinn-Phong
* Light sources
    * Point
    * Sphere
    * Box
    * Triangle
    * Mesh
* Cameras
    * Pinhole
    * Thin lens
* Acceleration
    * Threading
    * Octrees
    * KD-trees
* Anti-Aliasing
    * Multi Sampling

Compilation / Installation
--------------------------

XTracer should be able to compile and run on any system equipped with openmp
compatible c and c++ compilers and cmake. The source code is distributed along
with a makefile configuration script for unix based systems with a GNU toolchain
installed. Use the following commands:

    ./configure
    make

How to use
----------
You can find sample scenes in the scene directory. Note that some of the
scenes require textures or meshes that are not included in the repository.

External Libraries
------------------
Name          | URL
--------------|-----------
ImGui         | https://github.com/ocornut/imgui
TinyObjLoader | https://github.com/syoyo/tinyobjloader
TinyFiles     | https://github.com/RandyGaul/tinyheaders/blob/master/tinyfiles.h
STB           | https://github.com/nothings/stb
Remotery      | https://github.com/Celtoys/Remotery

License (BSD 3-clause)
----------------------
<a href="http://opensource.org/licenses/BSD-3-Clause" target="_blank">
<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">
</a>

    BSD 3-Clause License

    Copyright (c) 2010, Nikos Papadopoulos
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
