#pragma once

#include "canopy/tree.hpp"

#include "tokenization.hpp"

namespace silva {
  struct parse_tree_node_t : public tree_node_t {
    name_id_t rule_name;
    index_t token_begin = std::numeric_limits<index_t>::max();
    index_t token_end   = std::numeric_limits<index_t>::min();

    index_t fragment_begin = std::numeric_limits<index_t>::max();
    index_t fragment_end   = std::numeric_limits<index_t>::min();

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

    expected_t<token_id_t> token() const;
    expected_t<fragment_span_t> language() const;

    index_t token_size() const;
    token_span_t token_span() const;

    fragment_span_t fragment_span() const;
    fragment_location_t location() const;

    friend void pretty_write_impl(const parse_tree_span_t&, byte_sink_t*);

    enum class to_string_mode_t {
      NONE         = 0b000,
      TOKENIZATION = 0b001,
      PARSE_TREE   = 0b010,
      ALL          = 0b011,
    };
    expected_t<string_t> to_string(index_t token_indent = 50,
                                   to_string_mode_t     = to_string_mode_t::ALL) const;
    expected_t<string_t> to_graphviz() const;

    friend bool operator==(const parse_tree_span_t&, const parse_tree_span_t&) = default;
  };

  expected_t<name_id_t>
  name_id_definition(const lexicon_t&, name_id_t scope_name, const parse_tree_span_t&);

  template<Namespace Ns>
  expected_t<name_id_t>
  name_id_lookup(const lexicon_t&, name_id_t scope_name, const parse_tree_span_t&, const Ns&);

  struct name_id_ref_t {
    parse_tree_span_t pts;

    name_id_ref_t() = default;
    name_id_ref_t(parse_tree_span_t);

    void resolve_clear() const;

    template<Namespace Ns>
    expected_t<void> resolve(const name_id_t scope_name, const lexicon_t&, const Ns&) const;

    // Derived data; ignored for equality and hashing.
    mutable name_id_t resolved_name;
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
  expected_t<name_id_t> name_id_lookup(const lexicon_t& lexicon,
                                       const name_id_t scope_name,
                                       const parse_tree_span_t& pts,
                                       const Ns& ns)
  {
    SILVA_EXPECT(pts[0].num_children > 0, MINOR);
    const token_id_t front_token = SILVA_EXPECT_FWD(pts.sub_tree_span_at(1).token());
    if (front_token == lexicon.name_sep) {
      const name_id_t abs_name = SILVA_EXPECT_FWD(name_id_definition(lexicon, scope_name, pts));
      SILVA_EXPECT(ns.contains(abs_name),
                   MINOR,
                   "absolute name {} does not exist",
                   lexicon.name_id_wrap(abs_name));
      return abs_name;
    }
    else {
      name_id_t curr_scope = scope_name;
      error_nursery_t error_nursery;
      while (true) {
        const name_id_t curr_name = SILVA_EXPECT_FWD(name_id_definition(lexicon, curr_scope, pts));
        if (ns.contains(curr_name)) {
          return curr_name;
        }
        error_nursery.add_child_error(make_error(error_level_t::MINOR,
                                                 {},
                                                 "could not find {}",
                                                 lexicon.name_id_wrap(curr_name)));
        if (!curr_scope.is_valid()) {
          break;
        }
        curr_scope = lexicon.sfp->get(curr_scope).parent_name;
      }
      return std::unexpected(std::move(error_nursery)
                                 .finish(error_level_t::MINOR,
                                         "unable to lookup name in scope {}: {}",
                                         lexicon.name_id_wrap(scope_name),
                                         pts.fragment_span()));
    }
  }

  template<Namespace Ns>
  expected_t<void>
  name_id_ref_t::resolve(const name_id_t scope_name, const lexicon_t& lexicon, const Ns& ns) const
  {
    resolved_name = SILVA_EXPECT_FWD(name_id_lookup(lexicon, scope_name, pts, ns));
    return {};
  }
}
