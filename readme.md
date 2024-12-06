# Silva

Silva is Latin for "forest".

Silva is imagined to be language-oriented programming language or, equivalently, an ecosystem or
library of related programming languages.

The Seed language describes the grammar of another language. A program in the Seed language can be
automatically turned into a parser. A parser generated from a Seed program is represented by the
type `silva::parse_root_t`.

The Fern language is akin to JSON. It is described by the following Seed program
`silva::fern_seed_source_code` (in file [fern.hpp](src/fern.hpp)).


## Building and Running

A recent C++ compiler is required.

```bash
cmake -S. -Bbuild/ -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
ninja -Cbuild/
./build/silva_test
./build/silva_tokenization -f scratch/simple.fern
./build/silva_fern -f scratch/simple.fern -r -p direct/string
```
