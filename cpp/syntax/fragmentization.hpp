#pragma once

#include "canopy/expected.hpp"
#include "canopy/file_location.hpp"

#include "fragmentization_data.hpp"

namespace silva {
  enum class fragment_category_t {
    INVALID = 0,
    WHITESPACE,
    COMMENT,
    STRING,
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    PAREN_LEFT,
    PAREN_RIGHT,
    INDENT,
    DEDENT,
    NEWLINE,
  };

  struct fragmentization_t : public menhir_t {
    filesystem_path_t filepath;
    string_t source_code;

    array_t<fragment_category_t> categories;
    array_t<file_location_t> locations;

    friend void pretty_write_impl(const fragmentization_t&, byte_sink_t*);
  };

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize_load(filesystem_path_t);
  expected_t<unique_ptr_t<fragmentization_t>> fragmentize(filesystem_path_t descriptive_path,
                                                          string_t source_code);
}
