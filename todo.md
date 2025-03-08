# TODO

```silva.seed
- ParseAxe = [
  - Goal = "parse_axe" Nonterminal "[" LevelNest* Level* "]"
  - LevelNest = [
    - Goal  = "-" Nonterminal "=" "nest" List*
    - List  = "atom_nest" operator*
  ]
  - Level = [
    - Goal = "-" Nonterminal "=" Assoc List*
    - Assoc = "nest" | "ltr" | "rtl" | "flat
    - List = ( "prefix" | ... | "postfix_nest" ) operator*
  ]
]
```
* Seed
    * Introduce namespaces in Seed
        * Seed
        * Seed.Rule
        * Silva.Expr.FuncCall
    * translate Seed program into IR:
        * check Seed program during translation
        * resolve Nonterminal names to their respective full_name_id_t
    * packrat?
        * this might also enable recursion detection (and prevention)
        * recursion prevention could be a functional part of the parsing
          (by ignoring recursive branches certain grammars become viable that
          otherwise wouldn't be viable)

* memento/error-handling:
    * Seed:
        * make root-based error look more like the error from the manual Fern parser
        * error handling should allow to have multiple independent errors at the same time?
    * add dumping plain token_ids and full_name_ids, without any positional information
        * make token_id_t and full_name_id_t proper types
    * make memento based on proper move-ctor & dtor semantics, rather than memcpy
        * make token_position_t contain a token_context_ptr_t instead of a raw ptr
        * simplify creation of memoizable types (should just require a move-ctor, dtor, and a
        "materialize" member function).
    * replace memento_item_reader_t callback with delegate_?
    * put error materialisation into token_context_t?

* Document variant.hpp

* Trees:
    * Make tree.hpp not depend on expected_t (or rather: not depend on error_tree_t)?
    * Use tree_inv_t in error_tree_t?
    * Add tree_nursery_t and tree_inv_nursery_t
    * Add tree_view_t and tree_inv_view_t

    * Use num_children_total (and num_children_direct?) instead of children_begin/end
      For easier grafting.

* Fern:
    * Have [] for arrays and {} for dicts (basically make it JSON without commas)?

* various:
    * output_buffer_t / string_output_buffer_t
    * allow partial parse (return number of tokens consumed)

* tokenization
    * make "'" the default char to start strings.
    * "string_as_plain_contained" on token_id_t level, in token_context_t

* context:
    * logging
    * testing
    * memory
