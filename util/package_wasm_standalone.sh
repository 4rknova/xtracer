#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WEB_DIR="$ROOT_DIR/src/frontend/web-client"
SCENE_DIR="$ROOT_DIR/scene"
DIST_DIR="${1:-$ROOT_DIR/dist-wasm}"
PREVIEW_SRC="$ROOT_DIR/res/preview.jpg"
LOGO_SRC="$ROOT_DIR/res/logo.png"
LICENSE_SRC="$ROOT_DIR/LICENSE"

echo "Packaging standalone WASM frontend into: $DIST_DIR"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/scenes"

required_files=(
  "index.html"
  "app.js"
  "styles.css"
  "wasm_adapter.js"
  "wasm_worker.js"
  "integrators.json"
  "resolutions.json"
  "xtracer_wasm.js"
  "xtracer_wasm.wasm"
)

for f in "${required_files[@]}"; do
  if [[ ! -f "$WEB_DIR/$f" ]]; then
    echo "Missing required file: $WEB_DIR/$f"
    echo "Build WASM first with:"
    echo "  emcmake cmake -S . -B build/intermediate/build-wasm -DXTRACER_ENABLE_WEB=OFF -DXTRACER_ENABLE_RTMIDI=OFF -DXTRACER_ENABLE_WASM=ON"
    echo "  cmake --build build/intermediate/build-wasm -j --target xtracer_wasm"
    exit 1
  fi
done

if [[ ! -f "$PREVIEW_SRC" ]]; then
  echo "Missing required file: $PREVIEW_SRC"
  exit 1
fi

if [[ ! -f "$LOGO_SRC" ]]; then
  echo "Missing required file: $LOGO_SRC"
  exit 1
fi

if [[ ! -f "$LICENSE_SRC" ]]; then
  echo "Missing required file: $LICENSE_SRC"
  exit 1
fi

cp "$WEB_DIR/index.html" "$DIST_DIR/"
cp "$WEB_DIR/app.js" "$DIST_DIR/"
cp "$WEB_DIR/styles.css" "$DIST_DIR/"
cp "$WEB_DIR/wasm_adapter.js" "$DIST_DIR/"
cp "$WEB_DIR/wasm_worker.js" "$DIST_DIR/"
cp "$WEB_DIR/integrators.json" "$DIST_DIR/"
cp "$WEB_DIR/resolutions.json" "$DIST_DIR/"
cp "$WEB_DIR/xtracer_wasm.js" "$DIST_DIR/"
cp "$WEB_DIR/xtracer_wasm.wasm" "$DIST_DIR/"
cp "$PREVIEW_SRC" "$DIST_DIR/preview.jpg"
cp "$LOGO_SRC" "$DIST_DIR/logo.png"
cp "$LICENSE_SRC" "$DIST_DIR/license.txt"

"$ROOT_DIR/util/sync_wasm_scenes.sh" "$DIST_DIR/scenes" "$SCENE_DIR"
copied_count="$(find "$DIST_DIR/scenes" -maxdepth 1 -type f -name "*.scn" | wc -l | tr -d ' ')"

echo "Done."
echo "Packaged self-contained scenes: $copied_count"
echo "Run locally:"
echo "  cd \"$DIST_DIR\" && python3 -m http.server 8080"
echo "Open:"
echo "  http://127.0.0.1:8080/?backend=wasm"
