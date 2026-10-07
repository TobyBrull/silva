# Design

only the sub-language delimiters « » are required to be well-formed.

The nature of fragmentation, as described below, means that Silva can't parse Rust, for example,
because Rust uses `'a` as lifetime annotation, but this would always be fragmented as the beginning
of a string in Silva.

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
