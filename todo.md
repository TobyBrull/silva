# TODO

* Allow multiple code snippets
    * token_context_t -> naming_context_t
        * could include the set of tokenizations

* Add "keywords": if a token is used as 'then' in Seed, make sure it's not parsed as an identifier
    anymore

* memento/error-handling:
    * Seed:
        * make root-based error look more like the error from the manual Fern parser
        * error handling should allow to have multiple independent errors at the same time?
    * add dumping plain token_ids and name_ids, without any positional information
        * make token_id_t and name_id_t proper types
    * make memento based on proper move-ctor & dtor semantics, rather than memcpy
        * make token_position_t contain a token_context_ptr_t instead of a raw ptr
        * simplify creation of memoizable types (should just require a move-ctor, dtor, and a
        "materialize" member function).
    * replace memento_item_reader_t callback with delegate_?
    * put error materialisation into token_context_t?

* various:
    * output_buffer_t / string_output_buffer_t
    * allow partial parse (return number of tokens consumed)
    * Can't use token_id_none for concat exprs anymore

* tokenization
    * "string_as_plain_contained" on token_id_t level, in token_context_t

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
