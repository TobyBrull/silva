#pragma once

#include "fragmentization.hpp"

namespace silva::seed {
  struct lexicon_t : public silva::lexicon_t {
   private:
    lexicon_t(syntax_farm_ptr_t sfp) : silva::lexicon_t(sfp)
    {
      language_name = sfp->token_id("Seed");
    }
    friend struct silva::syntax_farm_t;

   public:
    const fragmented_token_t ti_dot           = *fragmented_token(sfp, ".");
    const fragmented_token_t ti_here          = *fragmented_token(sfp, "⊙");
    const fragmented_token_t ti_comma         = *fragmented_token(sfp, ",");
    const fragmented_token_t ti_dash          = *fragmented_token(sfp, "-");
    const fragmented_token_t ti_equal         = *fragmented_token(sfp, "=");
    const fragmented_token_t ti_axe           = *fragmented_token(sfp, "axe", true);
    const fragmented_token_t ti_alias         = *fragmented_token(sfp, "alias", true);
    const fragmented_token_t ti_no_whitespace = *fragmented_token(sfp, "no_whitespace", true);
    const fragmented_token_t ti_right_arrow   = *fragmented_token(sfp, "->");
    const fragmented_token_t ti_brack_open    = *fragmented_token(sfp, "[");
    const fragmented_token_t ti_brack_close   = *fragmented_token(sfp, "]");
    const fragmented_token_t ti_brace_open    = *fragmented_token(sfp, "{");
    const fragmented_token_t ti_brace_close   = *fragmented_token(sfp, "}");
    const fragmented_token_t ti_paren_open    = *fragmented_token(sfp, "(");
    const fragmented_token_t ti_paren_close   = *fragmented_token(sfp, ")");
    const fragmented_token_t ti_identifier    = *fragmented_token(sfp, "identifier", true);
    const fragmented_token_t ti_slash         = *fragmented_token(sfp, "/");
    const fragmented_token_t ti_ltr           = *fragmented_token(sfp, "ltr", true);
    const fragmented_token_t ti_rtl           = *fragmented_token(sfp, "rtl", true);
    const fragmented_token_t ti_postfix       = *fragmented_token(sfp, "postfix", true);
    const fragmented_token_t ti_postfix_n     = *fragmented_token(sfp, "postfix_nest", true);
    const fragmented_token_t ti_infix         = *fragmented_token(sfp, "infix", true);
    const fragmented_token_t ti_infix_flat    = *fragmented_token(sfp, "infix_flat", true);
    const fragmented_token_t ti_ternary       = *fragmented_token(sfp, "ternary", true);
    const fragmented_token_t ti_prefix        = *fragmented_token(sfp, "prefix", true);
    const fragmented_token_t ti_prefix_n      = *fragmented_token(sfp, "prefix_nest", true);
    const fragmented_token_t ti_concat        = *fragmented_token(sfp, "concat", true);
    const fragmented_token_t ti_not           = *fragmented_token(sfp, "not", true);
    const fragmented_token_t ti_but_then      = *fragmented_token(sfp, "but_then", true);
    const fragmented_token_t ti_language      = *fragmented_token(sfp, "language", true);
    const fragmented_token_t ti_colon         = *fragmented_token(sfp, ":");
    const fragmented_token_t ti_triple_colon  = *fragmented_token(sfp, ":::");
    const fragmented_token_t ti_backslash     = *fragmented_token(sfp, "\\");
    const fragmented_token_t ti_pipe          = *fragmented_token(sfp, "|");
    const fragmented_token_t ti_qmark         = *fragmented_token(sfp, "?");
    const fragmented_token_t ti_star          = *fragmented_token(sfp, "*");
    const fragmented_token_t ti_plus          = *fragmented_token(sfp, "+");
    const fragmented_token_t ti_eps           = *fragmented_token(sfp, "ε");
    const fragmented_token_t ti_end_of_lang   = *fragmented_token(sfp, "end_of_language", true);
    const fragmented_token_t ti_skip          = *fragmented_token(sfp, "skip", true);

    const fragmented_token_t ti_ID_START    = *fragmented_token(sfp, "ID_START");
    const fragmented_token_t ti_ID_CONTINUE = *fragmented_token(sfp, "ID_CONTINUE");

    const token_id_t ti_r_defaults = sfp->token_id("Defaults");
    const token_id_t ti_r_offside  = sfp->token_id("OffSide");
    const token_id_t ti_r_freeform = sfp->token_id("FreeForm");
    const token_id_t ti_r_seed     = sfp->token_id("Seed");
    const token_id_t ti_r_fern     = sfp->token_id("Fern");

    const name_id_t ni_id              = sfp->name_id_of("identifier");
    const name_id_t ni_id_snake        = sfp->name_id_of("identifier_snake_case");
    const name_id_t ni_id_pascal       = sfp->name_id_of("identifier_pascal_case");
    const name_id_t ni_id_macro        = sfp->name_id_of("identifier_macro_case");
    const name_id_t ni_string          = sfp->name_id_of("string");
    const name_id_t ni_number          = sfp->name_id_of("number");
    const name_id_t ni_newline         = sfp->name_id_of("newline");
    const name_id_t ni_indent          = sfp->name_id_of("indent");
    const name_id_t ni_dedent          = sfp->name_id_of("dedent");
    const name_id_t ni_operator_single = sfp->name_id_of("operator_single");
    const name_id_t ni_operator_greedy = sfp->name_id_of("operator_greedy");

    const name_id_t ni_seed = sfp->name_id_of("Seed");

    const name_id_t ni_frag_name      = sfp->name_id_of(ni_seed, "frag_name");
    const name_id_t ni_rule_name      = sfp->name_id_of(ni_seed, "rule_name");
    const name_id_t ni_token_cat_name = sfp->name_id_of(ni_seed, "token_category_name");

    const name_id_t ni_language    = sfp->name_id_of(ni_seed, "Language");
    const name_id_t ni_scope       = sfp->name_id_of(ni_seed, "Scope");
    const name_id_t ni_rule        = sfp->name_id_of(ni_seed, "Rule");
    const name_id_t ni_qualifier   = sfp->name_id_of(ni_seed, "Qualifier");
    const name_id_t ni_expr        = sfp->name_id_of(ni_seed, "Expr");
    const name_id_t ni_atom        = sfp->name_id_of(ni_expr, "Atom");
    const name_id_t ni_alternation = sfp->name_id_of(ni_expr, "Alternation");
    const name_id_t ni_quantifier  = sfp->name_id_of(ni_expr, "Quantifier");
    const name_id_t ni_oper        = sfp->name_id_of(ni_expr, "operator");
    const name_id_t ni_term        = sfp->name_id_of(ni_seed, "Terminal");
    const name_id_t ni_tok_cat     = sfp->name_id_of(ni_seed, "TokenCategory");

    const name_id_t ni_expr_prefix   = sfp->name_id_of(ni_expr, "Prefix");
    const name_id_t ni_expr_postfix  = sfp->name_id_of(ni_expr, "Postfix");
    const name_id_t ni_expr_concat   = sfp->name_id_of(ni_expr, "Concat");
    const name_id_t ni_expr_or       = sfp->name_id_of(ni_expr, "Or");
    const name_id_t ni_expr_and      = sfp->name_id_of(ni_expr, "And");
    const name_id_t ni_expr_followup = sfp->name_id_of(ni_expr, "Followup");

    const name_id_t ni_nt   = sfp->name_id_of(ni_seed, "Nonterminal");
    const name_id_t ni_name = sfp->name_id_of(ni_seed, "Name");

    const name_id_t ni_axe         = sfp->name_id_of(ni_seed, "Axe");
    const name_id_t ni_axe_level   = sfp->name_id_of(ni_axe, "Level");
    const name_id_t ni_axe_assoc   = sfp->name_id_of(ni_axe, "Assoc");
    const name_id_t ni_axe_ops     = sfp->name_id_of(ni_axe, "Ops");
    const name_id_t ni_axe_op_type = sfp->name_id_of(ni_axe, "OpType");
    const name_id_t ni_axe_op      = sfp->name_id_of(ni_axe, "Op");
  };
  using lexicon_ptr_t = ptr_t<const lexicon_t>;
}
