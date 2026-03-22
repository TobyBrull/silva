#pragma once

#include "canopy/file_location.hpp"

#include "fragmentization.hpp"

namespace silva {
  struct tokenization_t : public menhir_t {
    syntax_ward_ptr_t swp;
    filepath_t filepath;

    fragmentization_ptr_t fp;

    array_t<fragment_span_t> languages;

    // For tokens that are a "language", the corresponding entry in "categories" will be "language"
    // and the corresponding entry in "tokens" will be the index in the "languages" array.
    array_t<token_id_t> tokens;
    array_t<token_id_t> categories;
    array_t<file_location_t> locations;

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

  expected_t<tokenization_ptr_t> tokenize_load(syntax_ward_ptr_t, filepath_t);
  expected_t<tokenization_ptr_t> tokenize(syntax_ward_ptr_t, filepath_t, string_view_t source_code);
}

// IMPLEMENTATION

namespace silva {
}
