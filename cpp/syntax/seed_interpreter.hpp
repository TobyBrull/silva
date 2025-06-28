#pragma once

#include "seed_axe.hpp"

#include <regex>

namespace silva::seed {
  // Driver for a program in the Seed language.
  struct interpreter_t {
    syntax_ward_ptr_t swp;

    // For each rule name, gives the node-index of the expression describing that rule.
    hash_map_t<name_id_t, parse_tree_span_t> rule_exprs;

    // For each node-index that is a "_.Seed.Nonterminal", gives the full name of the rule that this
    // nonterminal references, taking into account the relative scope in which the rule was
    // encountered.
    hash_map_t<parse_tree_span_t, name_id_t> nonterminal_rules;

    // Maps the token-id's that correspond to regexes to the compiled version of that regex.
    hash_map_t<token_id_t, optional_t<std::regex>> regexes;

    // Maps the rule-name of a seed-axe to the corresponding seed-axe.
    hash_map_t<name_id_t, seed_axe_t> seed_axes;

    // Maps the scope/rule-name to the set of keywords associated with it.
    hash_map_t<name_id_t, hash_set_t<token_id_t>> keyword_scopes;

    // Maps a token of the form ['keyword'] (i.e., of category: string) to a token of the form
    // [keyword] (i.e., of category: identifier or operator).
    hash_map_t<token_id_t, token_id_t> string_to_keyword;

    // Callbacks triggered by Seed function "parse_and_callback_f".
    using callback_t = delegate_t<expected_t<void>(const parse_tree_span_t&)>;
    hash_map_t<name_id_t, callback_t> parse_callbacks;
    expected_t<void> callback_if(const parse_tree_span_t&) const;

    interpreter_t(syntax_ward_ptr_t);

    // The given parse_tree_span_t should be part of one of the "seed_parse_trees".
    expected_t<void> add(parse_tree_span_t);
    expected_t<void> add_copy(const parse_tree_span_t&);

    expected_t<parse_tree_ptr_t> add_complete_file(filesystem_path_t filepath, string_view_t text);

    // Returns a parse-tree of the given "sprout_tokens" according to the language defined by the
    // "seed" parse-tree.
    expected_t<parse_tree_ptr_t> apply(tokenization_ptr_t, name_id_t goal_rule_name) const;
  };
}
