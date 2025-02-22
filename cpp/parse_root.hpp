#pragma once

#include "seed.hpp"

#include <regex>

namespace silva {

  // Driver for a program in the Seed language.
  struct parse_root_t {
    // Parse tree of the Seed program for this driver.
    shared_ptr_t<const parse_tree_t> seed_parse_tree;

    struct rule_t {
      // Token-id of nonterminal that the rule defines.
      token_id_t token_id;
      // Lower value means higher precedence.
      index_t precedence = 0;
      // Combination of token_id and precedence atm.
      full_name_id_t rule_name = full_name_id_none;
      // Node in the "seed_parse_tree" that contains the expression for this rule.
      index_t expr_node_index = 0;
      // If this rule is an alias, contains the offset of the aliased rule.
      optional_t<index_t> aliased_rule_offset;
    };
    vector_t<rule_t> rules;

    // Maps a token-id corresponding to a rule-name to the first rule with that name (index == 0).
    hashmap_t<token_id_t, index_t> rule_indexes;

    token_id_t goal_rule_token_id = 0;

    // Maps the token-id's that correspond to regexes to the compiled version of that regex.
    hashmap_t<token_id_t, optional_t<std::regex>> regexes;

    // Main parse_root_t constructor.
    static expected_t<unique_ptr_t<parse_root_t>> create(shared_ptr_t<const parse_tree_t>);

    // Convenience function for essentially
    //    tokenize | seed_parse | parse_root_t::create
    static expected_t<unique_ptr_t<parse_root_t>>
    create(token_context_ptr_t, filesystem_path_t filepath, string_t text);

    // Returns a parse-tree of the given "sprout_tokens" according to the language defined by the
    // "seed" parse-tree.
    expected_t<unique_ptr_t<parse_tree_t>> apply(shared_ptr_t<const tokenization_t>) const;
  };
}
