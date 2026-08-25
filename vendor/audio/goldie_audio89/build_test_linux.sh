#!/bin/sh
set -eu
CC=${CC:-cc}
OUT=build_test
rm -rf "$OUT"
mkdir -p "$OUT"
V=vendor
KNM="$V/knm_audio_refactor_cc0_v030_phase3_plus/KNM_audio_refactor_phase3"
RAW="$V/rawmix_phase5_dawstyle_giffany/rawmix"
COMMON="-std=c89 -pedantic -Wall -Wextra -Werror -Iruntime/include -I$RAW/include -I$KNM/KNM_audio -I$V/audiocodecs/mwav89/include -I$V/audio/mpcm89/include -I$V/audiocodecs/mp3_frame89/include"
SRC="runtime/src/goldie_audio89.c $RAW/src/rawmix.c $KNM/KNM_audio/knm_hwr_audio.c $KNM/KNM_audio/mnk_core.c $KNM/knm_backends/mka_audio_null.c $V/audiocodecs/mwav89/src/mwav89.c $V/audio/mpcm89/src/mpcm89.c $V/audiocodecs/mp3_frame89/src/mp3_frame89.c"
$CC $COMMON $SRC tests/test_null.c -o "$OUT/test_null"
"$OUT/test_null"
$CC $COMMON $SRC tests/test_wav_decode.c -o "$OUT/test_wav_decode"
"$OUT/test_wav_decode"
$CC $COMMON $SRC tests/test_matryoshka_mix.c -o "$OUT/test_matryoshka_mix"
"$OUT/test_matryoshka_mix"
