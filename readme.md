# Silva

Silva is Latin for "forest".

Silva is meant to be language-oriented programming language or, equivalently, a ecosystem of related
programming languages.

The Seed language describes the grammar of other languages in a way that can be automatically turned
into a parser for those languages. A parser generated from a Seed program is represented by the type
"parse_root_t". A "parse_root_t" can be used to turn a stream of tokens into a "parse_tree_t".

The Fern language is similar to JSON. It is described by the following Seed program in the string
`silva::fern_seed` (file fern.hpp).
