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
languages that Silva supports. Fragmentation does not impose any nesting requirements on
"parentheses"; only the sub-language delimiters « » are required to be well-formed.

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
* Fragmentation does *not* know about comments or single-line string-literals; those are the
concern of the individual languages and are described in Seed (see "Comments and strings" below).
Only Zig-style multi-line string literals (introduced by '¶') are recognised here, as a single
MULTILINE_STRING fragment, because they span several lines and would otherwise interfere with the
off-side rule.
* A '\\' at the end of a line is a line continuation (LINE_CONTINUATION).
* [identifier] XID_Start XID_Continue*.
* [number] everything that starts with [0-9] followed by XID_Continue.
* [operator,parenthesis] Every unicode code-point that has the derived core property Math but is not
also in XID_Continue. A distinction is made between operators representing opening or closing
parentheses (called parentheses-chars, as per [this
answer](https://stackoverflow.com/a/13535289/1171688)) and all other operator chars. The parentheses
chars are not required to be properly nested at this stage.
* [indent,dedent,newline] Only space and newline are allowed. Indenting works like Python, except
that INDENT and DEDENT are generated regardless of any enclosing parentheses (like in Haskell and
F#); use a trailing '\\' to continue a line. Note that here the equivalent of Python's INDENT and
DEDENT are still fragments rather than tokens. Also, at this stage there is only a single NEWLINE
fragment (not NL and NEWLINE like in Python); every line-end produces one, including the line-ends
of blank lines (fragmentation cannot tell which lines are blank, as it does not know about
comments). A line whose indentation matches none of the enclosing indentation levels produces an
INDENTATION_BROKEN fragment.
* Any other Unicode code-point not explicitly allowed by any of the semantic fragments or any
sequence that's not in NFC in the semantic part means that the input file is ill-formed.

## Comments and strings

Because fragmentation does not recognise comments or single-line strings, their syntax is written
in Seed instead, using two fragment atoms that only twig-rules may use:

* `ANY` matches one *visible* fragment, i.e. anything except INDENT, DEDENT, INDENTATION_BROKEN,
NEWLINE and LINE_CONTINUATION. Rules built out of `ANY` are therefore automatically confined to a
single line.
* `LANGUAGE` matches a whole balanced LANG_BEGIN/LANG_END region; putting it before `ANY` in an
alternation is what allows a comment to contain « … ».

The standard definitions live in `seed::globals_str`: `string`, `indent`, `dedent`, `newline` and
the two skip-rules `offSide` and `freeForm`. A language selects one of the latter via its `skip`
rule.

Two consequences follow from doing it this way:

* The code-points that fragmentation still treats specially -- '⎢', '«', '»', '¶' and a '\\' at the
end of a line -- keep their meaning inside comments and strings. In particular a '«' inside a
comment must still be matched by a '»'.
* Comments are code as far as the off-side rule is concerned: they take part in indentation and a
mis-indented comment is a parse error. Blank and comment-only lines produce no tokens, though;
their NEWLINE is absorbed by the `offSide.blankLines` rule, which the `indent`, `dedent` and
`newline` rules of `seed::globals_str` apply after themselves.

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
