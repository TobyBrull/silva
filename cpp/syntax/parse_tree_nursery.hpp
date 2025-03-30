#pragma once

#include "canopy/tree_nursery.hpp"

#include "parse_tree.hpp"

namespace silva {

#define SILVA_EXPECT_PARSE(cond, fmt_str, ...) \
  SILVA_EXPECT(cond, MINOR, "{} " fmt_str, token_position_by() __VA_OPT__(, ) __VA_ARGS__);

  struct parse_tree_nursery_t : public tree_nursery_t<parse_tree_node_t> {
    shared_ptr_t<const tokenization_t> tokenization;
    token_context_ptr_t tcp;
    index_t token_index = 0;

    parse_tree_nursery_t(shared_ptr_t<const tokenization_t>);

    struct state_t : public tree_nursery_t::state_t {
      state_t() = default;
      state_t(const parse_tree_nursery_t*);

      index_t token_index = 0;
    };
    void set_state_derived(const state_t&);

    struct stake_t {
      parse_tree_nursery_t* nursery = nullptr;
      state_t orig_state;
      parse_tree_node_t proto_node;

      bool owns_node = false;

      stake_t(parse_tree_nursery_t*);

      stake_t(stake_t&&)                 = delete;
      stake_t& operator=(stake_t&&)      = delete;
      stake_t(const stake_t&)            = delete;
      stake_t& operator=(const stake_t&) = delete;

      void create_node(name_id_t);

      void add_proto_node(const parse_tree_node_t&);

      parse_tree_node_t commit();
      void clear();
      ~stake_t();
    };
    [[nodiscard]] stake_t stake() { return stake_t{this}; }

    parse_tree_t finish() &&;

    const index_t num_tokens_left() const;
    const token_id_t token_id_by(index_t token_index_offset = 0) const;
    const token_info_t* token_data_by(index_t token_index_offset = 0) const;
    token_position_t token_position_by(index_t token_index_offset = 0) const;
    token_position_t token_position_at(index_t token_index) const;
  };
}
