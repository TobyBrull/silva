#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 0 ]; then
    echo "Usage: $0" >&2
    exit 1
fi

# C++
git ls-files | grep "^cpp/.*\.[hctm]pp$" | xargs --verbose clang-format -i

# CMake
git ls-files | grep "CMakeLists.txt\|^cmake/" | grep -v "^.thirdparty/" | xargs --verbose cmake-format -i

# TOML
taplo format
