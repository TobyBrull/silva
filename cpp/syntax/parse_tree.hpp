#pragma once

#include "canopy/tree.hpp"

#include "tokenization.hpp"

namespace silva {
  struct seed_engine_t;

  struct parse_tree_node_t : public tree_node_t {
    name_id_t rule_name = 0;
    index_t token_begin = std::numeric_limits<index_t>::max();
    index_t token_end   = std::numeric_limits<index_t>::min();

    index_t num_tokens() const { return token_end - token_begin; }

    friend auto operator<=>(const parse_tree_node_t&, const parse_tree_node_t&) = default;
  };

  struct parse_tree_t : public sprite_t {
    tokenization_ptr_t tp;
    vector_t<parse_tree_node_t> nodes;

    auto span(this auto&&);
  };
  using parse_tree_ptr_t = ptr_t<const parse_tree_t>;

  struct parse_tree_span_t : public tree_span_t<const parse_tree_node_t> {
    tokenization_ptr_t tp;

    parse_tree_span_t() = default;
    parse_tree_span_t(const parse_tree_node_t* root, index_t stride, tokenization_ptr_t);
    parse_tree_span_t(const parse_tree_t&);

    parse_tree_t copy() const;

    parse_tree_span_t sub_tree_span_at(index_t) const;

    token_id_t first_token_id() const;

    token_range_t token_range() const;
    token_position_t token_position() const;

    friend string_or_view_t to_string_impl(const parse_tree_span_t&);

    enum class parse_tree_printing_t {
      ABSOLUTE,
      RELATIVE,
    };
    expected_t<string_t> to_string(index_t token_offset  = 50,
                                   parse_tree_printing_t = parse_tree_printing_t::ABSOLUTE) const;

    expected_t<string_t> to_graphviz() const;
  };
}

// IMPLEMENTATION

namespace silva {
  inline auto parse_tree_t::span(this auto&& self)
  {
    return parse_tree_span_t{self};
  }
}
