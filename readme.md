# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.

## Concepts

The Seed sub-language allows users to define PEG parsers. Fern shows what Seed looks like for a very
simple language.

* [Fern](cpp/zoo/fern/fern.hpp): A bit like JSON, but simpler.
  ([Example](silva/syntax/01-simple.fern))
* [Seed](cpp/syntax/seed.hpp): The Seed language defined in itself.
    * Expressions are parsed with a variant of the Shunting Yard algorithm, described by
      [seed_axe.hpp](cpp/syntax/seed_axe.hpp).
    * Some Seed global definitions are in [seed.globals.hpp](cpp/syntax/seed.globals.hpp).

## Zoo

Example parsers for existing languages. More or less faithful.

* [Lox](cpp/zoo/lox/lox.hpp): The toy language from the book "Crafting Interpreters".
  ([Example](cpp/zoo/lox/example.lox))
* [C](cpp/zoo/c/c.seed) ([Example](cpp/zoo/c/example.c))
* [Python](cpp/zoo/python/python.seed) ([Example](cpp/zoo/python/example.python))
* [Bash](cpp/zoo/bash/bash.seed) ([Example](cpp/zoo/bash/example.bash))
* [Rust](cpp/zoo/rust/rust.seed) ([Example](cpp/zoo/rust/example.rust))
* [TOML](cpp/zoo/toml/toml.seed) ([Example](cpp/zoo/toml/example.toml))

## Development

Requires [Pixi](https://pixi.prefix.dev/latest/#installation).

```bash
pixi run test-all && echo "ALL TESTS PASSED!"

eval "$( pixi shell-hook )"

cmake --preset "debug"

ninja -C "build/" && time "build/cpp/silva_test"
bash task_format_check.sh && echo "ALL FORMATTING OKAY!"
bash task_format.sh
bash task_test.sh "debug" && echo "ALL TESTS PASSED!"
bash task_test_python.sh && echo "ALL PYTHON TESTS PASSED!"
```

## Packaging

```bash
pixi publish --target-dir=var/
```
