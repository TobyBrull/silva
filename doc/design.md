# Design

## Tokenization

As a language-oriented programming language, Silva allows to nest one language in another language.
To indicate one language inside another, Silva mainly uses the double-angle quotation marks.

```
auto x = Json « { "Hello": "World" } »
```

We want to allow some flexibility in terms of the tokenization of different languages. For example,
we want to allow languages in which identation is part of the tokenization (like in Python) and
languages in which it is not (like in C). This implies that nested languages can only be tokenized
once the outer language has decided what language is expected in the inner language. This then
raises the question where to start and stop the first tokenization of the outer language.

Although we want *some* flexibility of the tokenization, we're also happy to accept certain
invariants between different tokenization; for example, in `x = "Hello"` the `"Hello"` bit will
always be a string literal. For this reason, Silva uses the concept of "fragmentation" (or
pre-tokenization), which expresses a common denominator with respect to tokenization between all
languages that Silva supports. Fragmentation also already checks that all "parentheses" are
nested/well-formed in the standard way. This nesting structure can then be used to define nested
languages.

The nature of fragmentation, as described below, means that Silva can't parse Rust, for example,
because Rust uses `'a` as lifetime annotation, but this would always be fragmented as the beginning
of a string in Silva.

## Text Fragmentation (Pre-Tokenization)

All input files are assumed to be in UTF-8.

Input: UTF-8 string. Output: Vector of fragments.

Fragmentation is fixed across all languages in Silva.

By 'XID_Start' and 'XID_Continue' here we mean the Unicode derived core properties with the same
name.

* [whitespace] Only space and newlines are allowed; no tabs; no carriage-return. Some of those are
then interpreted under the "indent" and "newline" rubriks below, others are "genuine whitespace".
* [comment] This covers C-style (/**/), C++-style (//), and Python-style comments (#).
* [string] Allows a wide range of string literals of C++ and Python. Also supports Zig's multi-line
literals. This part could also be made to support Python-style f-strings.
* The content of string-literals, comments, and genuine whitespace together is called the
"non-semantic" part of an input file. String-literals, comments, and real whitespace are called
non-semantic fragments. The following, on the other hand, are called "semantic" fragments, forming
the semantic part of the input file. Also, a '\\' at the end of a line (unless in string) will be
treated as a line continuation.
* [identifier] XID_Start XID_Continue*.
* [number] everything that starts with [0-9] followed by XID_Continue.
* [operator,parenthesis] Every unicode code-point that has the derived core property Math but is not
also in XID_Continue. A distinction is made between operators representing opening or closing
parentheses (called parentheses-chars, as per [this
answer](https://stackoverflow.com/a/13535289/1171688)) and all other operator chars. The parentheses
chars are expected to be properly nested already at this stage.
* [indent,dedent,newline] Only space and newline are allowed. Indenting works like Python. Note that
here the equivalent of Python's INDENT and DEDENT are still fragments rather than tokens. Also, at
this stage there is only a single NEWLINE fragment (not NL and NEWLINE like in Python). Consecutive
newlines are fragmented as a single NEWLINE followed by whitespace.
* Any other Unicode code-point not explicitly allowed by any of the semantic fragments or any
sequence that's not in NFC in the semantic part means that the input file is ill-formed.

## Tokenization

Input: Vector of fragments. Output: Vector of tokens.

Tokenization can be configured for each language mainly via the Peat language. Peat has somewhat
similar function to lex/flex/re2c/ragel, but it's not based on regular expressions and is much
simpler. It's basically a configuration language.

A tokenization defines the classes of tokens that are generated.


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
