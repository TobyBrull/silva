# TODO

* Allow error forwarding completely without adding error node. (fern.mpp:26)

* Rename seed_engine_t to seed_interpreter_t

* tokenization:
    * Allow for python-style indenting (i.e., such that indentation generates it generated tokens in the tokenization)?
    * Support user-defined tokenization rules.

## Long Term

* Seed
    * add `joined_f(',', Base)`
    * add `- Nonterminal = name_f('.', '_' | 'x' | 'p' | identifier / '^[A-Z]')`
    * add `- Arg = p.Expr | Variable | '<$' p.Nonterminal -> nt_v nonterminal_f(_, nt_v) '$>'`
    * add Seed functions/templates/macros?
        * e.g.> - func name_id(root_term, curr_term, parent_term, separator_term, token_nt) = [
                    - x = Base ( separator_terminal Base ) *
                    - Base = root_term | curr_term | parent_term | token_nt
                  ]
        * e.g.> - SeedNonterminal = name_id('_', 'x', 'p', '.')
        * e.g.> - Expr = axe_f(Atom, [
                    - Prefix = rtl prefix 'not'
                    - Postfix = ltr postfix '?' '*'
                  ])
        * e.g.> - Impl = 'impl' ':' _.Seed.Nonterminal -> nt_v print_f(nt_v) resolve_f(_, nt_v)
        * e.g.> - Args = joined_f(',', p.Expr)
        * e.g.> - Expr = parse_axe_f(Atom, <$ Axe [
                  - Parens    = nest  atom_nest '(' ')'
                  - Prefix    = rtl   prefix 'not'
                  - Postfix   = ltr   postfix '?' '*' '+'
                  - Concat    = ltr   infix_flat concat
                  - And       = ltr   infix_flat 'but_then'
                  - Or        = ltr   infix_flat '|'
                  ] $> )
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
