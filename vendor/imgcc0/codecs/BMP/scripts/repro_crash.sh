#!/bin/sh
set -eu

MODE=${1:?usage: repro_crash.sh MODE HARNESS INPUT [extra args...]}
HARNESS=${2:?usage: repro_crash.sh MODE HARNESS INPUT [extra args...]}
INPUT=${3:?usage: repro_crash.sh MODE HARNESS INPUT [extra args...]}
shift 3

case "$MODE" in
  libfuzzer)
    exec "$HARNESS" -runs=1 "$INPUT" "$@"
    ;;
  file)
    exec "$HARNESS" "$INPUT" "$@"
    ;;
  stdin)
    exec sh -c '"$1" "$@" < "$2"' sh "$HARNESS" "$INPUT" "$@"
    ;;
  *)
    echo "error: unknown mode: $MODE" >&2
    exit 2
    ;;
esac
