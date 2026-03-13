#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST_DIR="${1:-$ROOT_DIR/src/frontend/web-client/scenes}"
SCENE_DIR="${2:-$ROOT_DIR/scene}"

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

mkdir -p "$DEST_DIR"
find "$DEST_DIR" -maxdepth 1 -type f -name "*.scn" -delete

tmp_list="$(mktemp)"
trap 'rm -f "$tmp_list"' EXIT

copied_count=0
for s in "$SCENE_DIR"/*.scn; do
  if [[ ! -f "$s" ]]; then
    continue
  fi
  if is_self_contained_scene "$s"; then
    scene_name="$(basename "$s")"
    cp "$s" "$DEST_DIR/$scene_name"
    printf "%s\n" "$scene_name" >> "$tmp_list"
    copied_count=$((copied_count + 1))
  fi
done

echo "Synced $copied_count self-contained scene(s) to $DEST_DIR"
