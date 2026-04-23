# Third-Party Dependencies

This registry tracks vendored third-party code and where each dependency is used in the xtracer tree.

| Dependency | Local Path | Used In | Upstream | License | Notes |
|---|---|---|---|---|---|
| `cgltf` | `ext/cgltf.h`, `ext/cgltf.c` | `xtcore` | <https://github.com/jkuhlmann/cgltf> | MIT | glTF/GLB parsing for external asset import |
| `Crow` | `ext/crow/crow_all.h` | `xtracer-web` | <https://github.com/CrowCpp/Crow> | BSD-3-Clause | HTTP/WebSocket server framework used by `xtracer_web` |
| `pugixml` | `ext/pugixml/pugixml.hpp`, `ext/pugixml/pugixml.cpp` | `convertMitsuba`, `lib/nmesh` | <https://pugixml.org/> | MIT | XML parser used for Mitsuba conversion and SVG mesh parsing |
| `stb_image` | `ext/stb/stb_image.h` | `lib/nimg`, `xtcore` | <https://github.com/nothings/stb> | Public domain or MIT | Image decoding |
| `stb_image_write` | `ext/stb/stb_image_write.h` | `lib/nimg`, `xtracer-web` | <https://github.com/nothings/stb> | Public domain or MIT | Image encoding |
| `strpool` | `ext/strpool/strpool.h` | `xtcore`, `frontend/common`, `xtracer-web`, `xtracer-wasm` | <https://github.com/mattiasgustavsson/libs/blob/main/strpool.h> | MIT or public domain | String pool utility |
| `Three.js` | `src/frontend/web-client/vendor/three.min.js` | `xtracer-web` | <https://github.com/mrdoob/three.js> | MIT | Web 3D viewport and scene interaction |
| `TinyEXR` | `ext/tinyexr/tinyexr.h` | `lib/nimg` | <https://github.com/syoyo/tinyexr> | BSD-3-Clause with embedded OpenEXR code notices | EXR image support |
| `tinyobjloader` | `ext/tinyobj/tiny_obj_loader.h` | `lib/nmesh`, `xtcore` | <https://github.com/tinyobjloader/tinyobjloader> | MIT | OBJ/MTL parsing |
| `ufbx` | `ext/ufbx/ufbx.h`, `ext/ufbx/ufbx.c` | `xtcore` | <https://github.com/ufbx/ufbx> | MIT | FBX parsing for external asset import |

## Notes

- This file is the canonical dependency registry for vendored third-party code used by the repository.
- License texts are stored either alongside the vendored file, inside the vendored header/source, or both.
- When adding a new dependency, update this file and `README.md` in the same change.
