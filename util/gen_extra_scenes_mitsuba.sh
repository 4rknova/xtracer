#!/usr/bin/env bash
set -euo pipefail

RESOURCES_URL="https://benedikt-bitterli.me/resources/"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CONVERTER="$ROOT_DIR/build/intermediate/build/convertMitsuba"
SCENE_DIR="$ROOT_DIR/scene"
DOWNLOAD_DIR="${TMPDIR:-/tmp}/xtracer-mitsuba-downloads"
LOG_FILE="$DOWNLOAD_DIR/gen_extra_scenes_mitsuba.log"

timestamp() {
  date "+%Y-%m-%d %H:%M:%S"
}

log() {
  local message="$1"
  printf '[%s] %s\n' "$(timestamp)" "$message" | tee -a "$LOG_FILE"
}

require_command() {
  local cmd="$1"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Missing required command: $cmd" >&2
    exit 1
  fi
}

fetch_url() {
  local url="$1"
  if command -v curl >/dev/null 2>&1; then
    curl -fsSL "$url"
    return
  fi
  if command -v wget >/dev/null 2>&1; then
    wget -qO- "$url"
    return
  fi
  echo "Missing required command: curl or wget" >&2
  exit 1
}

download_file() {
  local url="$1"
  local output_path="$2"
  if command -v curl >/dev/null 2>&1; then
    curl -fL --retry 3 -o "$output_path" "$url"
    return
  fi
  wget -O "$output_path" "$url"
}

decode_html_text() {
  local text="$1"
  text="${text//&amp;/&}"
  text="${text//&quot;/\"}"
  text="${text//&#39;/\'}"
  text="${text//&apos;/\'}"
  text="${text//&lt;/<}"
  text="${text//&gt;/>}"
  printf '%s' "$text"
}

extract_mitsuba3_scene_records() {
  perl -0ne '
    my $base = "https://benedikt-bitterli.me/resources/";

    while (/<div class="scene-block">.*?<div class="name-author">\s*<div><a\b[^>]*>(.*?)<\/a><\/div>\s*<div>by <a\b[^>]*>(.*?)<\/a><\/div>.*?<a class="scene-download" href=(["\x27])([^"\x27]*mitsuba\/[^"\x27]+\.zip(?:\?[^"\x27]*)?)\3[^>]*>.*?Mitsuba(?:\s*3)?\s*<\/a>/gsi) {
      my ($name, $author, $url) = ($1, $2, $4);

      for ($name, $author) {
        s/<[^>]+>//g;
        s/&amp;/&/g;
        s/&quot;/"/g;
        s/&#39;/'\''/g;
        s/&apos;/'\''/g;
        s/&lt;/</g;
        s/&gt;/>/g;
        s/\s+/ /g;
        s/^\s+|\s+$//g;
      }

      if ($url !~ m{^[a-z][a-z0-9+.-]*://}i) {
        if ($url =~ m{^/}) {
          $url = "https://benedikt-bitterli.me" . $url;
        } else {
          $url = $base . $url;
        }
      }

      print "$url\t$name\t$author\n";
    }
  ' | awk -F '\t' '!seen[$1]++'
}

if [[ ! -x "$CONVERTER" ]]; then
  echo "Converter binary not found or not executable: $CONVERTER" >&2
  echo "Build it first, for example:" >&2
  echo "  cmake -S \"$ROOT_DIR\" -B \"$ROOT_DIR/build/intermediate/build\"" >&2
  echo "  cmake --build \"$ROOT_DIR/build/intermediate/build\" --target convertMitsuba -j" >&2
  exit 1
fi

require_command unzip
require_command perl
mkdir -p "$DOWNLOAD_DIR"
mkdir -p "$SCENE_DIR"
rm -f "$LOG_FILE"

mapfile -t scene_records < <(fetch_url "$RESOURCES_URL" | extract_mitsuba3_scene_records)

if [[ "${#scene_records[@]}" -eq 0 ]]; then
  echo "No Mitsuba 3 scene zip links found at $RESOURCES_URL" >&2
  exit 1
fi

total_scenes="${#scene_records[@]}"

log "Found $total_scenes Mitsuba 3 scene archive(s)"
log "Downloading archives into: $DOWNLOAD_DIR"
log "Writing detailed log to: $LOG_FILE"

for idx in "${!scene_records[@]}"; do
  IFS=$'\t' read -r scene_url scene_name scene_author <<< "${scene_records[$idx]}"
  zip_name="$(basename "${scene_url%%\?*}")"
  zip_path="$DOWNLOAD_DIR/$zip_name"
  current=$((idx + 1))
  percent=$((current * 100 / total_scenes))
  scene_label="$(decode_html_text "$scene_name") by $(decode_html_text "$scene_author")"
  comment=$'Sourced from Benedikt Bitterli'\''s Rendering Resources page\nurl: '"$scene_url"$'\n'"$scene_label"

  log "[$current/$total_scenes][$percent%] Starting: $zip_name"
  log "[$current/$total_scenes][$percent%] Scene: $scene_label"
  log "[$current/$total_scenes][$percent%] Source URL: $scene_url"
  log "[$current/$total_scenes][$percent%] Downloading to: $zip_path"
  download_file "$scene_url" "$zip_path"

  log "[$current/$total_scenes][$percent%] Converting: $zip_path"
  "$CONVERTER" "$zip_path" -d "$SCENE_DIR" -c "$comment"
  log "[$current/$total_scenes][$percent%] Finished: $zip_path"
done

log "Completed $total_scenes/$total_scenes scene archive(s)"
log "Finished converting Mitsuba 3 scenes into: $SCENE_DIR"
