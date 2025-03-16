# Silva

Silva aspires to be an [esoteric](https://en.wikipedia.org/wiki/Esoteric_programming_language),
[language-oriented](https://en.wikipedia.org/wiki/Language-oriented_programming) programming
language.


## Concepts

The Seed language is as a formal specification for the syntax of another language.

The Fern language is akin to JSON. Its syntax is described by the Seed program
`silva::fern_seed` ([fern.hpp](cpp/fern.hpp)).

The syntax of the Seed language itself is described by the Seed program
`silva::seed_seed` ([seed.hpp](cpp/seed.hpp)).

A parser can automatically be derived from a Seed program.
This is implemented in the class
`silva::seed_engine_t` ([seed_engine.hpp](cpp/seed_engine.hpp)).


## Building and Running

A recent C++ compiler is required. Clang 19 works for me.

```bash
conda create --name silva-clang
conda install --name silva-clang --channel conda-forge cmake catch2=3 fmt boost
conda install --name silva-clang --channel conda-forge llvm=19 lldb=19 llvm-tools=19
conda install --name silva-clang --channel conda-forge clang=19 clangxx=19 clang-tools=19
conda install --name silva-clang --channel conda-forge compiler-rt=19 compiler-rt_linux-64=19
```

```bash
rm -rf build/
cmake -S. -Bbuild/ -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
ninja -Cbuild/
ninja -Cbuild/ && ./build/cpp/silva_test
ninja -Cbuild/ && ./build/cpp/silva_tokenization filename=silva/fern/simple.fern
ninja -Cbuild/ && ./build/cpp/silva_fern filename=silva/fern/simple.fern process=direct/string root-based=false
ninja -Cbuild/ && ./build/cpp/silva_fern filename=silva/fern/broken.fern process=direct/string root-based=true
ninja -Cbuild/ && ./build/cpp/silva_seed silva/seed/test.seed
ninja -Cbuild/ && ./build/cpp/silva_seed silva/seed/test.seed silva/seed/test.code
ninja -Cbuild/ && ./build/cpp/silva_seed scratchpad/silva.seed scratchpad/test-01.silva
ninja -Cbuild/ && ./build/cpp/silva_seed scratchpad/silva.seed scratchpad/std.silva
```
