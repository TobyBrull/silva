#pragma once

#include "canopy/file_location.hpp"

#include "fragmentization.hpp"

namespace silva {
  struct tokenization_t : public menhir_t {
    syntax_farm_ptr_t sfp;
    filepath_t filepath;

    fragment_span_t fs;

    // For tokens that are a "language", the corresponding entry in "tokens" and in "categories"
    // will both be "token_id_language".
    array_t<token_id_t> tokens;
    array_t<token_id_t> categories;
    array_t<file_location_t> locations;

    // Maps the *index* of an entry in "tokens" and "categories" that contains "token_id_language"
    // to the corresponding range of fragments.
    hash_map_t<index_t, fragment_span_t> languages;

    index_t size() const;

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
}

// IMPLEMENTATION

namespace silva {
}
