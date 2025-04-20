#pragma once

#include "parse_axe.hpp"
#include "seed.hpp"
#include "syntax_ward.hpp"

#include <regex>

namespace silva {

  // Driver for a program in the Seed language.
  struct seed_engine_t {
    syntax_ward_ptr_t scp;

    // For each rule name, gives the node-index of the expression describing that rule.
    hashmap_t<name_id_t, parse_tree_span_t> rule_exprs;

    // For each node-index that is a "Silva.Seed.Nonterminal", gives the full name of the rule that
    // this nonterminal references.
    hashmap_t<parse_tree_span_t, name_id_t> nonterminal_rules;

    // Maps the token-id's that correspond to regexes to the compiled version of that regex.
    hashmap_t<token_id_t, optional_t<std::regex>> regexes;

    // Maps the rule-name of a parse-axe to the corresponding parse-axe.
    struct parse_axe_data_t {
      name_id_t atom_rule_name = name_id_root;
      parse_axe::parse_axe_t parse_axe;
    };
    hashmap_t<name_id_t, parse_axe_data_t> parse_axes;

    // Maps the scope/rule-name to the set of keywords associated with it.
    hashmap_t<name_id_t, hashset_t<token_id_t>> keyword_scopes;

    // Maps a token of the form ['keyword'] (i.e., of category: string) to a token of the form
    // [keyword] (i.e., of category: identifier or operator).
    hashmap_t<token_id_t, token_id_t> string_to_keyword;

    seed_engine_t(syntax_ward_ptr_t);

    // The given parse_tree_span_t should be part of one of the "seed_parse_trees".
    expected_t<void> add(parse_tree_span_t);

    expected_t<parse_tree_ptr_t> add_complete_file(filesystem_path_t filepath, string_view_t text);

    // Returns a parse-tree of the given "sprout_tokens" according to the language defined by the
    // "seed" parse-tree.
    expected_t<parse_tree_ptr_t>
    apply(syntax_ward_t&, tokenization_ptr_t, name_id_t goal_rule_name) const;
  };
}
