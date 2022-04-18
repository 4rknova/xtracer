<img align="center" src="https://raw.githubusercontent.com/4rknova/xtracer/develop/res/preview.jpg">

XTRACER
-------

Copyright 2010 (c) Nikos Papadopoulos [nikpapas@gmail.com]

[![Build Status](https://github.com/4rknova/xtracer/actions/workflows/ci.yml/badge.svg)](https://github.com/4rknova/xtracer/actions/workflows/ci.yml)

XTracer is an experimental rendering framework written in c and c++.

## Samples

You can find sample scenes in the scene directory. Note that some of the
scenes require textures or meshes that are not included in the repository.

Sample renders can be found in [this page](https://www.artstation.com/artwork/xkaGO).

## Features

* Renderers
    * Distributed ray-tracing
    * Depth
    * Stencil
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

## Compilation / Installation

Component    | Linux   | Windows | OSX     |
:------------|:-------:|:-------:|:-------:|
xtcore       |    X    |         |         |
frontend cli |    X    |         |         |
frontend gui |    X    |         |         |

Use the following commands to build:

    ./configure
    make

## Dependencies

Name          | License            | URL
--------------|--------------------|-----------------------------------------------------------------
ImGui         | MIT License        | https://github.com/ocornut/imgui
TinyObjLoader | MIT License        | https://github.com/syoyo/tinyobjloader
TinyFiles     | Public Domain      | https://github.com/RandyGaul/tinyheaders/blob/master/tinyfiles.h
STB           | Public Domain      | https://github.com/nothings/stb
TinyEXR       | 3-clause BSD       | https://github.com/syoyo/tinyexr
Remotery      | Apache License 2.0 | https://github.com/Celtoys/Remotery
strpool       | Public Domain      | https://github.com/mattiasgustavsson/libs

## License

<a href="http://opensource.org/licenses/BSD-3-Clause" target="_blank">
<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">
</a>

BSD 3-Clause License.

Please see License File for more information.
