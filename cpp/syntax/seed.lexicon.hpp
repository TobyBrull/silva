#pragma once

#include "syntax/parse_tree.hpp"

namespace silva::seed {
  struct lexicon_t {
    syntax_farm_ptr_t sfp;

    token_id_t ti_dot         = *sfp->token_id(".");
    token_id_t ti_comma       = *sfp->token_id(",");
    token_id_t ti_dash        = *sfp->token_id("-");
    token_id_t ti_equal       = *sfp->token_id("=");
    token_id_t ti_axe         = *sfp->token_id("axe");
    token_id_t ti_alias       = *sfp->token_id("alias");
    token_id_t ti_right_arrow = *sfp->token_id("->");
    token_id_t ti_brack_open  = *sfp->token_id("[");
    token_id_t ti_brack_close = *sfp->token_id("]");
    token_id_t ti_paren_open  = *sfp->token_id("(");
    token_id_t ti_paren_close = *sfp->token_id(")");
    token_id_t ti_identifier  = *sfp->token_id("identifier");
    token_id_t ti_regex       = *sfp->token_id("/");
    token_id_t ti_up          = *sfp->token_id("p");
    token_id_t ti_silva       = *sfp->token_id("_");
    token_id_t ti_here        = *sfp->token_id("x");
    token_id_t ti_operator    = *sfp->token_id("operator");
    token_id_t ti_string      = *sfp->token_id("string");
    token_id_t ti_number      = *sfp->token_id("number");
    token_id_t ti_any         = *sfp->token_id("any");
    token_id_t ti_eps         = *sfp->token_id("epsilon");
    token_id_t ti_eof         = *sfp->token_id("end_of_file");
    token_id_t ti_nest        = *sfp->token_id("nest");
    token_id_t ti_ltr         = *sfp->token_id("ltr");
    token_id_t ti_rtl         = *sfp->token_id("rtl");
    token_id_t ti_atom_nest   = *sfp->token_id("atom_nest");
    token_id_t ti_postfix     = *sfp->token_id("postfix");
    token_id_t ti_postfix_n   = *sfp->token_id("postfix_nest");
    token_id_t ti_infix       = *sfp->token_id("infix");
    token_id_t ti_infix_flat  = *sfp->token_id("infix_flat");
    token_id_t ti_ternary     = *sfp->token_id("ternary");
    token_id_t ti_prefix      = *sfp->token_id("prefix");
    token_id_t ti_prefix_n    = *sfp->token_id("prefix_nest");
    token_id_t ti_concat      = *sfp->token_id("concat");
    token_id_t ti_keywords_of = *sfp->token_id("keywords_of");

    name_id_t ni_seed         = sfp->name_id_of("Seed");
    name_id_t ni_rule         = sfp->name_id_of(ni_seed, "Rule");
    name_id_t ni_alias        = sfp->name_id_of(ni_seed, "Alias");
    name_id_t ni_expr         = sfp->name_id_of(ni_seed, "Expr");
    name_id_t ni_atom         = sfp->name_id_of(ni_seed, "Atom");
    name_id_t ni_var          = sfp->name_id_of(ni_seed, "Variable");
    name_id_t ni_func         = sfp->name_id_of(ni_seed, "Function");
    name_id_t ni_func_name    = sfp->name_id_of(ni_func, "Name");
    name_id_t ni_func_arg     = sfp->name_id_of(ni_func, "Arg");
    name_id_t ni_func_args    = sfp->name_id_of(ni_func, "Args");
    name_id_t ni_axe          = sfp->name_id_of(ni_seed, "Axe");
    name_id_t ni_axe_level    = sfp->name_id_of(ni_axe, "Level");
    name_id_t ni_axe_assoc    = sfp->name_id_of(ni_axe, "Assoc");
    name_id_t ni_axe_ops      = sfp->name_id_of(ni_axe, "Ops");
    name_id_t ni_axe_op_type  = sfp->name_id_of(ni_axe, "OpType");
    name_id_t ni_axe_op       = sfp->name_id_of(ni_axe, "Op");
    name_id_t ni_nt_maybe_var = sfp->name_id_of(ni_seed, "NonterminalMaybeVar");
    name_id_t ni_nt           = sfp->name_id_of(ni_seed, "Nonterminal");
    name_id_t ni_nt_base      = sfp->name_id_of(ni_nt, "Base");
    name_id_t ni_term         = sfp->name_id_of(ni_seed, "Terminal");
    name_id_t ni_tok_cat      = sfp->name_id_of(ni_seed, "TokenCategory");
  };
}
