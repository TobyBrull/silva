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

# Simple parsing (including error message)
"./${BUILD_DIR}/src/silva_fern" silva/syntax/01-simple.fern
"./${BUILD_DIR}/src/silva_fern" silva/syntax/01-broken.fern 2>"$TEMPFILE" || true
cat "$TEMPFILE"
"./${BUILD_DIR}/src/silva_syntax" silva/syntax/01-simplest.fern
SEED_EXEC_TRACE=true "./${BUILD_DIR}/src/silva_syntax" silva/syntax/01-simplest.fern --action=none

# Parsing user-defined languages
"./${BUILD_DIR}/src/silva_syntax" silva/syntax/02-example.silva
"./${BUILD_DIR}/src/silva_syntax" silva/syntax/03-somelang.seed silva/syntax/03-test.somelang
"./${BUILD_DIR}/src/silva_syntax" silva/soil/soil.silva silva/soil/example.silva

# Zoo

"./${BUILD_DIR}/src/silva_lox" src/zoo/lox/lox.lox < src/zoo/lox/example.lox

"./${BUILD_DIR}/src/silva_syntax" src/zoo/c/{c.seed,example.c}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/python/{python.seed,example.python}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/bash/{bash.seed,example.bash}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/rust/{rust.seed,example.rust}
"./${BUILD_DIR}/src/silva_syntax" src/zoo/toml/{toml.seed,example.toml}
