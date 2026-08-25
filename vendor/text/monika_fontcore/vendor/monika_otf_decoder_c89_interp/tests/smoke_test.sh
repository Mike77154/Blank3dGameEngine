#!/bin/sh
set -eu
make >/dev/null
./otf_dump "${1:-/usr/share/fonts/truetype/adf/AccanthisADFStd-Regular.otf}" 41 >/dev/null
if [ -f /usr/share/fonts/opentype/cantarell/Cantarell-VF.otf ]; then
  ./otf_dump /usr/share/fonts/opentype/cantarell/Cantarell-VF.otf 41 >/dev/null
fi
echo "smoke ok"
