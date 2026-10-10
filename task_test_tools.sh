#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 0 ]; then
    echo "Usage: $0" >&2
    exit 1
fi

# Shunting Yard
python tools/seed_axe_py/parser_shunting_yard.py

# Unicode table
python tools/unicode_table_gen/main.py --workdir=tmp/ download
python tools/unicode_table_gen/main.py --workdir=tmp/ generate --output-file-base tmp/fragmentization_data
diff src/syntax/fragmentization_data.hpp tmp/fragmentization_data.hpp
diff src/syntax/fragmentization_data.cpp tmp/fragmentization_data.cpp

# C tests
rm -rf tmp/wacct/ tmp/c_tests/ && python tools/c_tests/run.py setup
python tools/c_tests/run.py run-tests --output-file-list tmp/failed.txt
if [ "$(wc -l < tmp/failed.txt)" -gt 3 ]; then
    echo "More than three C tests failed, see tmp/failed.txt" >&2
    exit 1
fi
