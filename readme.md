# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.


## Concepts

The Seed language is as a formal specification for the syntax of another language.

The Fern language is akin to JSON. Its syntax is described by the Seed program
`silva::fern::seed_str` ([fern.hpp](cpp/zoo/fern/fern.hpp)).

The syntax of the Seed language itself is described by the Seed program
`silva::seed::seed_str` ([seed.hpp](cpp/syntax/seed.hpp)) and
`silva::seed::seed_axe_str` ([seed_axe.hpp](cpp/syntax/seed_axe.hpp)).

A parser can automatically be derived from a Seed program.
This is implemented in the class
`silva::seed::interpreter_t` ([seed_interpreter.hpp](cpp/syntax/seed_interpreter.hpp)).


## Testing

```bash
pixi run test-all && echo "ALL TESTS SUCCEEDED!"
```


## Development

```bash
pixi run format

eval "$( pixi shell-hook )"

PRESET=debug
PRESET=release
PRESET=tracy
BUILD_DIR="build.default.${PRESET}/"
ln -sfn "${BUILD_DIR}" build

rm -rf "${BUILD_DIR}"
cmake --preset "${PRESET}"
ninja -C "${BUILD_DIR}" && time "${BUILD_DIR}/cpp/silva_test"
ninja -C "${BUILD_DIR}" && bash task_demo.sh "${BUILD_DIR}" > task_demo.sh.output && git status task_demo.sh.output
```


## Release

```bash
pixi publish --target-dir=var/
```
