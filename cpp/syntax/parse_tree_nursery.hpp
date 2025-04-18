#pragma once

#include "canopy/tree_nursery.hpp"

#include "parse_tree.hpp"

namespace silva {

#define SILVA_EXPECT_PARSE(name, cond, fmt_str, ...) \
  SILVA_EXPECT(cond,                                 \
               MINOR,                                \
               "[{}] {}: " fmt_str,                  \
               token_position_by(),                  \
               tcp->name_id_wrap(name) __VA_OPT__(, ) __VA_ARGS__);
#define SILVA_EXPECT_PARSE_FWD(name, expr) \
  SILVA_EXPECT_FWD(expr, "[{}] {}", token_position_by(), tcp->name_id_wrap(name))

  struct parse_tree_nursery_state_t : public tree_nursery_state_t {
    index_t token_index = 0;
  };

  struct parse_tree_nursery_t
    : public tree_nursery_t<parse_tree_node_t, parse_tree_nursery_state_t, parse_tree_nursery_t> {
    shared_ptr_t<const tokenization_t> tokenization;
    token_catalog_ptr_t tcp;
    index_t token_index = 0;

    parse_tree_nursery_t(shared_ptr_t<const tokenization_t>);

    void on_get_state(parse_tree_nursery_state_t&) const;
    void on_set_state(const parse_tree_nursery_state_t&);

    void on_stake_create_node(parse_tree_node_t& proto_node, name_id_t) const;
    void on_stake_add_proto_node(parse_tree_node_t& proto_node,
                                 const parse_tree_node_t& other) const;
    void on_stake_commit_pre(parse_tree_node_t& proto_node) const;
    void on_stake_commit_owning_to_proto(parse_tree_node_t& proto_node) const;

    parse_tree_t finish() &&;

    const index_t num_tokens_left() const;
    const token_id_t token_id_by(index_t token_index_offset = 0) const;
    const token_info_t* token_data_by(index_t token_index_offset = 0) const;
    token_position_t token_position_by(index_t token_index_offset = 0) const;
    token_position_t token_position_at(index_t token_index) const;
  };
}
