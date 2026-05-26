#pragma once

#include "fragmentization.hpp"

namespace silva {
  struct tokenization_t : public menhir_t {
    syntax_farm_ptr_t sfp;
    filepath_t filepath;

    fragment_span_t fs;
    array_t<token_t> tokens;

    index_t size() const;

    file_location_t location_at(index_t idx) const;

    const token_info_t* token_info_get(index_t token_index) const;

    friend void pretty_write_impl(const tokenization_t&, byte_sink_t*);
  };
  using tokenization_ptr_t = ptr_t<const tokenization_t>;

  struct token_location_t {
    tokenization_ptr_t tp;
    index_t token_index = 0;

    friend void pretty_write_impl(const token_location_t&, byte_sink_t*);
  };

  struct token_span_t {
    tokenization_ptr_t tp;
    index_t begin = 0;
    index_t end   = 0;

    index_t size() const;

    operator span_t<const token_t>() const;

    friend void pretty_write_impl(const token_span_t&, byte_sink_t*);
  };
}

// IMPLEMENTATION

namespace silva {
}
