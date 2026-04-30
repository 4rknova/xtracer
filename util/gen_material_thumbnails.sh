#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_SERVER_BINARIES=(
  "$ROOT_DIR/build/intermediate/build/xtracer"
  "$ROOT_DIR/build/xtracer"
  "$ROOT_DIR/build/intermediate/build-release/xtracer"
)

SERVER_BIN=""
PORT="${XTRACER_MATERIAL_THUMB_PORT:-18096}"
PORT_EXPLICIT=0
WIDTH="${XTRACER_MATERIAL_THUMB_WIDTH:-256}"
HEIGHT="${XTRACER_MATERIAL_THUMB_HEIGHT:-256}"
SAMPLES="${XTRACER_MATERIAL_THUMB_SAMPLES:-128}"
THREADS="${XTRACER_MATERIAL_THUMB_THREADS:-0}"
TILE_SIZE="${XTRACER_MATERIAL_THUMB_TILE_SIZE:-32}"
SCENE_DIR="$ROOT_DIR/scene"
WEB_ROOT="$ROOT_DIR/src/apps/web-client"
OUT_DIR="$WEB_ROOT/res/lib/materials"
BASE_URL=""
MANAGE_SERVER=1
FORCE=0
ONLY_IDS_CSV=""

if [[ -n "${XTRACER_MATERIAL_THUMB_PORT:-}" ]]; then
  PORT_EXPLICIT=1
fi

usage() {
  cat <<EOF
Usage: $0 [options]

Options:
  --server-bin <path>   xtracer binary to launch
  --base-url <url>      Use an already-running xtracer instead of launching one
  --port <n>            Local port for the managed server (default: $PORT)
  --width <px>          Thumbnail width  (default: $WIDTH)
  --height <px>         Thumbnail height (default: $HEIGHT)
  --samples <n>         Samples per pixel for thumbnail renders (default: $SAMPLES)
  --threads <n>         Render thread count, 0=auto (default: $THREADS)
  --tile-size <n>       Tile size (default: $TILE_SIZE)
  --scene-dir <path>    Scene directory (default: $SCENE_DIR)
  --web-root <path>     Web root (default: $WEB_ROOT)
  --out-dir <path>      Output directory (default: $OUT_DIR)
  --only <ids>          Comma-separated material ids to render
  --force               Re-render thumbnails even if the PNG already exists
  --help                Show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --server-bin)
      SERVER_BIN="${2:?missing value for --server-bin}"
      shift 2
      ;;
    --base-url)
      BASE_URL="${2:?missing value for --base-url}"
      MANAGE_SERVER=0
      shift 2
      ;;
    --port)
      PORT="${2:?missing value for --port}"
      PORT_EXPLICIT=1
      shift 2
      ;;
    --width)
      WIDTH="${2:?missing value for --width}"
      shift 2
      ;;
    --height)
      HEIGHT="${2:?missing value for --height}"
      shift 2
      ;;
    --samples)
      SAMPLES="${2:?missing value for --samples}"
      shift 2
      ;;
    --threads)
      THREADS="${2:?missing value for --threads}"
      shift 2
      ;;
    --tile-size)
      TILE_SIZE="${2:?missing value for --tile-size}"
      shift 2
      ;;
    --scene-dir)
      SCENE_DIR="${2:?missing value for --scene-dir}"
      shift 2
      ;;
    --web-root)
      WEB_ROOT="${2:?missing value for --web-root}"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="${2:?missing value for --out-dir}"
      shift 2
      ;;
    --only)
      ONLY_IDS_CSV="${2:?missing value for --only}"
      shift 2
      ;;
    --force)
      FORCE=1
      shift
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

need_cmd curl
need_cmd ruby

if [[ -z "$BASE_URL" ]]; then
  if [[ -z "$SERVER_BIN" ]]; then
    for candidate in "${DEFAULT_SERVER_BINARIES[@]}"; do
      if [[ -x "$candidate" ]]; then
        SERVER_BIN="$candidate"
        break
      fi
    done
  fi

  if [[ -z "$SERVER_BIN" ]]; then
    echo "Could not find xtracer. Pass --server-bin or build one first." >&2
    exit 1
  fi

  BASE_URL="http://127.0.0.1:$PORT"
fi

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xtracer-material-thumbs.XXXXXX")"
SERVER_PID=""
SERVER_LOG="$TMP_DIR/xtracer.log"

cleanup() {
  if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" >/dev/null 2>&1; then
    kill "$SERVER_PID" >/dev/null 2>&1 || true
    wait "$SERVER_PID" >/dev/null 2>&1 || true
  fi
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT

show_server_log() {
  if [[ -f "$SERVER_LOG" ]]; then
    echo "--- xtracer log ---" >&2
    tail -n 80 "$SERVER_LOG" >&2 || true
  fi
}

wait_for_server() {
  local url="$1"
  local attempt
  for attempt in $(seq 1 80); do
    if [[ "$MANAGE_SERVER" -eq 1 ]] && [[ -n "$SERVER_PID" ]] && ! kill -0 "$SERVER_PID" >/dev/null 2>&1; then
      echo "xtracer exited during startup for $url" >&2
      show_server_log
      exit 1
    fi
    if curl -fsS "$url/api/materials" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.25
  done
  echo "Timed out waiting for xtracer at $url" >&2
  show_server_log
  exit 1
}

launch_managed_server() {
  local port_candidate="$1"
  : > "$SERVER_LOG"
  echo "Launching xtracer from $SERVER_BIN on port $port_candidate"
  "$SERVER_BIN" \
    --host 127.0.0.1 \
    --port "$port_candidate" \
    --scene-dir "$SCENE_DIR" \
    --web-root "$WEB_ROOT" \
    --gallery-dir "$TMP_DIR/gallery" \
    --max-concurrent-renders 1 \
    --render-reserve-threads 1 \
    >"$SERVER_LOG" 2>&1 &
  SERVER_PID="$!"
  sleep 0.25
  if kill -0 "$SERVER_PID" >/dev/null 2>&1; then
    PORT="$port_candidate"
    BASE_URL="http://127.0.0.1:$PORT"
    return 0
  fi
  wait "$SERVER_PID" >/dev/null 2>&1 || true
  SERVER_PID=""
  return 1
}

if [[ "$MANAGE_SERVER" -eq 1 ]] && [[ -z "$SERVER_PID" ]] && [[ -n "$SERVER_BIN" ]]; then
  if [[ "$PORT_EXPLICIT" -eq 1 ]]; then
    if ! launch_managed_server "$PORT"; then
      echo "Failed to launch xtracer on port $PORT" >&2
      show_server_log
      exit 1
    fi
  else
    launched=0
    for offset in $(seq 0 9); do
      candidate_port="$((PORT + offset))"
      if launch_managed_server "$candidate_port"; then
        launched=1
        break
      fi
    done
    if [[ "$launched" -ne 1 ]]; then
      echo "Failed to launch xtracer on ports $PORT-$((PORT + 9))" >&2
      show_server_log
      exit 1
    fi
  fi
fi

wait_for_server "$BASE_URL"

mkdir -p "$OUT_DIR"
MATERIALS_JSON="$TMP_DIR/materials.json"
curl -fsS "$BASE_URL/api/materials" -o "$MATERIALS_JSON"

MANIFEST="$TMP_DIR/materials.tsv"
ruby -rjson -e '
  data = JSON.parse(File.read(ARGV[0]))
  (data["materials"] || []).each do |m|
    puts m.fetch("id")
  end
' "$MATERIALS_JSON" > "$MANIFEST"

poll_job_done() {
  local job_id="$1"
  local state_line=""
  while true; do
    state_line="$(curl -fsS "$BASE_URL/api/jobs/$job_id" | ruby -rjson -e '
      snap = JSON.parse(STDIN.read)
      state = snap["state"].to_s
      progress = ((snap["progress"] || 0.0).to_f * 100.0).round
      error = snap["error"].to_s
      puts [state, progress, error].join("\t")
    ')"
    local state progress error
    IFS=$'\t' read -r state progress error <<< "$state_line"
    case "$state" in
      done)
        return 0
        ;;
      error|aborted)
        echo "Render $state for job $job_id: $error" >&2
        return 1
        ;;
      *)
        printf '  progress: %s%%\r' "$progress"
        sleep 0.5
        ;;
    esac
  done
}

count_selected() {
  local count=0
  local material_id=""
  while IFS= read -r material_id; do
    [[ -n "$material_id" ]] || continue
    if [[ -n "$ONLY_IDS_CSV" && ",$ONLY_IDS_CSV," != *",$material_id,"* ]]; then
      continue
    fi
    count=$((count + 1))
  done < "$MANIFEST"
  printf '%s\n' "$count"
}

total="$(count_selected)"
if [[ "$total" -eq 0 ]]; then
  echo "No matching materials to render."
  exit 0
fi
index=0

while IFS= read -r material_id; do
  [[ -n "$material_id" ]] || continue
  if [[ -n "$ONLY_IDS_CSV" && ",$ONLY_IDS_CSV," != *",$material_id,"* ]]; then
    continue
  fi
  index=$((index + 1))
  target_png="$OUT_DIR/$material_id.png"

  if [[ "$FORCE" -ne 1 && -f "$target_png" ]]; then
    echo "[$index/$total] skip $material_id"
    continue
  fi

  tmp_png="$TMP_DIR/$material_id.png"

  echo "[$index/$total] render $material_id"
  render_json="$(curl -fsS -X POST "$BASE_URL/api/materials/$material_id/preview" \
    --data-urlencode "width=$WIDTH" \
    --data-urlencode "height=$HEIGHT" \
    --data-urlencode "samples=$SAMPLES" \
    --data-urlencode "tm=aces" \
    --data-urlencode "tile_size=$TILE_SIZE" \
    --data-urlencode "threads=$THREADS")"
  job_id="$(printf '%s' "$render_json" | ruby -rjson -e 'puts JSON.parse(STDIN.read).fetch("job_id")')"

  poll_job_done "$job_id"
  printf '\n'

  curl -fsS "$BASE_URL/api/jobs/$job_id/export?format=png" -o "$tmp_png"
  mv "$tmp_png" "$target_png"
done < "$MANIFEST"

echo "Wrote thumbnails to $OUT_DIR"
