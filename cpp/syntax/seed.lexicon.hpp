#pragma once

#include "syntax/parse_tree.hpp"

namespace silva::seed {
  struct lexicon_t {
    syntax_farm_ptr_t sfp;

    lexicon_t(syntax_farm_ptr_t sfp) : sfp(sfp) {}

    const token_id_t ti_dot          = sfp->token_id(".");
    const token_id_t ti_comma        = sfp->token_id(",");
    const token_id_t ti_dash         = sfp->token_id("-");
    const token_id_t ti_equal        = sfp->token_id("=");
    const token_id_t ti_axe          = sfp->token_id("axe");
    const token_id_t ti_alias        = sfp->token_id("alias");
    const token_id_t ti_right_arrow  = sfp->token_id("->");
    const token_id_t ti_brack_open   = sfp->token_id("[");
    const token_id_t ti_brack_close  = sfp->token_id("]");
    const token_id_t ti_paren_open   = sfp->token_id("(");
    const token_id_t ti_paren_close  = sfp->token_id(")");
    const token_id_t ti_identifier   = sfp->token_id("identifier");
    const token_id_t ti_slash        = sfp->token_id("/");
    const token_id_t ti_up           = sfp->token_id("p");
    const token_id_t ti_silva        = sfp->token_id("_");
    const token_id_t ti_here         = sfp->token_id("x");
    const token_id_t ti_operator     = sfp->token_id("operator");
    const token_id_t ti_string       = sfp->token_id("string");
    const token_id_t ti_number       = sfp->token_id("number");
    const token_id_t ti_any          = sfp->token_id("any");
    const token_id_t ti_eps          = sfp->token_id("epsilon");
    const token_id_t ti_eof          = sfp->token_id("end_of_file");
    const token_id_t ti_nest         = sfp->token_id("nest");
    const token_id_t ti_ltr          = sfp->token_id("ltr");
    const token_id_t ti_rtl          = sfp->token_id("rtl");
    const token_id_t ti_atom_nest    = sfp->token_id("atom_nest");
    const token_id_t ti_postfix      = sfp->token_id("postfix");
    const token_id_t ti_postfix_n    = sfp->token_id("postfix_nest");
    const token_id_t ti_infix        = sfp->token_id("infix");
    const token_id_t ti_infix_flat   = sfp->token_id("infix_flat");
    const token_id_t ti_ternary      = sfp->token_id("ternary");
    const token_id_t ti_prefix       = sfp->token_id("prefix");
    const token_id_t ti_prefix_n     = sfp->token_id("prefix_nest");
    const token_id_t ti_concat       = sfp->token_id("concat");
    const token_id_t ti_keywords_of  = sfp->token_id("keywords_of");
    const token_id_t ti_tokenizer    = sfp->token_id("tokenizer");
    const token_id_t ti_include      = sfp->token_id("include");
    const token_id_t ti_ignore       = sfp->token_id("ignore");
    const token_id_t ti_triple_colon = sfp->token_id(":::");
    const token_id_t ti_backslash    = sfp->token_id("\\");
    const token_id_t ti_pipe         = sfp->token_id("|");
    const token_id_t ti_qmark        = sfp->token_id("?");
    const token_id_t ti_star         = sfp->token_id("*");
    const token_id_t ti_plus         = sfp->token_id("+");

    const token_id_t ti_indent         = sfp->token_id("indent");
    const token_id_t ti_dedent         = sfp->token_id("dedent");
    const token_id_t ti_newline        = sfp->token_id("newline");
    const token_id_t ti_rule_name      = sfp->token_id("rule_name");
    const token_id_t ti_func_name      = sfp->token_id("func_name");
    const token_id_t ti_token_cat_name = sfp->token_id("token_category_name");
    const token_id_t ti_frag_name      = sfp->token_id("frag_name");

    const token_id_t ti_WHITESPACE             = sfp->token_id("WHITESPACE");
    const token_id_t ti_COMMENT                = sfp->token_id("COMMENT");
    const token_id_t ti_NUMBER                 = sfp->token_id("NUMBER");
    const token_id_t ti_STRING                 = sfp->token_id("STRING");
    const token_id_t ti_INDENT                 = sfp->token_id("INDENT");
    const token_id_t ti_DEDENT                 = sfp->token_id("DEDENT");
    const token_id_t ti_NEWLINE                = sfp->token_id("NEWLINE");
    const token_id_t ti_OPERATOR               = sfp->token_id("OPERATOR");
    const token_id_t ti_PARENTHESIS            = sfp->token_id("PARENTHESIS");
    const token_id_t ti_IDENTIFIER             = sfp->token_id("IDENTIFIER");
    const token_id_t ti_IDENTIFIER_SILVA_CASE  = sfp->token_id("IDENTIFIER_SILVA_CASE");
    const token_id_t ti_IDENTIFIER_SNAKE_CASE  = sfp->token_id("IDENTIFIER_SNAKE_CASE");
    const token_id_t ti_IDENTIFIER_CAMEL_CASE  = sfp->token_id("IDENTIFIER_CAMEL_CASE");
    const token_id_t ti_IDENTIFIER_PASCAL_CASE = sfp->token_id("IDENTIFIER_PASCAL_CASE");
    const token_id_t ti_IDENTIFIER_MACRO_CASE  = sfp->token_id("IDENTIFIER_MACRO_CASE");
    const token_id_t ti_IDENTIFIER_UPPER_CASE  = sfp->token_id("IDENTIFIER_UPPER_CASE");
    const token_id_t ti_IDENTIFIER_LOWER_CASE  = sfp->token_id("IDENTIFIER_LOWER_CASE");

    const token_id_t ti_r_defaults = sfp->token_id("Defaults");
    const token_id_t ti_r_offside  = sfp->token_id("OffSide");
    const token_id_t ti_r_freeform = sfp->token_id("FreeForm");
    const token_id_t ti_r_seed     = sfp->token_id("Seed");
    const token_id_t ti_r_fern     = sfp->token_id("Fern");

    const name_id_t ni_seed    = sfp->name_id_of("Seed");
    const name_id_t ni_rule    = sfp->name_id_of(ni_seed, "Rule");
    const name_id_t ni_alias   = sfp->name_id_of(ni_seed, "Alias");
    const name_id_t ni_expr    = sfp->name_id_of(ni_seed, "Expr");
    const name_id_t ni_atom    = sfp->name_id_of(ni_seed, "Atom");
    const name_id_t ni_term    = sfp->name_id_of(ni_seed, "Terminal");
    const name_id_t ni_tok_cat = sfp->name_id_of(ni_seed, "TokenCategory");

    const name_id_t ni_expr_parens  = sfp->name_id_of(ni_expr, "Parens");
    const name_id_t ni_expr_prefix  = sfp->name_id_of(ni_expr, "Prefix");
    const name_id_t ni_expr_postfix = sfp->name_id_of(ni_expr, "Postfix");
    const name_id_t ni_expr_concat  = sfp->name_id_of(ni_expr, "Concat");
    const name_id_t ni_expr_or      = sfp->name_id_of(ni_expr, "Or");
    const name_id_t ni_expr_and     = sfp->name_id_of(ni_expr, "And");

    const name_id_t ni_nt      = sfp->name_id_of(ni_seed, "Nonterminal");
    const name_id_t ni_nt_base = sfp->name_id_of(ni_nt, "Base");

    const name_id_t ni_func      = sfp->name_id_of(ni_seed, "Function");
    const name_id_t ni_func_name = sfp->name_id_of(ni_func, "Name");
    const name_id_t ni_func_arg  = sfp->name_id_of(ni_func, "Arg");
    const name_id_t ni_func_args = sfp->name_id_of(ni_func, "Args");

    const name_id_t ni_axe         = sfp->name_id_of(ni_seed, "Axe");
    const name_id_t ni_axe_level   = sfp->name_id_of(ni_axe, "Level");
    const name_id_t ni_axe_assoc   = sfp->name_id_of(ni_axe, "Assoc");
    const name_id_t ni_axe_ops     = sfp->name_id_of(ni_axe, "Ops");
    const name_id_t ni_axe_op_type = sfp->name_id_of(ni_axe, "OpType");
    const name_id_t ni_axe_op      = sfp->name_id_of(ni_axe, "Op");

    const name_id_t ni_tok             = sfp->name_id_of(ni_seed, "Tokenizer");
    const name_id_t ni_tok_inc_rule    = sfp->name_id_of(ni_tok, "IncludeRule");
    const name_id_t ni_tok_ign_rule    = sfp->name_id_of(ni_tok, "IgnoreRule");
    const name_id_t ni_tok_tok_rule    = sfp->name_id_of(ni_tok, "TokenRule");
    const name_id_t ni_tok_defn        = sfp->name_id_of(ni_tok, "Defn");
    const name_id_t ni_tok_prefix_item = sfp->name_id_of(ni_tok, "PrefixItem");
    const name_id_t ni_tok_item        = sfp->name_id_of(ni_tok, "Item");
    const name_id_t ni_tok_matcher     = sfp->name_id_of(ni_tok, "Matcher");
    const name_id_t ni_tok_list        = sfp->name_id_of(ni_tok, "List");
  };
}
