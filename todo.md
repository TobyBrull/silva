# TODO

* Seed:
    * Also support rules of the form `PrimaryExpr = "(" Expr ")"` for alias-rules.

* error-handling
    * make root-based error look more like the error from the manual Fern parser

* various:
    * output_buffer_t / string_output_buffer_t
    * allow partial parse (return number of tokens consumed)

* delegate_t
    * replace memento_item_reader_t callback

* context:
    * logging
    * testing
    * memory

* Seed
    * seed.mpp
    * check Seed program when constructing parse_root_t?

* tokenization
    * make "'" the default char to start strings.

* Introduce namespaces in Seed
    * Seed
    * Seed.Rule
    * Silva.Expr.FuncCall

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
