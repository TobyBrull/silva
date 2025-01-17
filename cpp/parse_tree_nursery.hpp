#pragma once

#include "parse_tree.hpp"

namespace silva {

  struct parse_tree_sub_t {
    index_t num_children       = 0;
    index_t num_children_total = 0;

    void operator+=(parse_tree_sub_t&& other);
  };

  struct parse_tree_guard_t : public menhir_t {
    parse_tree_t* pt         = nullptr;
    index_t* token_index     = nullptr;
    index_t orig_node_size   = 0;
    index_t orig_token_index = 0;

    parse_tree_sub_t sub;

    [[nodiscard]] parse_tree_guard_t(parse_tree_t* pt, index_t* token_index);

    parse_tree_guard_t(parse_tree_guard_t&&)                 = delete;
    parse_tree_guard_t(const parse_tree_guard_t&)            = delete;
    parse_tree_guard_t& operator=(parse_tree_guard_t&&)      = delete;
    parse_tree_guard_t& operator=(const parse_tree_guard_t&) = delete;

    parse_tree_sub_t release();

    bool is_released() const;

    ~parse_tree_guard_t();
  };

  struct parse_tree_guard_for_rule_t : public parse_tree_guard_t {
    index_t node_index = 0;

    [[nodiscard]] parse_tree_guard_for_rule_t(parse_tree_t* pt, index_t* token_index);

    void set_rule_index(index_t rule_index);

    parse_tree_sub_t release();

    ~parse_tree_guard_for_rule_t();
  };

#define SILVA_EXPECT_PARSE(cond, fmt_str, ...) \
  SILVA_EXPECT(cond, MINOR, "{} " fmt_str, token_position_by() __VA_OPT__(, ) __VA_ARGS__);

  struct parse_tree_nursery_t {
    parse_tree_t retval;

    token_index_t token_index = 0;

    parse_tree_nursery_t(const_ptr_t<tokenization_t>, const_ptr_t<parse_root_t>);

    const index_t num_tokens_left() const;

    const token_id_t token_id_by(index_t token_index_offset = 0) const;

    const tokenization_t::token_data_t* token_data_by(index_t token_index_offset = 0) const;

    token_position_t token_position_by(index_t token_index_offset = 0) const;
    token_position_t token_position_at(index_t token_index) const;
  };
}
