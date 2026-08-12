# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.


## Concepts

The Seed language allows users to define PEG parsers.

Implemented Seed parsers:
* [Fern](cpp/zoo/fern/fern.hpp): A bit like JSON, but simpler.
* [Seed](cpp/syntax/seed.hpp): The Seed language defined in itself. For expression parsing a
  shunting yard algorithm is used that's described by [seed_axe.hpp](cpp/syntax/seed_axe.hpp).
* [Cedar](cpp/zoo/cedar/cedar.hpp): Basically preprocessed C.
* [TOML](cpp/zoo/toml/toml.hpp): Tom's Obvious, Minimal Language.
* [Lox](cpp/zoo/lox/lox.hpp): The toy language from the book "Crafting Interpreters".


## Development

Requires [Pixi](https://pixi.prefix.dev/latest/#installation).

```bash
pixi run test-all && echo "ALL TESTS SUCCEEDED!"

eval "$( pixi shell-hook )"

PRESET=debug    ; BUILD_DIR="build.default.${PRESET}/"
PRESET=release  ; BUILD_DIR="build.default.${PRESET}/"
PRESET=tracy    ; BUILD_DIR="build.default.${PRESET}/"

rm -rf "${BUILD_DIR}"
cmake --preset "${PRESET}"
ninja -C "${BUILD_DIR}" && time "${BUILD_DIR}/cpp/silva_test"
bash task_format_check.sh && echo "ALL FORMATTING OKAY!"
bash task_format.sh
bash task_test.sh "${PRESET}" && echo "ALL TESTS PASSED!"
bash task_test_python.sh && echo "ALL PYTHON TESTS PASSED!"
```


## Packaging

```bash
pixi publish --target-dir=var/
```
