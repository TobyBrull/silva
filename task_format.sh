#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -eq 1 ] && [ "$1" = "check" ]; then
    CLANG_FORMAT_ARGS=(--dry-run --Werror)
    CMAKE_FORMAT_ARGS=(--check)
    TAPLO_ARGS=(--check)
elif [ "$#" -eq 1 ] && [ "$1" = "update" ]; then
    CLANG_FORMAT_ARGS=(-i)
    CMAKE_FORMAT_ARGS=(-i)
    TAPLO_ARGS=()
else
    echo "Usage: $0 check|update" >&2
    exit 1
fi

# C++
git ls-files | grep "^src/.*\.[hctm]pp$" | xargs --verbose clang-format "${CLANG_FORMAT_ARGS[@]}"

# CMake
git ls-files | grep "CMakeLists.txt\|^cmake/" | grep -v "^.thirdparty/" | xargs --verbose cmake-format "${CMAKE_FORMAT_ARGS[@]}"

# TOML
taplo format "${TAPLO_ARGS[@]}"
