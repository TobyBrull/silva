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


## Building and Running

A recent C++ compiler is required. Clang 19 works for me.

```bash
conda create --name silva-clang
conda install --name silva-clang --channel conda-forge \
    cmake ccache catch2=3 fmt boost llvm=19 lldb=19 llvm-tools=19 clang=19 \
    clangxx=19 clang-tools=19 compiler-rt=19 compiler-rt_linux-64=19
eval "$(conda shell.bash hook)"
conda activate silva-clang

rm -rf build/
cmake -S . -B build/ -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
ninja -C build/ && ./build/cpp/silva_test
ninja -C build/ && bash demo.sh > demo.sh.output && git status
```
