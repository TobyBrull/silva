# TODO

* Turn all tokenization rules into normal rules
    * use camelCase for rules that do NOT create nodes by default
    * use PascalCase for rule that DO create notes by default
    * remove 'no_node' qualifier

* tokenization-less parse-tree

* `number` tokenization: write tests
* better tokenization
    * token rules startswith(...) endswith(...) (and use "_t" and "_f" in soil again)
    * also make '`...`' strings in fragmentization?
    * Identifier tokens should always parse the chars greedily; then apply some predicate afterwards
* enforce:
    * node-rules may only use other node-rules or token-rules
    * token-rules may only use other token-rules or FRAGMENTS
    * tokens may only have other tokens as sub-rules
* parse skip-rule AFTER every token
    * to support "end_of_language"
    * don't skip in seed.cpp in recursive token calls
* seed-axe:
    * avoid common duplication in oper rule?
    * allow more than just "" and '' in operators
* In parsing errors, show what has been successfully parsed so far

* Parsing:
    * Parse Python
    * Parse C
    * Parse Toml
    * Python style string interpolation
    * After errors, parsing should be resume (for error handling in IDEs)

* Lox:
    * Unify: object_pool_t, cactus_t?
        * get rid of object_t::clear_scopes()

* ParseTrees
    * Support conversion of parse_tree_t (with Seed) back to text/tokenization_t.
        * I think the parse_tree_t should be detached from the tokenization_t
    * Support construction of parse_tree_t's
        * Allow a parse_tree_t to be spliced into another parse_tree_t.
    * Support checking if a parse_tree_t is valid according to a given Seed.


## Long Term

* Seed
    * function
        * allow uses to write typical parse functions in silva directly
        * add `joined_f(',', Base)`?
    * add Seed Axe derivation (sub-Axe, super-Axe) mechanism?
    * translate Seed program into IR:
        * check Seed program during translation
        * check that all Nonterminals can be resolved
        * resolve Nonterminal names to their respective name_id_t
        * resolve string Terminals to their corresponding operator
    * packrat?
        * this might also enable recursion detection (and prevention)
        * recursion prevention could be a functional part of the parsing
          (by ignoring recursive branches certain grammars become viable that
          otherwise wouldn't be viable)
    * make seed-engine-based error look more like the error from the manual Fern parser; by creating
      bespoke error messages for certain edge cases.
        * For Seed expressions of the form ( 'a' | 'b' | 'c' ) make sure that the error
          is just one level ("could not parse ( 'a' | 'b' | 'c' )").
        * For Seed expressions of the form ( not keywords_of _.Fern ), give the error
          "not one of the keywords of _.Fern".
    * Resumable parser, i.e., the parser should continue to produced a (broken) parse_tree_t even if
      errors are encountered. For example, take all rules of the form `'<$' ... '$>'`, `'[' ... ']'`,
      `( Expr ';' ) *`, or `( '-' Expr ) *`, determine how they are nested, and infer a overall
      structure from this first. Then parse the rest by filling in the gaps in this overall
      structure where possible, generating errors otherwise. The returned data-structure could be a
      parse_tree_t that contains `_.Error` rules in those gaps where parsing failed.

* Write a language server
* Write a REPL

* Library
    * output_buffer_t / string_output_buffer_t
    * context:
        * logging
        * testing
        * memory
    * implement using memory_context
        * vector_t
        * hashmap_t
        * using string_t = vector_t<char>
