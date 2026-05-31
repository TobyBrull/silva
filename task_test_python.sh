#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 0 ]; then
    echo "Usage: $0" >&2
    exit 1
fi

# Shunting Yard
python python/seed_axe_py/parser_shunting_yard.py

# Unicode table
python python/unicode_table_gen/main.py --workdir=var/ download
python python/unicode_table_gen/main.py --workdir=var/ generate --output-file-base var/fragmentization_data
diff cpp/syntax/fragmentization_data.hpp var/fragmentization_data.hpp
diff cpp/syntax/fragmentization_data.cpp var/fragmentization_data.cpp
