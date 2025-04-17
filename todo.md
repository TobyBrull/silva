# TODO

* memento/error-handling:
    * Seed:
        * make seed-engine-based error look more like the error from the manual Fern parser
    * put error materialisation into token_context_t?

* various:
    * output_buffer_t / string_output_buffer_t
    * allow partial parse (return number of tokens consumed)
    * Can't use token_id_none for concat exprs anymore

* context:
    * logging
    * testing
    * memory

## Long Term

* Seed
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
