#pragma once

#include "seed.hpp"

#include <regex>

namespace silva {

  // Driver for a program in the Seed language.
  struct parse_root_t {
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

    // Maps the token-id's that correspond to regexes to the compiled version of that regex.
    hashmap_t<token_id_t, std::regex> regexes;

    expected_t<void> add_rule(string_view_t rule_name, index_t precendece, index_t expr_node_index);

    // Main parse_root_t constructor.
    static expected_t<parse_root_t> create(const_ptr_t<parse_tree_t>);

    // Convenience function for essentially
    //    parse_root_t::create(seed_parse(tokenize(source_code)))
    static expected_t<parse_root_t> create(const_ptr_t<source_code_t>);

    // Returns a parse-tree of the given "sprout_tokens" according to the language defined by the
    // "seed" parse-tree.
    struct workspace_t {
      struct per_seed_token_id_t {
        struct uncached_t {};
        variant_t<uncached_t, none_t, index_t> target_token_id = uncached_t{};
        optional_t<index_t> get_target_token_id(const tokenization_t::token_data_t* sp_token_data,
                                                const tokenization_t* target_tokenization);
      };
      vector_t<per_seed_token_id_t> seed_token_id_data;
    };
    expected_t<parse_tree_t> apply(const_ptr_t<tokenization_t>, workspace_t* = nullptr) const;
  };
}
