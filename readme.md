# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.


## Concepts

The Seed language is as a formal specification for the syntax of another language. A parser can
automatically be generated from a Seed program; such a parser is represented by the type
`silva::parse_root_t` ([parse_root.hpp](cpp/parse_root.hpp)).

The Fern language is akin to JSON; its syntax is described by the Seed program
`silva::fern_seed_source_code` ([fern.hpp](cpp/fern.hpp)).

The syntax of the Seed language is described in its own terms by the Seed program
`silva::seed_seed_source_code` ([seed.hpp](cpp/seed.hpp)).

The syntax of the Silva language is described by the Seed program
`silva::silva_seed_source_code` ([silva.hpp](cpp/silva.hpp)).

## Building and Running

A recent C++ compiler is required. Clang 19 works for me.

```bash
rm -rf build/
cmake -S. -Bbuild/ -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
ninja -Cbuild/
./build/cpp/silva_test
./build/cpp/silva_tokenization filename=silva/fern/simple.fern
./build/cpp/silva_fern filename=silva/fern/simple.fern process=direct/string root-based=false
./build/cpp/silva_fern filename=silva/fern/simple-broken.fern process=direct/string root-based=true
./build/cpp/silva_seed silva/seed/test.seed silva/seed/test.code
./build/cpp/silva_silva scratchpad/test-01.silva
./build/cpp/silva_silva scratchpad/std.silva
```
