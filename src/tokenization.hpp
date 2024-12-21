#pragma once

#include "canopy/const_ptr.hpp"
#include "canopy/source_code.hpp"
#include "canopy/string_or_view.hpp"
#include "canopy/types.hpp"

namespace silva {
  enum class token_category_t {
    INVALID = 0,
    IDENTIFIER,
    OPERATOR,
    STRING,
    NUMBER,
  };
  string_view_t to_string(token_category_t);

  // A reference to a position in the "token_datas" vector of the "tokenization_t" struct.
  // Equality of two tokens is equivalent to equality of their token_id_t.
  using token_id_t = index_t;

  // A reference to a position in the "tokens" vector of the "tokenization_t" struct.
  using token_index_t = index_t;

  struct tokenization_t {
    const_ptr_t<source_code_t> source_code;

    struct token_data_t {
      string_view_t str;
      token_category_t category = token_category_t::INVALID;

      string_t as_string() const;
      string_or_view_t as_string_or_view() const;
      expected_t<double> as_double() const;

      friend auto operator<=>(const token_data_t&, const token_data_t&) = default;
    };
    vector_t<token_data_t> token_datas;

    // Maps a token-string to the index of the element in "token_datas" holding that token's data.
    hashmap_t<string_view_t, token_id_t> token_data_lookup;

    // List of tokens that make up this tokenization.
    vector_t<token_id_t> tokens;

    struct line_data_t {
      // The index of the first token in that line. If a line does not contain any tokens, this is
      // the index of any token in a following line. What if a token spans multiple lines (could
      // only happend for strings)?
      token_index_t token_index = 0;
      // Offset of the start of this line within "text" of "source_code".
      index_t source_code_offset = 0;

      friend auto operator<=>(const line_data_t&, const line_data_t&) = default;
    };
    // Contains one entry for each line in "source_code".
    vector_t<line_data_t> lines;

    const token_data_t* token_data(token_index_t) const;

    optional_t<token_id_t> lookup_token(string_view_t) const;

    const line_data_t* binary_search_line(token_index_t) const;

    source_location_t retokenize_source_location(token_index_t) const;

    string_t to_string() const;
  };

  expected_t<tokenization_t> tokenize(const_ptr_t<source_code_t>);
}

// IMPLEMENTATION

namespace silva {
  inline const tokenization_t::token_data_t*
  tokenization_t::token_data(const token_index_t token_index) const
  {
    return &token_datas[tokens[token_index]];
  }
}
