# TODO

* error-handling
    * rethink approach:
        * maybe make memento simpler and express format strings via parse_error_tree_t?
    * never use vector_t when constructing multi-error (parse_root.cpp)

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
