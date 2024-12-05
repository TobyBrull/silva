#pragma once

#include "misc.hpp"

namespace silva {
  struct source_code_t {
    string_t filename;
    string_view_t text;
  };

  struct source_location_t {
    const source_code_t* source_code = nullptr;

    index_t line   = 0;
    index_t column = 0;
  };

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
    const source_code_t* source_code = nullptr;

    struct token_data_t {
      string_view_t str;
      token_category_t category = token_category_t::INVALID;

      string_t as_string() const;
      double as_double() const;

      friend auto operator<=>(const token_data_t&, const token_data_t&) = default;
    };
    vector_t<token_data_t> token_datas;

    // Maps a token-string to the index of the element in "token_datas" holding that token's data.
    hashmap_t<string_view_t, token_id_t> token_lookup;

    // List of tokens that make up this tokenization.
    vector_t<token_id_t> tokens;

    struct line_data_t {
      // The index of the first token in that line. If a line does not contain any tokens, this is
      // the index of the next token. If a token (can only be a string) spans multiple lines, this
      // is the index of just that token.
      token_index_t token_index = 0;
      // Offset of the start of this line within "text" of "source_code".
      index_t source_code_offset = 0;

      friend auto operator<=>(const line_data_t&, const line_data_t&) = default;
    };
    // If "source_code" contains (n) lines, then this vector contains (n + 1) entries. The last
    // entry in this vector is the size/length of "tokens".
    vector_t<line_data_t> lines;

    const token_data_t* token_data(token_index_t) const;

    optional_t<token_id_t> lookup_token(string_view_t) const;

    source_location_t compute_source_location(token_index_t) const;

    string_t to_string() const;

    void append_token(const tokenization_t::token_data_t*);
    void append_new_line(index_t source_code_offset);
  };

  tokenization_t tokenize(const source_code_t*);
}

// IMPLEMENTATION

namespace silva {
  inline const tokenization_t::token_data_t*
  tokenization_t::token_data(const token_index_t token_index) const
  {
    return &token_datas[tokens[token_index]];
  }
}
