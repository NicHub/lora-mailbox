#!/usr/bin/env bash

set -euo pipefail

HOST="mqtt.ouilogique.ch"
PORT="8883"
USER_NAME="guest"
PASSWORD="guest123"
TOPIC="mbx_nj/#"
LOG_FILE="$(date '+%Y-%m-%d_%H-%M-%S')_mqtt_bad_json_nogit.log"

usage() {
  cat <<'EOF'
Usage: mqtt-jq-safe.sh [options]

Subscribe to an MQTT topic, pretty-print valid JSON with jq, and log invalid
JSON payloads to a file without breaking the pipeline.

Options:
  -h HOST     MQTT host (default: mqtt.ouilogique.ch)
  -p PORT     MQTT port (default: 8883)
  -u USER     MQTT username (default: guest)
  -P PASS     MQTT password (default: guest123)
  -t TOPIC    MQTT topic (default: mbx_nj/#)
  -l FILE     Log file for invalid JSON (default: YYYY-MM-DD_HH-MM-SS_mqtt_bad_json_nogit.log)
  --help      Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h)
      HOST="${2:?missing value for -h}"
      shift 2
      ;;
    -p)
      PORT="${2:?missing value for -p}"
      shift 2
      ;;
    -u)
      USER_NAME="${2:?missing value for -u}"
      shift 2
      ;;
    -P)
      PASSWORD="${2:?missing value for -P}"
      shift 2
      ;;
    -t)
      TOPIC="${2:?missing value for -t}"
      shift 2
      ;;
    -l)
      LOG_FILE="${2:?missing value for -l}"
      shift 2
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown argument: %s\n\n' "$1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

export MQTT_JQ_SAFE_LOG="$LOG_FILE"

mosquitto_sub \
  -h "$HOST" \
  -p "$PORT" \
  -u "$USER_NAME" \
  -P "$PASSWORD" \
  -t "$TOPIC" |
python3 -u -c '
import binascii
import datetime
import json
import os
import sys

log_path = os.environ["MQTT_JQ_SAFE_LOG"]

with open(log_path, "a", encoding="utf-8") as log:
    for i, raw_line in enumerate(sys.stdin.buffer, 1):
        try:
            obj = json.loads(raw_line)
            print(json.dumps(obj), flush=True)
        except Exception as e:
            ts = datetime.datetime.now().isoformat()
            safe_text = raw_line.decode("utf-8", "backslashreplace")
            log.write(f"[{ts}] line={i} error={e}\n")
            log.write(f"raw_repr={safe_text!r}\n")
            log.write(f"raw_hex={binascii.hexlify(raw_line).decode()}\n\n")
            log.flush()
            print(
                f"[bad json logged] line={i}: {e} -> {log_path}",
                file=sys.stderr,
                flush=True,
            )
' |
jq -S .
