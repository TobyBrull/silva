# TODO

* Document variant.hpp

* Trees:
    * Make tree.hpp not depend on expected_t (or rather: not depend on error_tree_t)?
    * Use tree_inv_t in error_tree_t?
    * Add tree_nursery_t and tree_inv_nursery_t
    * Add tree_view_t and tree_inv_view_t

    * Use num_children_total (and num_children_direct?) instead of children_begin/end
      For easier grafting.

* memento:
    * all dumping plain token_ids and full_name_ids, without any positional information

* Seed:
    * Also support rules of the form `PrimaryExpr = "(" Expr ")"` for alias-rules.

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

* tokenization
    * make "'" the default char to start strings.

* put error materialisation into token_context_t?

* context:
    * logging
    * testing
    * memory

* Seed
    * seed.mpp
    * check Seed program when constructing parse_root_t?
    * Introduce namespaces in Seed
        * Seed
        * Seed.Rule
        * Silva.Expr.FuncCall

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
