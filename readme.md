# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.

## Module Readmes

* [syntax](src/syntax/readme.md)

## Zoo

Made-up example language:

* [Fern](src/zoo/fern/fern.hpp): A bit like JSON, but simpler. ([Example](src/zoo/fern/example.simple.fern))

Parsers for existing languages. More or less faithful.

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

ninja -C "build/" && time build/src/silva_test
bash tools/task_format.sh check
bash tools/task_format.sh update
bash tools/task_test.sh release
bash tools/task_test_tools.sh
```

## Packaging

```bash
pixi publish --target-dir=tmp/
```
