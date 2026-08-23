# TODO

* Fragmentization:
    * NEWLINE fragments should never have empty size

* parse-tree::to_string(): show escaped fragments for branch-rules and pure fragments for twig-rules

* Seed-Axe:
    * seed-axe generated rule-names should not contain operator token
    * support synthesising the "oper" rule somehow?
        * avoid common duplication in oper rule?
        * allow more than just "" and '' in operators?

* Python: add basic parser

* Errors:
    * color furthest fragment in readable color?
    * pass node_and_error_t::last_error through seed-axe
        * error involving Cedar's ExprStmt = Expr ? ';' have no useful info
    * rethink error generation fundamentally
        * In parsing errors, show what has been successfully parsed so far?
    * After errors, parsing should be resume (for error handling in IDEs)

* Lox:
    * Unify: object_pool_t, cactus_t?
        * get rid of object_t::clear_scopes()


## Long Term

* Seed / Fragmentization:
    * function
        * allow uses to write typical parse functions in silva directly
        * add `joined_f(',', Base)`?
    * Axe:
        * add Seed Axe derivation (sub-Axe, super-Axe) mechanism?
    * translate Seed program into IR:
        * check Seed program during translation
        * check that all Nonterminals can be resolved
        * resolve Nonterminal names to their respective name_id_t
        * resolve string Terminals to their corresponding operator
    * packrat?
        * this might also enable recursion detection (and prevention)
        * recursion prevention could be a functional part of the parsing (by ignoring recursive
          branches certain grammars become viable that otherwise wouldn't be viable)
    * allow any type of parentheses to denote sub-language
    * allow the parser to descent into strings?
        * for example for the Seed literal « "not" », the parser could be modified to output a
          parse-tree that already contains the token `not` (i.e., without the double-quotes)
    * Type-checking:
        * branch-rules may not use FRAGMENTS
        * token-rules may only use other token-rules or FRAGMENTS
        * tokens may only have other tokens as nested rules
        * axe.name must not be twig-rule
    * write tests for rules `number` and `date`
    * make seed-engine-based error look more like the error from the manual Fern parser; by creating
      bespoke error messages for certain edge cases.
        * For Seed expressions of the form ( 'a' | 'b' | 'c' ) make sure that the error is just one
          level ("could not parse ( 'a' | 'b' | 'c' )").
        * For Seed expressions of the form ( not keywords_of _.Fern ), give the error "not one of
          the keywords of _.Fern".
    * Resumable parser, i.e., the parser should continue to produced a (broken) parse_tree_t even if
      errors are encountered. For example, take all rules of the form `'<$' ... '$>'`, `'[' ...
      ']'`, `( Expr ';' ) *`, or `( '-' Expr ) *`, determine how they are nested, and infer a
      overall structure from this first. Then parse the rest by filling in the gaps in this overall
      structure where possible, generating errors otherwise. The returned data-structure could be a
      parse_tree_t that contains `_.Error` rules in those gaps where parsing failed.
    * also make '`...`' strings in fragmentization?
    * support explicitly forcing 'node' or 'no_node' on individual called rule
    * Python style string interpolation
    * Mappings:
        * Given a parse-tree and a language, can you validate if the parse-tree conforms to that
          language?
        * Given a parse-tree and a language, reconstruct a normalized version of the input text?

* Write a language server
* Write a REPL

* Library/Canopy:
    * output_buffer_t / string_output_buffer_t
    * context:
        * logging
        * testing
        * memory
    * implement using memory_context
        * vector_t
        * hashmap_t
        * using string_t = vector_t<char>
