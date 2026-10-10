# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.

## Concepts

The Seed sub-language allows users to define PEG parsers. Fern shows what Seed looks like for a very
simple language.

* [Fern](src/zoo/fern/fern.hpp): A bit like JSON, but simpler.
  ([Example](silva/syntax/01-simple.fern))
* [Seed](src/syntax/seed.hpp): The Seed language defined in itself.
    * Expressions are parsed with a variant of the Shunting Yard algorithm, described by
      [seed_axe.hpp](src/syntax/seed_axe.hpp).
    * Some Seed global definitions are in [seed.globals.hpp](src/syntax/seed.globals.hpp).

## Zoo

Example parsers for existing languages. More or less faithful.

* [Lox](src/zoo/lox/lox.hpp): The toy language from the book "Crafting Interpreters".
  ([Example](src/zoo/lox/example.lox))
* [C](src/zoo/c/c.seed) ([Example](src/zoo/c/example.c))
* [Python](src/zoo/python/python.seed) ([Example](src/zoo/python/example.python))
* [Bash](src/zoo/bash/bash.seed) ([Example](src/zoo/bash/example.bash))
* [Rust](src/zoo/rust/rust.seed) ([Example](src/zoo/rust/example.rust))
* [TOML](src/zoo/toml/toml.seed) ([Example](src/zoo/toml/example.toml))

## Development

Requires [Pixi](https://pixi.prefix.dev/latest/#installation).

```bash
pixi run test-all

eval "$( pixi shell-hook )"

cmake --preset release

ninja -C "build/" && time "build/src/silva_test"
bash task_format.sh check
bash task_format.sh update
bash task_test.sh release && echo "ALL TESTS PASSED!"
bash task_test_tools.sh && echo "ALL PYTHON TESTS PASSED!"
```

## Packaging

```bash
pixi publish --target-dir=tmp/
```
