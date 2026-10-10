#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 0 ]; then
    echo "Usage: $0" >&2
    exit 1
fi

# Shunting Yard
python tools/seed_axe_py/parser_shunting_yard.py

# Unicode table
python tools/unicode_table_gen/main.py --workdir=var/ download
python tools/unicode_table_gen/main.py --workdir=var/ generate --output-file-base var/fragmentization_data
diff src/syntax/fragmentization_data.hpp var/fragmentization_data.hpp
diff src/syntax/fragmentization_data.cpp var/fragmentization_data.cpp

# C tests
rm -rf var/wacct/ var/c_tests/ && python tools/c_tests/run.py setup
python tools/c_tests/run.py run-tests --output-file-list var/failed.txt
if [ "$(wc -l < var/failed.txt)" -gt 3 ]; then
    echo "More than three C tests failed, see var/failed.txt" >&2
    exit 1
fi
