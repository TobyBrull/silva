# TODO

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

* code review

* Soil and Toil
    * Extend Seed:
        * support for parse-root to commit to certain rules
        * packrat
            * this might also enable recursion detection
            * recursion detection could be part of the parsing (ignore recursive branches)
    * error handling should allow to have multiple independent errors at the same time?
