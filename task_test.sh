#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <PRESET>" >&2
    exit 1
fi
PRESET=$1

BUILD_DIR="build.${PIXI_ENVIRONMENT_NAME}.${PRESET}"

cmake --preset "${PRESET}"
ninja -C "${BUILD_DIR}/"
ctest --test-dir "${BUILD_DIR}/" -j "$( nprocs )"
bash task_demo.sh "${BUILD_DIR}" > var/task_demo.sh.output
diff task_demo.sh.output var/task_demo.sh.output
