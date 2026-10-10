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
ctest --test-dir "${BUILD_DIR}/" -j "$( nproc )"
mkdir -p tmp/
bash regression_test.sh "${BUILD_DIR}" > tmp/regression_test.sh.output
diff regression_test.sh.output tmp/regression_test.sh.output
