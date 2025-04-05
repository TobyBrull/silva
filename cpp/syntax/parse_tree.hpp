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

  struct parse_tree_t {
    shared_ptr_t<const tokenization_t> tokenization;
    vector_t<parse_tree_node_t> nodes;

    auto span(this auto&&);
  };

  struct parse_tree_span_t : public tree_span_t<const parse_tree_node_t> {
    shared_ptr_t<const tokenization_t> tokenization;

    parse_tree_span_t() = default;
    parse_tree_span_t(const parse_tree_node_t* root,
                      index_t stride,
                      shared_ptr_t<const tokenization_t>);
    parse_tree_span_t(const parse_tree_t&);

    parse_tree_t copy() const;

    parse_tree_span_t sub_tree_span_at(index_t) const;

    enum class parse_tree_printing_t {
      ABSOLUTE,
      RELATIVE,
    };
    expected_t<string_t> to_string(index_t token_offset  = 50,
                                   parse_tree_printing_t = parse_tree_printing_t::ABSOLUTE);

    expected_t<string_t> to_graphviz();
  };
}

// IMPLEMENTATION

namespace silva {
  inline auto parse_tree_t::span(this auto&& self)
  {
    return parse_tree_span_t{self};
  }
}
