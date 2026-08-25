#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CC_BIN="${CC:-cc}"
UPGRADE_BIN="$ROOT/.ci_test_upgrade"

cd "$ROOT"
./tools/audit_protocol.sh

$CC_BIN -std=c89 -Wall -Wextra -Werror -pedantic -O2 -DPNG_DEC_USE_ZLIB -I"$ROOT" \
  "$ROOT/test_upgrade.c" "$ROOT"/png_*.c -lz -o "$UPGRADE_BIN"
"$UPGRADE_BIN"
rm -f "$UPGRADE_BIN"

./tests/run_smoke.sh
rm -f tests/roundtrip_smoke tests/repeat_smoke

./differential/run_local.sh
./fuzz/run_corpus_standalone.sh

echo "ALL_LOCAL_C89_PROTOCOL_TESTS_OK"
