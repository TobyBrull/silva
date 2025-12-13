# Design Ideas

All input files must be in UTF-8.


## Text Fragmentation (Pre-Tokenization)

Input: UTF-8 string. Output: Vector of fragments.

Fragmentation is fixed across all languages in Silva.

By 'XID_Start' and 'XID_Continue' here we mean the Unicode derived core properties with the same
name.

* [comment] find comments that cover most languages: C-style (/**/), C++-style (//), and
Python-style comments (#).
* [string-literal] find strings in such a way that covers 99% of string uses in across all major
programming languages (Python, C++, ..., but also Zig's multi-line literals). This part could also
be made to support Python-style f-strings.
* The content of string-literals and comments is together called the "non-semantic" part of an input
file. String-literals and comments are called non-semantic fragments. The following, on the other
hand, are called "semantic" fragments, forming the semantic part of the input file.
* [identifier-name] XID_Start XID_Continue*.
* [number-literal] everything that starts with [0-9] followed by XID_Continue.
* [operator-char] Every unicode code-point that has the derived core property Math but is not also
in XID_Continue.
* [whitespace,indent,newline] Only space and newline are allowed. Works mostly like Python, with the
equivalent of INDENT and DEDENT (which are at this point fragments rather than tokens), but at this
stage there is only a single NEWLINE fragment (the equivalent of the Python distinction between NL
and NEWLINE happens in tokenization).
* Any other Unicode code-point not explicitly allowed by any of the semantic fragments or any
sequence that's not in NFC in the semantic part means that the input file is ill-formed.


## Tokenization

Input: Vector of fragments. Output: Vector of tokens.

Tokenization can be configured for each language mainly via the Peat language. Peat has somewhat
similar function to lex/flex/re2c/ragel, but it's not based on regular expressions and is much
simpler. It's basically a configuration language.


## Parsing

Input: Vector of tokens. Output: a parse-tree.

The "parse-tree" is the core data-structure of Silva. More specifically, by "parse-tree" a
data-structure equivalent to the C++ data-structure `silva::parse_tree` is meant.

The Seed language allows to quickly define recursive descent parsers (plus shunting-yard). Where
this not enough, parser should be amendable with aribtrary user code.

* A parse-tree can always be converted back to a source file that, if parsed, would result in the
same parse-tree.
* Silva allows transformations of parse-trees. For example, a parse-tree representing a C program
can be transformed into a parse-tree representing an assmbly program with the same semantics.
* The parse-tree data-structure is built directly into Silva. One can define literals that are
parse-trees. Silva variables can holds parse-trees. Silva functions can take and return parse-trees.


## Miscellaneous Desirable Features

* Deduction of language to be parsed.
