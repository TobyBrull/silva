#pragma once

#include "parse_axe.hpp"
#include "seed.hpp"

#include <regex>

namespace silva {

  // Driver for a program in the Seed language.
  struct parse_root_t {
    // Parse trees containing the used Seed programs.
    vector_t<shared_ptr_t<const parse_tree_t>> seed_parse_trees;

    struct tree_node_index_t {
      // Index in the "seed_parse_trees" vector_t.
      index_t tree_index = 0;
      // Node-index in the parse_tree_t referenced by "tree_index".
      index_t node_index = 0;

      tree_node_index_t with_node_index(index_t) const;

      friend auto operator<=>(const tree_node_index_t&, const tree_node_index_t&) = default;
      friend hash_value_t hash_impl(const tree_node_index_t& x);
    };

    // For each rule name, gives the node-index of the expression describing that rule.
    hashmap_t<full_name_id_t, tree_node_index_t> rule_exprs;

    // For each node-index that is a "Silva.Seed.Nonterminal", gives the full name of the rule that
    // this nonterminal references.
    hashmap_t<tree_node_index_t, full_name_id_t> nonterminal_rules;

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
    expected_t<unique_ptr_t<parse_tree_t>> apply(shared_ptr_t<const tokenization_t>,
                                                 full_name_id_t goal_rule_name) const;
  };
}
