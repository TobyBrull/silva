#pragma once

#include "seed.hpp"
#include "seed_axe.hpp"
#include "seed_tokenizer.hpp"

namespace silva::seed {
  // Driver for a program in the Seed language.
  struct interpreter_t {
    syntax_farm_ptr_t sfp;
    bootstrap_interpreter_t bootstrap_interpreter;

    tokenizer_farm_t tokenizer_farm;

    // For each rule name, gives the node-index of the expression describing that rule.
    hash_map_t<name_id_t, parse_tree_span_t> rule_exprs;

    // Maps the rule-name of a seed-axe to the corresponding seed-axe.
    hash_map_t<name_id_t, axe_t> axes;

    // Maps a token of the form ['word'] (i.e., of category: string) to a token of the form [word]
    // (i.e., of category: identifier or operator).
    hash_map_t<token_id_t, token_id_t> string_to_token;

    // For each node-index that is a "_.Seed.Nonterminal", gives the full name of the rule that this
    // nonterminal references, taking into account the relative scope in which the rule was
    // encountered.
    hash_map_t<parse_tree_span_t, name_id_t> resolved_names;

    interpreter_t(syntax_farm_ptr_t);

    expected_t<void> add_seed(parse_tree_span_t pts_seed);
    expected_t<void> add_seed_copy(const parse_tree_span_t& pts_seed);
    expected_t<parse_tree_ptr_t> add_seed(fragment_span_t);
    expected_t<parse_tree_ptr_t> add_seed_text(filepath_t, string_t);

    expected_t<parse_tree_ptr_t> apply(fragment_span_t, name_id_t goal_rule_name);
    expected_t<parse_tree_ptr_t> apply_text(filepath_t, string_t, name_id_t goal_rule_name);
  };
}
