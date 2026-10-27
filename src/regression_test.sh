#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <BUILD_DIR>" >&2
    exit 1
fi
BUILD_DIR=$1

TEMPFILE=$( mktemp )
trap 'rm -f "$TEMPFILE"' EXIT

# Doc demos
"./${BUILD_DIR}/src/silva_fragmentization" src/syntax/readme.fragmentization.demo
"./${BUILD_DIR}/src/silva_syntax" src/syntax/readme.calc.seed src/syntax/readme.calc

# src/zoo/
"./${BUILD_DIR}/src/silva_fern" src/zoo/fern/example.simple.fern
"./${BUILD_DIR}/src/silva_fern" src/zoo/fern/example.broken.fern 2>"$TEMPFILE" || true
cat "$TEMPFILE"
"./${BUILD_DIR}/src/silva_syntax" src/zoo/fern/example.simplest.fern
SEED_EXEC_TRACE=true "./${BUILD_DIR}/src/silva_syntax" src/zoo/fern/example.simplest.fern --action=none
#
"./${BUILD_DIR}/src/silva_lox" src/zoo/lox/lox.lox < src/zoo/lox/example.lox
"./${BUILD_DIR}/src/silva_syntax" src/zoo/c/{c.seed,example.c}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/python/{python.seed,example.python}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/bash/{bash.seed,example.bash}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/rust/{rust.seed,example.rust}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/toml/{toml.seed,example.toml}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/qbe/{qbe.seed,example.qbe}

# sandbox/
"./${BUILD_DIR}/src/silva_syntax" sandbox/soil.{seed,example.soil}
