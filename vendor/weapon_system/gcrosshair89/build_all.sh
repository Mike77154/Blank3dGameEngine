#!/bin/sh
set -e
for d in gcrosshair_core89 gcrosshair_params89 gcrosshair_base89 gcrosshair_provider89 gcrosshair_anim89 gcrosshair_runtime89; do
  echo "== $d =="
  (cd "$d" && make clean && make && make test)
done
