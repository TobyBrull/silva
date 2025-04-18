# TODO

* review all error messages

## Long Term

* Seed
    * add Seed functions/templates?
        * e.g.> - func name_id(root_term, curr_term, parent_term, separator_term, token_nt) = [
                    - x = Base ( separator_terminal Base ) *
                    - Base = root_term | curr_term | parent_term | token_nt
                  ]
        * e.g.> - SeedNonterminal = name_id('_', 'x', 'p', '.')
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
    * make seed-engine-based error look more like the error from the manual Fern parser
        * For Seed expressions of the form ( 'a' | 'b' | 'c' ) make sure that the error
          is just one level ("could not parse ( 'a' | 'b' | 'c' )").

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
