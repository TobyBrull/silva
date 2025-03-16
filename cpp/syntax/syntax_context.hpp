#pragma once

#include "canopy/expected.hpp"

namespace silva {
  enum class token_category_t {
    NONE = 0,
    IDENTIFIER,
    OPERATOR,
    STRING,
    NUMBER,
  };
  string_view_t to_string(token_category_t);

  // An index in the "token_infos" vector of "token_context_t". Equality of two tokens is then
  // equivalent to the equality of their token_info_index_t.
  using token_id_t = index_t;

  // Index in "full_name_infos".
  using full_name_id_t = index_t;

  constexpr inline token_id_t token_id_none         = 0;
  constexpr inline full_name_id_t full_name_id_none = 0;

  struct token_info_t {
    string_t str;
    token_category_t category = token_category_t::NONE;

    expected_t<string_view_t> string_as_plain_contained() const;
    expected_t<double> number_as_double() const;

    friend auto operator<=>(const token_info_t&, const token_info_t&) = default;
  };

  struct full_name_info_t {
    full_name_id_t parent_name = full_name_id_none;
    token_id_t base_name       = token_id_none;

    friend auto operator<=>(const full_name_info_t&, const full_name_info_t&) = default;
    friend hash_value_t hash_impl(const full_name_info_t& x);
  };

  struct syntax_context_t : public menhir_t {
    vector_t<token_info_t> token_infos;
    hashmap_t<string_t, token_id_t> token_lookup;

    vector_t<full_name_info_t> full_name_infos;
    hashmap_t<full_name_info_t, full_name_id_t> full_name_lookup;
  };
}
