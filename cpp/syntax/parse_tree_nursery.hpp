#pragma once

#include "parse_tree.hpp"

namespace silva {

  struct parse_tree_sub_t {
    index_t num_children       = 0;
    index_t num_children_total = 0;
    index_t token_begin        = std::numeric_limits<index_t>::max();
    index_t token_end          = std::numeric_limits<index_t>::min();
    void operator+=(parse_tree_sub_t&& other);
  };

  struct parse_tree_guard_t {
    parse_tree_t* pt         = nullptr;
    index_t* token_index     = nullptr;
    index_t orig_node_size   = 0;
    index_t orig_token_index = 0;
    parse_tree_sub_t sub;

    parse_tree_guard_t() = default;

    [[nodiscard]] parse_tree_guard_t(parse_tree_t* pt, index_t* token_index);

    parse_tree_guard_t(parse_tree_guard_t&&);
    parse_tree_guard_t& operator=(parse_tree_guard_t&&);

    parse_tree_guard_t(const parse_tree_guard_t&)            = delete;
    parse_tree_guard_t& operator=(const parse_tree_guard_t&) = delete;

    void swap(parse_tree_guard_t&);

    parse_tree_sub_t release();

    bool is_empty() const;

    void reset();

    ~parse_tree_guard_t();
  };

  struct parse_tree_guard_for_rule_t : public parse_tree_guard_t {
    index_t node_index       = 0;
    bool include_token_index = true;

    [[nodiscard]] parse_tree_guard_for_rule_t(parse_tree_t* pt,
                                              index_t* token_index,
                                              bool include_token_index = true);

    void set_rule_name(name_id_t);

    void sync();

    parse_tree_sub_t release();

    void implant(const parse_tree_t& other_pt, index_t other_node_index);

    ~parse_tree_guard_for_rule_t();
  };

#define SILVA_EXPECT_PARSE(cond, fmt_str, ...) \
  SILVA_EXPECT(cond, MINOR, "{} " fmt_str, token_position_by() __VA_OPT__(, ) __VA_ARGS__);

  struct parse_tree_nursery_t {
    token_context_ptr_t tcp;
    parse_tree_t retval;
    index_t token_index = 0;

    parse_tree_nursery_t(shared_ptr_t<const tokenization_t>);

    [[nodiscard]] parse_tree_guard_t guard() { return parse_tree_guard_t{&retval, &token_index}; }
    [[nodiscard]] parse_tree_guard_for_rule_t guard_for_rule(bool include_token_index = true)
    {
      return parse_tree_guard_for_rule_t{&retval, &token_index, include_token_index};
    }

    const index_t num_tokens_left() const;

    const token_id_t token_id_by(index_t token_index_offset = 0) const;

    const token_info_t* token_data_by(index_t token_index_offset = 0) const;

    token_position_t token_position_by(index_t token_index_offset = 0) const;
    token_position_t token_position_at(index_t token_index) const;
  };
}
