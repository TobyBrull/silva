#pragma once

#include "canopy/expected.hpp"

namespace silva {
  enum class token_category_t {
    INVALID = 0,
    IDENTIFIER,
    OPERATOR,
    STRING,
    NUMBER,
    WHITESPACE,
    COMMENT,
  };

  tuple_t<string_view_t, token_category_t> tokenize_one(const string_view_t text);

  // An index in the "token_infos" vector of "syntax_ward_t". Equality of two tokens is then
  // equivalent to the equality of their token_info_index_t.
  using token_id_t = index_t;

  // Index in "name_infos".
  using name_id_t = index_t;

  constexpr inline token_id_t token_id_none = 0;
  constexpr inline name_id_t name_id_root   = 0;

  struct token_info_t {
    token_category_t category = token_category_t::INVALID;
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

  struct name_id_style_t;
  struct token_id_wrap_t;
  struct name_id_wrap_t;

  struct tokenization_t;
  struct parse_tree_t;
  using tokenization_ptr_t = ptr_t<const tokenization_t>;
  using parse_tree_ptr_t   = ptr_t<const parse_tree_t>;

  struct syntax_ward_t : public menhir_t {
    array_t<token_info_t> token_infos;
    hash_map_t<string_t, token_id_t> token_lookup;

    array_t<name_info_t> name_infos;
    hash_map_t<name_info_t, name_id_t> name_lookup;

    array_t<unique_ptr_t<const tokenization_t>> tokenizations;
    array_t<unique_ptr_t<const parse_tree_t>> parse_trees;

    struct impl_t;
    unique_ptr_t<impl_t> impl;

    syntax_ward_t();
    ~syntax_ward_t();

    expected_t<token_id_t> token_id(string_view_t);
    expected_t<token_id_t> token_id_in_string(token_id_t);

    name_id_t name_id(name_id_t parent_name, token_id_t base_name);
    name_id_t name_id_span(name_id_t parent_name, span_t<const token_id_t>);
    bool name_id_is_parent(name_id_t parent_name, name_id_t child_name) const;

    name_id_t name_id_lca(name_id_t, name_id_t) const;

    template<typename... Ts>
    name_id_t name_id_of(Ts&&... xs);
    template<typename... Ts>
    name_id_t name_id_of(name_id_t parent_name, Ts&&... xs);

    const name_id_style_t& default_name_id_style() const;

    token_id_wrap_t token_id_wrap(token_id_t);
    name_id_wrap_t name_id_wrap(name_id_t);

    tokenization_ptr_t add(unique_ptr_t<const tokenization_t>);
    parse_tree_ptr_t add(unique_ptr_t<const parse_tree_t>);
  };
  using syntax_ward_ptr_t = ptr_t<syntax_ward_t>;

  struct token_id_wrap_t {
    syntax_ward_ptr_t swp;
    token_id_t token_id = token_id_none;

    friend void pretty_write_impl(const token_id_wrap_t&, byte_sink_t*);
  };

  struct name_id_wrap_t {
    syntax_ward_ptr_t swp;
    name_id_t name_id = name_id_root;

    friend void pretty_write_impl(const name_id_wrap_t&, byte_sink_t*);
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename... Ts>
  name_id_t syntax_ward_t::name_id_of(Ts&&... xs)
  {
    array_t<token_id_t> vec;
    ((vec.push_back(token_id(std::forward<Ts>(xs)).value())), ...);
    return name_id_span(name_id_root, vec);
  }

  template<typename... Ts>
  name_id_t syntax_ward_t::name_id_of(name_id_t parent_name, Ts&&... xs)
  {
    array_t<token_id_t> vec;
    ((vec.push_back(token_id(std::forward<Ts>(xs)).value())), ...);
    return name_id_span(parent_name, vec);
  }
}
