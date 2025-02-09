#pragma once

#include "seed.hpp"

#include <regex>

namespace silva {

  // Driver for a program in the Seed language.
  struct parse_root_t {
    // Parse tree of the Seed program for this driver.
    const_ptr_t<parse_tree_t> seed_parse_tree;

    struct rule_t {
      // Token-id of nonterminal that the rule defines.
      token_info_index_t token_id;
      // Lower value means higher precedence.
      index_t precedence = 0;
      // Name of the rule (can be inferred from the "token_id", but kept for convenience).
      string_view_t name;
      // Node in the "seed_parse_tree" that contains the expression for this rule.
      index_t expr_node_index = 0;
      // If this rule is an alias, contains the offset of the aliased rule.
      optional_t<index_t> aliased_rule_offset;
    };
    vector_t<rule_t> rules;

    // Maps a token-id corresponding to a rule-name to the first rule with that name (index == 0).
    hashmap_t<token_info_index_t, index_t> rule_indexes;

    token_info_index_t goal_rule_token_id = 0;

    // Maps the token-id's that correspond to regexes to the compiled version of that regex.
    hashmap_t<token_info_index_t, optional_t<std::regex>> regexes;

    // Main parse_root_t constructor.
    static expected_t<parse_root_t> create(const_ptr_t<parse_tree_t>);

    // Convenience function for essentially
    //    parse_root_t::create(seed_parse(tokenization))
    static expected_t<parse_root_t> create(const tokenization_t*);

    // Returns a parse-tree of the given "sprout_tokens" according to the language defined by the
    // "seed" parse-tree.
    struct workspace_t {
      struct per_seed_token_id_t {
        struct uncached_t {};
        variant_t<uncached_t, none_t, index_t> target_token_id = uncached_t{};
        optional_t<index_t> get_target_token_id(const token_info_t* sp_token_data,
                                                const tokenization_t* target_tokenization);
      };
      vector_t<per_seed_token_id_t> seed_token_id_data;
    };
    expected_t<parse_tree_t> apply(const tokenization_t*, workspace_t* = nullptr) const;
  };
}
