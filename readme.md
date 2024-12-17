# Silva

Silva means "forest" in Latin.

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.


## Concepts

The Seed language is as a formal specification for the syntax of another language. A parser can
automatically be generated from a Seed program; such a parser is represented by the type
`silva::parse_root_t` ([parse_root.hpp](src/parse_root.hpp)).

The Fern language is akin to JSON; its syntax is described by the Seed program
`silva::fern_seed_source_code` ([fern.hpp](src/fern.hpp)).

The syntax of the Seed language is described in its own terms by the Seed program
`silva::seed_seed_source_code` ([seed.hpp](src/seed.hpp)).


## Building and Running

A recent C++ compiler is required. Clang 19 works for me.

```bash
rm -rf build/
cmake -S. -Bbuild/ -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
ninja -Cbuild/
./build/src/silva_test
./build/src/silva_tokenization filename=scratchpad/simple.fern
./build/src/silva_fern filename=scratchpad/simple.fern process=direct/string root-based=true
```
