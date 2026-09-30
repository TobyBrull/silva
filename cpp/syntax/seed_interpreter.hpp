#pragma once

#include "seed.hpp"
#include "seed_axe.hpp"

namespace silva::seed {
  // Driver for a program in the Seed language.
  struct interpreter_t {
    syntax_farm_ptr_t sfp;
    bootstrap_interpreter_t bootstrap_interpreter;

    struct common_data_t {
      // Maps a rule/scope name to all string-literal tokens occuring inside that scope (including
      // nested rules); used to implement the "literals_of" mechanism.
      array_t<fragmented_token_t> scope_to_literals;
    };
    struct rule_data_t : public common_data_t {
      parse_tree_span_t expr;
      bool is_twig_rule     = false;
      bool is_no_node       = false;
      bool is_no_whitespace = false;
      bool is_literal_nodes = false;
    };
    struct axe_data_t : public rule_data_t {
      unique_ptr_t<axe_t> axe;
    };
    struct axe_level_data_t : public common_data_t {
      parse_tree_span_t expr;
      const axe_data_t* axe_data = nullptr;
    };
    struct scope_data_t : public common_data_t {
      // hash_set_t<name_id_t> sub_rules;
      // hash_set_t<name_id_t> sub_scopes;
    };
    struct language_data_t : public scope_data_t {
      parse_tree_span_t pts;
      name_id_t skip_rule_name;
      optional_t<rule_data_t> skip_rule_expr;
      name_id_t skip_initial_rule_name;
      optional_t<rule_data_t> skip_initial_rule_expr;
    };
    using definition_t =
        variant_t<language_data_t, scope_data_t, rule_data_t, axe_data_t, axe_level_data_t>;
    hash_map_t<name_id_t, definition_t> definitions;

    // Maps a token of the form ['word'] (i.e., of category: string) to a token of the form [word]
    // (i.e., of category: identifier or operator).
    hash_map_t<token_id_t, fragmented_token_t> string_to_ft;

    struct sub_expr_data_t {
      // If a concat expression contains a '~', this member contains the index of the '~' in that
      // concat expression.
      optional_t<index_t> commit_after;
    };
    hash_map_t<parse_tree_span_t, sub_expr_data_t> sub_expr_data;

    interpreter_t(syntax_farm_ptr_t);

    expected_t<void> add_seed(parse_tree_span_t pts_seed);
    expected_t<void> add_seed_copy(const parse_tree_span_t& pts_seed);
    expected_t<parse_tree_ptr_t> add_seed(fragment_span_t);
    expected_t<parse_tree_ptr_t> add_seed_text(filepath_t, string_t);

    void compile_reset();
    expected_t<void> compile();
    bool is_compiled = false;

    // For each parse_tree_span_t with rule-name = .Seed.Nonterminal, contains the full name of the
    // rule that this nonterminal references, taking into account the relative scope in which the
    // rule was encountered.
    hash_set_t<name_id_ref_t> resolved_names;

    expected_t<parse_tree_ptr_t> apply(fragment_span_t, name_id_t goal_rule_name);
    expected_t<parse_tree_ptr_t> apply_text(filepath_t, string_t, name_id_t goal_rule_name);
  };
}
