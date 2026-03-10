#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WEB_DIR="$ROOT_DIR/res/web"
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
    echo "  emcmake cmake -S . -B build-wasm -DXTRACER_ENABLE_GUI=OFF -DXTRACER_ENABLE_WEB=OFF -DXTRACER_ENABLE_RTMIDI=OFF -DXTRACER_ENABLE_WASM=ON"
    echo "  cmake --build build-wasm -j --target xtracer_wasm"
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

is_self_contained_scene() {
  local scene_path="$1"
  perl -0777 -e '
    my $p = shift @ARGV;
    open my $fh, "<", $p or exit 1;
    local $/;
    my $t = <$fh>;
    close $fh;

    $t =~ s/#.*$//mg;

    if ($t =~ /\bpath_[A-Za-z0-9_]*\s*=/i
      || $t =~ /=\s*<[^>\n]+>\s*\/[^\s#]+/i
      || $t =~ /^\s*source\s*=\s*(?!gen\s*\()[^\n#]*\//im
      || $t =~ /^\s*source\s*=\s*(?!gen\s*\()[^\n#]*\.(obj|ply|fbx|gltf|glb|hdr|exr|png|jpg|jpeg|bmp|tga)\b/im
      || $t =~ /\bext\s*\(/i) {
      exit 1;
    }
    exit 0;
  ' "$scene_path"
}

copied_count=0
for s in "$SCENE_DIR"/*.scn; do
  if is_self_contained_scene "$s"; then
    cp "$s" "$DIST_DIR/scenes/"
    copied_count=$((copied_count + 1))
  fi
done

if [[ $copied_count -eq 0 ]]; then
  echo "No self-contained scenes were found to package for WASM."
  exit 1
fi

{
  echo "{"
  echo "  \"scenes\": ["
  i=0
  for s in "$DIST_DIR"/scenes/*.scn; do
    name="$(basename "$s")"
    if [[ $i -gt 0 ]]; then
      echo "    ,\"$name\""
    else
      echo "    \"$name\""
    fi
    i=$((i + 1))
  done
  echo "  ]"
  echo "}"
} > "$DIST_DIR/scenes/index.json"

echo "Done."
echo "Packaged self-contained scenes: $copied_count"
echo "Run locally:"
echo "  cd \"$DIST_DIR\" && python3 -m http.server 8080"
echo "Open:"
echo "  http://127.0.0.1:8080/?backend=wasm"
