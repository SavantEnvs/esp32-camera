#!/usr/bin/env bash
#
# mayhem/build.sh — esp32-camera's image converters (conversions/) plus the esp_jpeg/tjpgd decoder they
# call, built for the host against stub ESP-IDF headers (mayhem/stubs/include). Runs in the commit image
# as `mayhem` in /mayhem; re-runnable offline (esp_jpeg comes from /opt/toolchains/deps).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

DEP=/opt/toolchains/deps/esp_jpeg-1.3.2/src
INC="-I$SRC/mayhem/stubs/include -I$SRC/conversions/include -I$SRC/conversions/private_include \
-I$SRC/driver/include -I$DEP/include -I$DEP/tjpgd -include $SRC/mayhem/shim/size32.h"
C_SOURCES="$SRC/conversions/yuv.c $SRC/conversions/to_bmp.c $DEP/jpeg_decoder.c $DEP/tjpgd/tjpgd.c"
CXX_SOURCES="$SRC/conversions/to_jpg.cpp $SRC/conversions/jpge.cpp"

# build_lib <dir> <flags...>: compile every library source with <flags> into <dir>/libconv.a
build_lib() {
  local dir="$1"; shift
  rm -rf "$dir"; mkdir -p "$dir"
  local objs=() src o extra
  for src in $C_SOURCES; do
    o="$dir/$(basename "${src%.c}").o"
    extra=""
    # to_bmp.c stores the 54-byte BMP header through a bmp_header_t* at output+2 (misaligned by design):
    # UBSan's `alignment` check fires on every successful conversion, so it is relaxed for this file.
    case "$src" in */to_bmp.c) [ -z "$SANITIZER_FLAGS" ] || extra="-fno-sanitize=alignment" ;; esac
    # tjpgd's fixed-point IDCT / dequantisation (block_idct, mcu_load) multiplies attacker-controlled coefficients by
    # the quantisation table in plain `int`; on corrupt input it wraps, and BYTECLIP's `& 0x3FF` index keeps the result
    # in bounds, so it only yields garbage pixels. It floods the defect list with line variants of one benign pattern,
    # so only UBSan's `signed-integer-overflow` is relaxed, for tjpgd.c alone (jpeg_decoder.c keeps it).
    case "$src" in */tjpgd.c) [ -z "$SANITIZER_FLAGS" ] || extra="-fno-sanitize=signed-integer-overflow" ;; esac
    $CC "$@" $extra $INC -c "$src" -o "$o"
    objs+=("$o")
  done
  for src in $CXX_SOURCES; do
    o="$dir/$(basename "${src%.cpp}").o"
    extra=""
    # jpge.cpp's forward DCT does `s0 << ROW_BITS` on negative ints (benign two's-complement shift, fires on
    # every encode), so only UBSan's `shift-base` check is relaxed for this file.
    case "$src" in */jpge.cpp) [ -z "$SANITIZER_FLAGS" ] || extra="-fno-sanitize=shift-base" ;; esac
    $CXX "$@" $extra $INC -c "$src" -o "$o"
    objs+=("$o")
  done
  ar rcs "$dir/libconv.a" "${objs[@]}"
}

# 1) the instrumented library: sanitizers + SanCov on the fuzzed code itself
build_lib "$SRC/build/fuzz" $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1

# 2) every harness twice: libFuzzer binary + standalone run-once reproducer
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -O1 -c "$SRC/mayhem/lsan_off.cc" -o "$SRC/build/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$SRC/build/standalone_main.o"
for t in jpg2bmp jpg2rgb fmt2rgb888 fmt2bmp fmt2jpg; do
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS -O1 $LIB_FUZZING_ENGINE $INC -c "$SRC/mayhem/harness_$t.c" -o "$SRC/build/harness_$t.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS -O1 $LIB_FUZZING_ENGINE "$SRC/build/harness_$t.o" "$SRC/build/lsan_off.o" \
       "$SRC/build/fuzz/libconv.a" -lm -o "/mayhem/fuzz_$t"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS -O1 $INC -c "$SRC/mayhem/harness_$t.c" -o "$SRC/build/harness_${t}_sa.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$SRC/build/standalone_main.o" "$SRC/build/harness_${t}_sa.o" "$SRC/build/lsan_off.o" \
       "$SRC/build/fuzz/libconv.a" -lm -o "/mayhem/fuzz_$t-standalone"
done

# 3) the oracle: a clean, unsanitized build of the same sources and the known-answer probe test.sh runs
build_lib "$SRC/build/test" -O2 $COVERAGE_FLAGS
$CC -O2 $COVERAGE_FLAGS $INC "$SRC/mayhem/kat.c" -o "$SRC/build/kat.o" -c
$CXX -O2 $COVERAGE_FLAGS "$SRC/build/kat.o" "$SRC/build/test/libconv.a" -lm -o /mayhem/kat_probe
