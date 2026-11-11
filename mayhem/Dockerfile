# syntax=docker/dockerfile:1.7
#
# Commit image for esp32-camera, on top of the org C/C++ base.
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile --build-arg COMMIT=$(git rev-parse HEAD) -t mytag .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# esp_jpeg (the tjpgd-based JPEG decoder that img_converters.h builds on; upstream pulls it in through
# idf_component.yml `espressif/esp_jpeg ^1.3.1`), pinned to a commit of idf-extra-components and fetched
# once so build.sh stays offline. The commit hash and the file hashes below are the integrity pin.
USER root
ARG ESP_JPEG_VER=1.3.2
ARG ESP_JPEG_COMMIT=d9d4f733b89389576fe7e5f58ffc321b81468b37
RUN d=/opt/toolchains/deps/esp_jpeg-${ESP_JPEG_VER} && mkdir -p "$d/src" /tmp/dep-fetch \
 && cd /tmp/dep-fetch && git init -q . \
 && git fetch -q --depth 1 https://github.com/espressif/idf-extra-components.git "${ESP_JPEG_COMMIT}" \
 && git checkout -q FETCH_HEAD \
 && [ "$(git rev-parse HEAD)" = "${ESP_JPEG_COMMIT}" ] \
 && cp -r esp_jpeg/. "$d/src/" \
 && cd "$d/src" \
 && printf '%s  %s\n' \
      4a6fd227648227ec0ee6b486f0eb915fce6b94a5ce0d7fc2c68ab9d04aa0554d jpeg_decoder.c \
      789f62d6c5e7ddc1e09599a67c3686d840a2b27e44c4a33317d59d93b279ce1a tjpgd/tjpgd.c \
      3b242db7108427fd64792bb9fa139a59404787385bb94fb1e569695ba84a8b81 tjpgd/tjpgd.h \
      c70fffd7fbd6a95d5aff7513cf01073a7cecb4b61b4b4a860be5c3780a604be9 tjpgd/tjpgdcnf.h \
      e9e48c8644defe9037123797a72f4e3a029bdedd1eb487bb8542d3f04cab78eb include/jpeg_decoder.h \
    | sha256sum -c - \
 && cd / && rm -rf /tmp/dep-fetch && chmod -R a+rX "$d"
USER mayhem

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
