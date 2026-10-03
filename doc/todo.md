# TODO

* seed-axe: is concat branch in main shunting_yard() loop missing precedence check?
    * should the precedence check be hoisted out?
    * factor out prefix_oper_in_atom_mode() and regular_oper_in_infix_mode()
* for example for the Seed literal « "not" », the parser could be modified to output a
  parse-tree that already contains the token `not` (i.e., without the double-quotes)

* Rewrite design.md

* Runtime:
    * ??
    * Lox:
        * Unify: object_pool_t, cactus_t?
            * get rid of object_t::clear_scopes()


## Long Term

* Seed / Fragmentization:
    * Support positive lookahead in Seed (similar to "&" in the python grammar; "!" is already
      equivalent to "not")
        * add force-parse ("&&") operators from Python's grammar?
        * replace prefix rule with the cut ("~") operator?
    * function
        * allow uses to write typical parse functions in silva directly
        * add `joined_f(',', Base)`?
            * rules should be able to take rules as parameters
            * scopes should be able to take rules as parameters, which results in multiple rules:
                * e.g., « skip, skip.initial, newline, indent, dedent = offSide('//') »
    * expression parentheses:
        * support Rust's raw strings that use an arbitrary number of '####...' and end when finding
          the same number of '#'.
        * support rules that use one of multiple matching parentheses. for example, something like
            « SubExpr = [ '(' '[' ] as OPEN Expr ClosingParenthesis(OPEN) »
    * Axe:
        * add Seed Axe derivation (sub-Axe, super-Axe) mechanism?
        * support "literal_nodes" attribute
    * translate Seed program into IR:
        * check Seed program during translation
        * check that all Nonterminals can be resolved
        * resolve Nonterminal names to their respective name_id_t
        * resolve string Terminals to their corresponding operator
    * packrat?
        * this might also enable recursion detection (and prevention)
        * recursion prevention could be a functional part of the parsing (by ignoring recursive
          branches certain grammars become viable that otherwise wouldn't be viable)
    * Type-checking:
        * branch-rules may not use FRAGMENTS
        * token-rules may only use other token-rules or FRAGMENTS
        * tokens may only have other tokens as nested rules
        * seed-axe (and its sub-rules) must not be defined as twig-rules
    * write tests for rules `number` and `date`
    * Errors:
        * rethink error generation fundamentally
            * In parsing errors, show what has been successfully parsed so far?
            * pass node_and_error_t::last_error through seed-axe
            * error involving Cedar's ExprStmt = Expr ? ';' have no useful info if the parse error is in
              the Expr
        * After errors, parsing should be resume (for error handling in IDEs)
        * Maybe use Python's "invalid_*" rules?
        * make seed-engine-based error look more like the error from the manual Fern parser; by creating
          bespoke error messages for certain edge cases.
            * For Seed expressions of the form ( 'a' | 'b' | 'c' ) make sure that the error is just one
              level ("could not parse ( 'a' | 'b' | 'c' )").
            * For Seed expressions of the form ( not keywords_of _.Fern ), give the error "not one of
              the keywords of _.Fern".
    * support explicitly forcing 'node' or 'no_node' on individual called rule
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
