<img align="center" src="https://raw.githubusercontent.com/4rknova/xtracer/develop/res/preview.jpg">

XTRACER
-------

Copyright 2010 (c) Nikos Papadopoulos [nikpapas@gmail.com]

XTracer is an experimental rendering framework written in c and c++.


[![CI](https://github.com/4rknova/xtracer/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/4rknova/xtracer/actions/workflows/ci.yml)

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

Or with CMake:

    cmake -S . -B build -DXTRACER_ENABLE_GUI=OFF -DXTRACER_ENABLE_WEB=ON
    cmake --build build -j

## Web Frontend (Simple)

Build target: `xtracer_web`

Run from repo root:

    ./bin/xtracer_web --host 127.0.0.1 --port 8080 --scene-dir scene --web-root res/web

Open:

    http://127.0.0.1:8080

Web UI includes:

- Render tab: scene/integrator/camera selection and render preview.
  - Progressive preview updates while render jobs are running.
- Editor tab: inspect selected scene source, create a new scene, and save as `.scn`.
- Settings tab: theme and frontend-only options.
- Logs tab: backend log stream with clear action.
- About tab: version, author metadata, and full project license text.

Web API includes:

- `GET /api/health`
- `GET /api/about`
- `GET /api/scenes`
- `GET /api/scenes/{scene}/cameras`
- `GET /api/scenes/{scene}/source`
- `POST /api/scenes/save`
- `GET /api/integrators`
- `POST /api/render`
- `GET /api/jobs/{id}`
- `GET /api/jobs/{id}/image` (`?final=1` for final-only image)
- `GET /api/logs?since=<id>`

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
