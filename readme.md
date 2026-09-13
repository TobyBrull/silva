# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.


## Concepts

The Seed sub-language allows users to define PEG parsers.

Implemented parsers:
* [Fern](cpp/zoo/fern/fern.hpp): A bit like JSON, but simpler.
    * [Example](silva/syntax/01-simple.fern).
* [Seed](cpp/syntax/seed.hpp): The Seed language defined in itself.
    * For expression parsing a shunting yard algorithm is used that's described by [seed_axe.hpp](cpp/syntax/seed_axe.hpp).
    * Some Seed global definitions are in [seed.globals.hpp](cpp/syntax/seed.globals.hpp).
* [Cedar](cpp/zoo/cedar/cedar.seed): Basically preprocessed C.
    * [Example](cpp/zoo/cedar/example.cedar).
* [Pine](cpp/zoo/pine/pine.seed): Basically Python (without pattern matching).
    * [Example](cpp/zoo/pine/example.pine).
* [Ash](cpp/zoo/ash/ash.seed): A language that looks a bit like Bash. Vibe coded.
    * [Example](cpp/zoo/ash/example.ash).
* [Tomel](cpp/zoo/tomel/tomel.seed): Basically Tom's Obvious, Minimal Language.
    * [Example](cpp/zoo/tomel/example.tomel).
* [Lox](cpp/zoo/lox/lox.hpp): The toy language from the book "Crafting Interpreters".
    * [Example](cpp/zoo/lox/example.lox).


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
