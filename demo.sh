#!/usr/bin/env bash

set -Eeuxo pipefail

TEMPFILE=$( mktemp )
trap 'rm -f "$TEMPFILE"' EXIT

./build/cpp/silva_tokenization silva/syntax/01-simple.fern

./build/cpp/silva_fern silva/syntax/01-simple.fern
./build/cpp/silva_fern silva/syntax/01-broken.fern 2>"$TEMPFILE" || true
cat "$TEMPFILE"
time ./build/cpp/silva_fern silva/syntax/01-large.fern --process=none --seed-engine=false
time ./build/cpp/silva_fern silva/syntax/01-large.fern --process=none --seed-engine=true

./build/cpp/silva_syntax silva/syntax/01-simplest.fern
SEED_EXEC_TRACE=true ./build/cpp/silva_syntax silva/syntax/01-simplest.fern --action=none

./build/cpp/silva_syntax silva/syntax/02-example.silva
./build/cpp/silva_syntax silva/syntax/03-somelang.seed silva/syntax/03-test.somelang

./build/cpp/silva_syntax silva/soil/soil.silva silva/soil/example.silva

VERBOSE=true ./build/cpp/silva_lox cpp/zoo/lox/example.lox 2>"$TEMPFILE" || true
cat "$TEMPFILE"

./build/cpp/silva_lox cpp/zoo/lox/lox.lox < cpp/zoo/lox/example.lox
