#pragma once

#include "canopy/expected.hpp"

namespace silva {
  enum class fragment_category_t {
    INVALID = 0,
    WHITESPACE,
    COMMENT,
    STRING,
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    PARENTHESIS,
    INDENT,
    DEDENT,
    NEWLINE,
  };

  struct fragmentization_t : public menhir_t {
    string_view_t source_code;

    // The first offset will always be zero. The last fragment goes from "back()" to the end of the
    // "source_code"
    array_t<index_t> fragment_start_indexes;

    array_t<fragment_category_t> categories;

    friend void pretty_write_impl(const fragmentization_t&, byte_sink_t*);
  };
  using fragmentization_ptr_t = ptr_t<const fragmentization_t>;

  expected_t<fragmentization_ptr_t> fragmentize_load(filesystem_path_t);
  expected_t<fragmentization_ptr_t> fragmentize(string_view_t source_code);
}
