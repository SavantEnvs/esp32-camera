#!/usr/bin/env bash
#
# mayhem/test.sh — run the known-answer probe (/mayhem/kat_probe, built by build.sh from the unsanitized
# conversions + esp_jpeg sources) and compare every reported value with the answer worked out by hand:
# RGB565/grayscale/YUYV packing, BMP header layout, JPEG pixel content from an independent encoder
# (Pillow), decoder buffer-size contracts. A probe that is replaced by `exit 0` prints nothing and fails.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

PROBE="$SRC/kat_probe"
if [ ! -x "$PROBE" ]; then
  echo "kat_probe missing: mayhem/build.sh should have built it" >&2
  emit_ctrf "esp32-camera-kat" 0 1 0
  exit 1
fi

OUT="$("$PROBE" 2>&1)"
echo "$OUT"

passed=0; failed=0
check() {
  local key="$1" want="$2" got
  got="$(printf '%s\n' "$OUT" | sed -n "s/^${key}=//p" | head -n1)"
  if [ "$got" = "$want" ]; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL $key: want '$want' got '$got'" >&2
  fi
}

le32() { printf '%02x%02x%02x%02x' $(($1 & 255)) $((($1 >> 8) & 255)) $((($1 >> 16) & 255)) $((($1 >> 24) & 255)); }
le16() { printf '%02x%02x' $(($1 & 255)) $((($1 >> 8) & 255)); }
# bmp_header <file size> <pixel offset> <width> <height, drawn top-down> <bits per pixel> <image size>
bmp_header() {
  printf '424d%s%s%s%s%s%s%s%s%s%s%s%s%s%s' "$(le32 "$1")" 00000000 "$(le32 "$2")" "$(le32 40)" "$(le32 "$3")" \
    "$(le32 $((-$4)))" "$(le16 1)" "$(le16 "$5")" 00000000 "$(le32 "$6")" "$(le32 2835)" "$(le32 2835)" 00000000 00000000
}

# fmt2rgb888: RGB565 pixels F800, 07E0, 001F -> bytes (b,g,r); grayscale; YUYV (Y0=0 Y1=255, neutral chroma)
check rgb565_to_rgb888 0000f800fc00f80000
check gray_to_rgb888 121212abababee
check yuyv_to_rgb888 000000ffffffee
# the bounded variant refuses (and writes nothing past) a buffer smaller than 3 bytes per pixel
check bounded_gray_5 0
check bounded_gray_6 1
check bounded_gray_6_out 121212abababee

# fmt2bmp: 'BM', size, 0, pixel offset, DIB size 40, width, -height, planes 1, bpp, 0, image size, 2835 dpi x2
check bmp_rgb888_len 60
check bmp_rgb888 "$(bmp_header 60 54 2 1 24 6)010203040506"
check bmp_gray_len 1080
check bmp_gray_header "$(bmp_header 1080 1078 2 1 8 2)"
check bmp_gray_pal0 00000000
check bmp_gray_pal1 01010100
check bmp_gray_pal255 ffffff00
check bmp_gray_pixels 7f80
check bmp_rgb565_len 57
check bmp_rgb565 "$(bmp_header 57 54 1 1 24 3)0000f8"

# JPEG decode (16x8 grayscale: left half black, right half white)
check jpg_gray_bmp_len 438
check jpg_gray_bmp_header "$(bmp_header 438 54 16 8 24 384)"
check jpg_gray_px_0_0 000000
check jpg_gray_px_7_0 000000
check jpg_gray_px_8_0 ffffff
check jpg_gray_px_15_0 ffffff
check jpg_gray_px_0_7 000000
check jpg_gray_px_15_7 ffffff
# 16x16 4:4:4 colour JPEG: left half pure primary, right half the other, at both ends of the image
check jpg_rb_bmp_len 822
check jpg_rb_left_pure 1
check jpg_rb_right_pure 1
check jpg_rb_sides_differ 1
# jpg2rgb565 / fmt2rgb888 output-size contracts and scaling
check jpg_gray_565 1
check jpg_gray_565_x0 0000
check jpg_gray_565_x8 ffff
check jpg_gray_565_small 0
check jpg_gray_565_half 1
check jpg_gray_565_half_x0 0000
check jpg_gray_565_half_x4 ffff
check jpg_gray_888_small 0
check jpg_gray_888 1
check jpg_gray_888_x7 000000
check jpg_gray_888_x8 ffffff
check jpg_garbage 0
# encoder: output is a JPEG (SOI..EOI) that decodes back to the two input halves
check enc_len_ok 1
check enc_soi ffd8
check enc_eoi ffd9
check enc_decode 1
check enc_roundtrip_left_primary 1
check enc_roundtrip_right_primary 1
check enc_roundtrip_sides_differ 1

emit_ctrf "esp32-camera-kat" "$passed" "$failed" 0
