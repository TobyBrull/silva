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
    * Support EOF literal (or token_id_none?)

* ParseAxe
    * Maybe think again about flat levels, but only allow exactly the same
    infix operator multiple times in a row.
```
1 + 2 + 3 - 4 - 5 + 6 + 7

infix "-" infix_flat "+"

[0] + 
  [0] -
    [0] -
      [0] +
        [0] 1
        [1] 2
        [2] 3
      [1] 4
    [1] 5
  [1] 6
  [2] 7
```

* memento:
    * add dumping plain token_ids and full_name_ids, without any positional information
        * make token_id_t and full_name_id_t proper types
    * make memento based on proper move-ctor & dtor semantics, rather than memcpy
        * make token_position_t contain a token_context_ptr_t instead of a raw ptr
        * simplify creation of memoizable types (should just require a move-ctor, dtor, and a
        "materialize" member function).

* Seed
    * check Seed program when constructing parse_root_t?

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

* error-handling
    * make root-based error look more like the error from the manual Fern parser
    * remove "major_error" from Seed

* various:
    * output_buffer_t / string_output_buffer_t
    * allow partial parse (return number of tokens consumed)

* delegate_t
    * replace memento_item_reader_t callback
    * Allow non-compile-time callbacks via additional level of indirection; use in
        parse_root_t::derivation_3 (parse-axe)

* tokenization
    * make "'" the default char to start strings.
    * "string_as_plain_contained" on token_id_t level, in token_context_t

* put error materialisation into token_context_t?

* context:
    * logging
    * testing
    * memory

* code review

* Soil and Toil
    * Extend Seed:
        * maybe add a construct that *commits* (major_error) to one of multiple choices
          based on the next keyword
        * maybe add a construct that implemenets operator precedence parsing for a set
          of operators
        * packrat
            * this might also enable recursion detection (and prevention)
            * recursion prevention could be a functional part of the parsing
              (by ignoring recursive branches certain grammars become viable that
              otherwise wouldn't be viable)
    * error handling should allow to have multiple independent errors at the same time?
