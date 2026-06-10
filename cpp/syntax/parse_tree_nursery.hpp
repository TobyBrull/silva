#pragma once

#include "parse_tree.hpp"

#include "canopy/tree_nursery.hpp"

namespace silva {

#define SILVA_EXPECT_PARSE(name, cond, fmt_str, ...) \
  SILVA_EXPECT(cond,                                 \
               MINOR,                                \
               "[{}] {}: " fmt_str,                  \
               fragment_location_by(),               \
               lexicon.name_id_wrap(name) __VA_OPT__(, ) __VA_ARGS__);

#define SILVA_EXPECT_PARSE_FWD(name, expr) \
  SILVA_EXPECT_FWD(expr, "[{}] {}", fragment_location_by(), lexicon.name_id_wrap(name))

#define SILVA_EXPECT_PARSE_FRAGMENT_CODEPOINT(name, codepoint)                              \
  {                                                                                         \
    const auto cp = SILVA_EXPECT_PARSE_FWD(name, fp->get_unique_codepoint(fragment_index)); \
    SILVA_EXPECT_PARSE(name, cp == codepoint, "expected {}, got {}", codepoint, cp);        \
    fragment_index += 1;                                                                    \
  }

#define SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(name, frag_cat)                      \
  SILVA_EXPECT_PARSE(name,                                                        \
                     num_fragments_left() >= 1 &&                                 \
                         fragment_category_by() == fragment_category_t::frag_cat, \
                     "expected category {}, got {}",                              \
                     frag_cat,                                                    \
                     fragment_category_by());                                     \
  fragment_index += 1;

  struct parse_tree_nursery_state_t : public tree_nursery_state_t {
    index_t fragment_index = 0;
  };

  struct parse_tree_nursery_t
    : public tree_nursery_t<parse_tree_node_t, parse_tree_nursery_state_t, parse_tree_nursery_t> {
    syntax_farm_ptr_t sfp;
    fragmentization_ptr_t fp;

    tokenization_t tokenization;

    index_t fragment_index = 0;

    parse_tree_nursery_t(fragmentization_ptr_t);

    void on_get_state(parse_tree_nursery_state_t&) const;
    void on_set_state(const parse_tree_nursery_state_t&);

    void on_stake_ctor(parse_tree_node_t& proto_node) const;
    void on_stake_create_node(parse_tree_node_t& proto_node, name_id_t) const;
    void on_stake_add_proto_node(parse_tree_node_t& proto_node,
                                 const parse_tree_node_t& other) const;
    void on_stake_commit_pre(parse_tree_node_t& proto_node) const;
    void on_stake_commit_owning_to_proto(parse_tree_node_t& proto_node) const;

    parse_tree_ptr_t finish() &&;

    index_t num_fragments_left() const;
    const fragment_t* fragment_by(index_t idx_offset = 0) const;

    // Returns U'\0' if the given codepoint does not have a unique codepoint.
    unicode::codepoint_t fragment_unique_codepoint_or_zero_by(index_t idx_offset = 0) const;

    fragment_category_t fragment_category_by(index_t idx_offset = 0) const;
    fragment_location_t fragment_location_by(index_t idx_offset = 0) const;
    fragment_location_t fragment_location_at(index_t idx) const;
  };
}
