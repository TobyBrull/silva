#pragma once

#include "parse_axe.hpp"
#include "seed.hpp"

#include <regex>

namespace silva {

  // Driver for a program in the Seed language.
  struct parse_root_t {
    // Parse tree of the Seed program for this driver.
    shared_ptr_t<const parse_tree_t> seed_parse_tree;

    struct rule_t {
      full_name_id_t name = full_name_id_none;
      // Node in the "seed_parse_tree" that contains the expression for this rule.
      index_t expr_node_index = 0;
    };
    vector_t<rule_t> rules;

    // Maps a rule name to the rule with that name (index == 0).
    hashmap_t<full_name_id_t, index_t> rule_indexes;

    full_name_id_t goal_rule = 0;

    // Maps the token-id's that correspond to regexes to the compiled version of that regex.
    hashmap_t<token_id_t, optional_t<std::regex>> regexes;

    // Maps the rule-name of a parse-axe to the corresponding parse-axe.
    struct parse_axe_data_t {
      full_name_id_t atom_rule_name = full_name_id_none;
      parse_axe::parse_axe_t parse_axe;
    };
    hashmap_t<full_name_id_t, parse_axe_data_t> parse_axes;

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
