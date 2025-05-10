#!/usr/bin/env bash

set -Eeuxo pipefail

rm -rf build/
cmake -S . -B build/ -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
ninja -C build/

./build/cpp/silva_test

./build/cpp/silva_tokenization silva/syntax/01-simple.fern

./build/cpp/silva_fern silva/syntax/01-simple.fern
./build/cpp/silva_fern silva/syntax/01-broken.fern || true
time ./build/cpp/silva_fern silva/syntax/01-large.fern --process=none --seed-engine=false
time ./build/cpp/silva_fern silva/syntax/01-large.fern --process=none --seed-engine=true

./build/cpp/silva_syntax silva/syntax/01-simplest.fern
SEED_EXEC_TRACE=true ./build/cpp/silva_syntax silva/syntax/01-simplest.fern --action=none

./build/cpp/silva_syntax silva/syntax/02-test.silva
./build/cpp/silva_syntax silva/syntax/03-somelang.seed silva/syntax/03-test.somelang
