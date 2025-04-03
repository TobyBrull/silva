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

  // Index in "name_infos".
  using name_id_t = index_t;

  constexpr inline token_id_t token_id_none = 0;
  constexpr inline name_id_t name_id_root   = 0;

  struct token_info_t {
    token_category_t category = token_category_t::NONE;
    string_t str;

    expected_t<string_view_t> string_as_plain_contained() const;
    expected_t<double> number_as_double() const;

    friend auto operator<=>(const token_info_t&, const token_info_t&) = default;
  };

  struct name_info_t {
    name_id_t parent_name = name_id_root;
    token_id_t base_name  = token_id_none;

    friend auto operator<=>(const name_info_t&, const name_info_t&) = default;
    friend hash_value_t hash_impl(const name_info_t& x);
  };

  struct tokenization_t;
  struct parse_tree_t;

  using tokenization_id_t = index_t;
  using tree_id_t         = index_t;
}
