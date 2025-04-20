#pragma once

#include "canopy/string_or_view.hpp"

#include "token_ward.hpp"

namespace silva {
  struct tokenization_t : public menhir_t {
    token_ward_ptr_t context;

    filesystem_path_t filepath;

    struct location_t {
      index_t line_num = 0;
      index_t column   = 0;
    };
    vector_t<location_t> token_locations;
    vector_t<token_id_t> tokens;

    tokenization_t copy() const;

    const token_info_t* token_info_get(index_t token_index) const;

    friend string_or_view_t to_string_impl(const tokenization_t&);
  };
  using tokenization_ptr_t = ptr_t<const tokenization_t>;

  struct token_position_t {
    tokenization_ptr_t tp;
    index_t token_index = 0;

    friend string_or_view_t to_string_impl(const token_position_t&);
  };

  struct token_range_t {
    tokenization_ptr_t tp;
    index_t token_begin = 0;
    index_t token_end   = 0;

    friend string_or_view_t to_string_impl(const token_range_t&);
  };

  expected_t<tokenization_ptr_t> tokenize_load(syntax_ward_t&, filesystem_path_t);
  expected_t<tokenization_ptr_t>
  tokenize(syntax_ward_t&, filesystem_path_t filepath, string_view_t text);
}

// IMPLEMENTATION

namespace silva {
}
