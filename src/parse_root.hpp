#pragma once

#include "seed.hpp"

namespace silva {

  // Driver for a program in the Seed language.
  struct parse_root_t : public sprite_t {
    // Parse tree of the Seed program for this driver.
    const_ptr_t<parse_tree_t> seed_parse_tree;

    struct rule_t {
      // Name of nonterminal that the rule defines.
      string_view_t name;
      // Lower value means higher precedence.
      index_t precedence = 0;
      // Node in the "seed_parse_tree" that contains the expression for this rule.
      index_t expr_node_index = 0;
    };
    vector_t<rule_t> rules;

    // Maps a rule name to the first rule with that name (index == 0).
    hashmap_t<string_view_t, index_t> rule_name_offsets;

    string_view_t goal_rule_name;

    expected_t<void> add_rule(string_view_t rule_name, index_t precendece, index_t expr_node_index);

    // Main parse_root_t constructor.
    static expected_t<parse_root_t> create(const_ptr_t<parse_tree_t>);

    // Convenience function for essentially
    //    parse_root_t::create(seed_parse(tokenize(source_code)))
    static expected_t<parse_root_t> create(const_ptr_t<source_code_t>);

    // Returns a parse-tree of the given "sprout_tokens" according to the language defined by the
    // "seed" parse-tree.
    struct workspace_t {
      vector_t<optional_t<index_t>> seed_token_id_to_target_token_id;
    };
    expected_t<parse_tree_t> apply(const_ptr_t<tokenization_t>, workspace_t* = nullptr) const;
  };
}
