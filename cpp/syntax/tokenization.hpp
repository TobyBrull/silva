#pragma once

#include "syntax_ward.hpp"

namespace silva {
  struct tokenization_t : public menhir_t {
    syntax_ward_ptr_t swp;

    filesystem_path_t filepath;

    struct location_t {
      index_t line_num = 0;
      index_t column   = 0;

      friend auto operator<=>(const location_t&, const location_t&) = default;

      friend void pretty_write_impl(const location_t&, byte_sink_t*);
    };
    static constexpr location_t location_eof{.line_num = -1, .column = -1};

    array_t<location_t> token_locations;
    array_t<token_id_t> tokens;

    tokenization_t copy() const;

    const token_info_t* token_info_get(index_t token_index) const;

    friend void pretty_write_impl(const tokenization_t&, byte_sink_t*);
  };
  using tokenization_ptr_t = ptr_t<const tokenization_t>;

  struct token_location_t {
    tokenization_ptr_t tp;
    index_t token_index = 0;

    friend void pretty_write_impl(const token_location_t&, byte_sink_t*);
  };

  struct token_range_t {
    tokenization_ptr_t tp;
    index_t token_begin = 0;
    index_t token_end   = 0;

    friend void pretty_write_impl(const token_range_t&, byte_sink_t*);
  };

  expected_t<tokenization_ptr_t> tokenize_load(syntax_ward_ptr_t, filesystem_path_t);
  expected_t<tokenization_ptr_t> tokenize(syntax_ward_ptr_t syntax_ward_ptr,
                                          filesystem_path_t descriptive_path,
                                          string_view_t source_code);
}

// IMPLEMENTATION

namespace silva {
}
