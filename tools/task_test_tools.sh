#!/usr/bin/env bash

set -Eeuxo pipefail

if [ "$#" -ne 0 ]; then
    echo "Usage: $0" >&2
    exit 1
fi

# seed_axe_py/
python tools/seed_axe_py/parser_shunting_yard.py

# unicode_table_gen/
python tools/unicode_table_gen/main.py --workdir=tmp/ download
python tools/unicode_table_gen/main.py --workdir=tmp/ generate --output-file-base tmp/fragmentization_data
diff src/syntax/fragmentization_data.hpp tmp/fragmentization_data.hpp
diff src/syntax/fragmentization_data.cpp tmp/fragmentization_data.cpp

# zoo_tests/
rm -rf tmp/zoo_tests/
python tools/zoo_tests/run.py --language=c --work-dir=tmp/zoo_tests setup
python tools/zoo_tests/run.py --language=c --work-dir=tmp/zoo_tests run-tests
python tools/zoo_tests/run.py --language=qbe --work-dir=tmp/zoo_tests setup
python tools/zoo_tests/run.py --language=qbe --work-dir=tmp/zoo_tests run-tests

echo "ALL PYTHON TESTS PASSED!"
