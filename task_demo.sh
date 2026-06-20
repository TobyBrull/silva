#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <BUILD_DIR>" >&2
    exit 1
fi
BUILD_DIR=$1

TEMPFILE=$( mktemp )
trap 'rm -f "$TEMPFILE"' EXIT

# Simple parsing (including error message)
"./${BUILD_DIR}/cpp/silva_fragmentization" silva/syntax/01-simple.fern
"./${BUILD_DIR}/cpp/silva_fern" silva/syntax/01-simple.fern
"./${BUILD_DIR}/cpp/silva_fern" silva/syntax/01-broken.fern 2>"$TEMPFILE" || true
cat "$TEMPFILE"
"./${BUILD_DIR}/cpp/silva_syntax" silva/syntax/01-simplest.fern
SEED_EXEC_TRACE=true "./${BUILD_DIR}/cpp/silva_syntax" silva/syntax/01-simplest.fern --action=none

# Parsing user-defined languages
"./${BUILD_DIR}/cpp/silva_syntax" silva/syntax/02-example.silva
"./${BUILD_DIR}/cpp/silva_syntax" silva/syntax/03-somelang.seed silva/syntax/03-test.somelang
"./${BUILD_DIR}/cpp/silva_syntax" silva/soil/soil.silva silva/soil/example.silva

# Lox
"./${BUILD_DIR}/cpp/silva_lox" cpp/zoo/lox/lox.lox --use-interpreter=true < cpp/zoo/lox/example.lox
"./${BUILD_DIR}/cpp/silva_lox" cpp/zoo/lox/lox.lox --use-interpreter=false < cpp/zoo/lox/example.lox

# Cedar
"./${BUILD_DIR}/cpp/silva_cedar" cpp/zoo/cedar/test.cedar

# TOML
"./${BUILD_DIR}/cpp/silva_toml" cpp/zoo/toml/example.toml
