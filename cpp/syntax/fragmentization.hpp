#pragma once

#include "canopy/expected.hpp"
#include "canopy/file_location.hpp"

#include "fragmentization_data.hpp"

namespace silva {
  enum class fragment_category_t {
    INVALID = 0,

    WHITESPACE,
    COMMENT,

    NUMBER,
    STRING,

    INDENT,
    DEDENT,
    NEWLINE,

    IDENTIFIER,

    OPERATOR,
    PAREN_LEFT,
    PAREN_RIGHT,

    LANG_BEGIN,
    LANG_END,
  };

  enum class fragment_case_mask_t {
    NONE                   = 0b0000000,
    IDENTIFIER_SILVA_CASE  = 0b0000001, // 'hello-world'
    IDENTIFIER_SNAKE_CASE  = 0b0000010, // 'hello_world'
    IDENTIFIER_CAMEL_CASE  = 0b0000100, // 'helloWorld'
    IDENTIFIER_PASCAL_CASE = 0b0001000, // 'HelloWorld'
    IDENTIFIER_MACRO_CASE  = 0b0010000,
    IDENTIFIER_UPPER_CASE  = 0b0100000, // EVERY character is upper-case, no '_' '-'
    IDENTIFIER_LOWER_CASE  = 0b1000000, // EVERY character is lower-case, no '_' '-'
    ANY                    = ~NONE,
  };

  constexpr bool is_fragment_category_real(fragment_category_t);

  struct fragment_t {
    fragment_category_t category   = fragment_category_t::INVALID;
    fragment_case_mask_t case_make = fragment_case_mask_t::NONE;
    file_location_t location;

    friend auto operator<=>(const fragment_t&, const fragment_t&) = default;

    friend void pretty_write_impl(const fragment_t&, byte_sink_t*);
  };

  struct fragmentization_t : public menhir_t {
    filesystem_path_t filepath;
    string_t source_code;

    array_t<fragment_t> fragments;

    friend void pretty_write_impl(const fragmentization_t&, byte_sink_t*);
  };

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize_load(filesystem_path_t);
  expected_t<unique_ptr_t<fragmentization_t>> fragmentize(filesystem_path_t descriptive_path,
                                                          string_t source_code);
}

// IMPLEMENTATION

namespace silva {
  constexpr bool is_fragment_category_real(const fragment_category_t fc)
  {
    using enum fragment_category_t;
    return (fc != WHITESPACE && fc != COMMENT);
  }
}
