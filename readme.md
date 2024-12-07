# Silva

Silva means "forest" in Latin.

Silva aspires to be a language-oriented programming language, or alternatively, an ecosystem or
library comprising a suite of interconnected programming languages.

The Seed language describes the grammar of another language. A program in the Seed language can be
automatically turned into a parser. A parser generated from a Seed program is represented by the
type `silva::parse_root_t`.

The Seed language is as a formal specification for the syntax of another language. A parser can
automatically be generated from a Seed program; such a parser is represented by the type
`silva::parse_root_t` ([parse_root.hpp](src/parse_root.hpp)).

The Fern language is akin to JSON. It is described by the Seed program
`silva::fern_seed_source_code` ([fern.hpp](src/fern.hpp)).


## Building and Running

A recent C++ compiler is required. Clang 19 works for me.

```bash
cmake -S. -Bbuild/ -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
ninja -Cbuild/
./build/silva_test
./build/silva_tokenization -f scratch/simple.fern
./build/silva_fern -f scratch/simple.fern -r -p direct/string
```
