# Design Ideas

1. All input files must be UTF-8 in NFC.
2. The "parse-tree" is the core data-structure of Silva. More specifically, by "parse-tree" a
   data-structure equivalent to the C++ data-structure `silva::parse_tree` is meant.
3. All inputs files are first translated to a parse-tree. The Seed language play a critical role in
   this. The Seed language allows to quickly define recursive descent parsers (plus shunting-yard)
with regex-based lexers (plus some other features, like Python-sytle indenting). Where this not
enough, the lexer and parser can be amended with aribtrary user code.
4. A parse-tree can always be converted back to a source file that, if parsed, would result in the
   same parse-tree.
5. Silva allows transformation of parse-trees. For example, a parse-tree representing a C program
   can be transformed into a parse-tree representing an assmbly program with the same semantics.
6. Silva programs can define parse-tree literals. Silva variables can holds parse-trees. Silva
   functions can take and return parse-trees.

## Miscellaneous Desirable Features

* Python f-strings.
* Python indent-based scopes.
* Zig string literals.
* Deduction of language to be parsed.
