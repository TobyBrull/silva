#pragma once

#include "canopy/tree.hpp"

#include "tokenization.hpp"

namespace silva {
  struct parse_tree_node_t : public tree_node_t {
    name_id_t rule_name = 0;
    index_t token_begin = std::numeric_limits<index_t>::max();
    index_t token_end   = std::numeric_limits<index_t>::min();

    index_t num_tokens() const { return token_end - token_begin; }

    friend auto operator<=>(const parse_tree_node_t&, const parse_tree_node_t&) = default;
    friend void pretty_write_impl(const parse_tree_node_t&, byte_sink_t*);
  };

  struct parse_tree_t : public sprite_t {
    tokenization_ptr_t tp;
    array_t<parse_tree_node_t> nodes;

    auto span(this auto&&);
  };
  using parse_tree_ptr_t = ptr_t<const parse_tree_t>;

  struct parse_tree_span_t : public tree_span_t<const parse_tree_node_t> {
    parse_tree_ptr_t ptp;

    parse_tree_span_t() = default;
    parse_tree_span_t(const parse_tree_node_t* root, index_t stride, parse_tree_ptr_t);
    parse_tree_span_t(const parse_tree_t&);

    parse_tree_t copy() const;

    parse_tree_span_t sub_tree_span_at(index_t) const;

    index_t count_children_with(name_id_t) const;

    index_t token_size() const;

    expected_t<token_id_t> front_token_id() const;
    expected_t<token_id_t> front_token_category() const;
    expected_t<fragment_span_t> front_language() const;
    expected_t<token_id_t> at_token_id(index_t) const;
    expected_t<token_id_t> at_token_category(index_t) const;
    expected_t<fragment_span_t> at_language(index_t) const;
    expected_t<token_id_t> back_token_id() const;
    expected_t<token_id_t> back_token_category() const;
    expected_t<fragment_span_t> back_language() const;

    token_span_t token_span() const;
    token_location_t token_location() const;

    friend void pretty_write_impl(const parse_tree_span_t&, byte_sink_t*);

    expected_t<string_t> to_string(index_t token_offset = 50) const;
    expected_t<string_t> to_graphviz() const;

    friend bool operator==(const parse_tree_span_t&, const parse_tree_span_t&) = default;
  };

  struct name_id_ref_t {
    parse_tree_span_t pts;

    name_id_ref_t() = default;
    name_id_ref_t(parse_tree_span_t);

    void resolve_clear() const;
    template<Namespace Ns>
    expected_t<void> resolve(const name_id_t scope_name, const lexicon_t&, const Ns&) const;

    // Derived data; ignored for equality and hashing.
    mutable name_id_t resolved_name = name_id_none;
    operator name_id_t() const { return resolved_name; }

    friend bool operator==(const name_id_ref_t& lhs, const name_id_ref_t& rhs);
    friend hash_value_t hash_impl(const name_id_ref_t& x);
  };
}

// IMPLEMENTATION

namespace silva {
  inline auto parse_tree_t::span(this auto&& self)
  {
    return parse_tree_span_t{self};
  }

  template<Namespace Ns>
  expected_t<void>
  name_id_ref_t::resolve(const name_id_t scope_name, const lexicon_t& lexicon, const Ns& ns) const
  {
    resolved_name = SILVA_EXPECT_FWD(lexicon.name_id_lookup(scope_name, pts.token_span(), ns));
    return {};
  }
}
