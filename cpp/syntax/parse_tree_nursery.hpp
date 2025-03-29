#pragma once

#include "parse_tree.hpp"

namespace silva {

  struct parse_tree_sub_t {
    index_t num_children = 0;
    index_t subtree_size = 0;
    index_t token_begin  = std::numeric_limits<index_t>::max();
    index_t token_end    = std::numeric_limits<index_t>::min();
    void operator+=(parse_tree_sub_t&& other);
  };

#define SILVA_EXPECT_PARSE(cond, fmt_str, ...) \
  SILVA_EXPECT(cond, MINOR, "{} " fmt_str, token_position_by() __VA_OPT__(, ) __VA_ARGS__);

  struct parse_tree_nursery_t {
    shared_ptr_t<const tokenization_t> tokenization;
    token_context_ptr_t tcp;
    vector_t<parse_tree_node_t> tree;
    index_t token_index = 0;

    parse_tree_nursery_t(shared_ptr_t<const tokenization_t>);

    struct stake_t {
      parse_tree_nursery_t* nursery = nullptr;
      index_t orig_node_size        = 0;
      index_t orig_token_index      = 0;
      parse_tree_sub_t sub;

      bool has_node = false;

      stake_t() = default;
      stake_t(parse_tree_nursery_t*);

      stake_t(stake_t&&);
      stake_t& operator=(stake_t&&);
      stake_t(const stake_t&)            = delete;
      stake_t& operator=(const stake_t&) = delete;

      void create_node(name_id_t);

      parse_tree_sub_t commit();
      void clear();
      ~stake_t();
    };

    [[nodiscard]] stake_t stake() { return stake_t{this}; }

    parse_tree_t commit_root() &&;

    const index_t num_tokens_left() const;
    const token_id_t token_id_by(index_t token_index_offset = 0) const;
    const token_info_t* token_data_by(index_t token_index_offset = 0) const;
    token_position_t token_position_by(index_t token_index_offset = 0) const;
    token_position_t token_position_at(index_t token_index) const;
  };
}
