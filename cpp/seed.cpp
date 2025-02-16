#include "seed.hpp"

#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

namespace silva {
  using enum token_category_t;

  namespace impl {
    struct seed_parse_tree_nursery_t : public parse_tree_nursery_t {
      token_id_t tt_dash        = tcp->token_id("-");
      token_id_t tt_comma       = tcp->token_id(",");
      token_id_t tt_paren_open  = tcp->token_id("(");
      token_id_t tt_paren_close = tcp->token_id(")");
      token_id_t tt_eq          = tcp->token_id("=");
      token_id_t tt_eq_choice   = tcp->token_id("=~");
      token_id_t tt_eq_alias    = tcp->token_id("=>");
      token_id_t tt_eq_axe      = tcp->token_id("=%");
      token_id_t tt_brack_open  = tcp->token_id("[");
      token_id_t tt_brack_close = tcp->token_id("]");
      token_id_t tt_qmark       = tcp->token_id("?");
      token_id_t tt_star        = tcp->token_id("*");
      token_id_t tt_plus        = tcp->token_id("+");
      token_id_t tt_emark       = tcp->token_id("!");
      token_id_t tt_amper       = tcp->token_id("&");
      token_id_t tt_major_error = tcp->token_id("major_error");
      token_id_t tt_identifier  = tcp->token_id("identifier");
      token_id_t tt_id_regex    = tcp->token_id("identifier_regex");
      token_id_t tt_operator    = tcp->token_id("operator");
      token_id_t tt_string      = tcp->token_id("string");
      token_id_t tt_number      = tcp->token_id("number");
      token_id_t tt_any         = tcp->token_id("any");
      token_id_t tt_nest        = tcp->token_id("primary_nest");
      token_id_t tt_ltr         = tcp->token_id("ltr");
      token_id_t tt_rtl         = tcp->token_id("rtl");
      token_id_t tt_flat        = tcp->token_id("flat");
      token_id_t tt_postfix     = tcp->token_id("postfix");
      token_id_t tt_postfix_n   = tcp->token_id("postfix_nest");
      token_id_t tt_infix       = tcp->token_id("infix");
      token_id_t tt_ternary     = tcp->token_id("ternary");
      token_id_t tt_prefix      = tcp->token_id("prefix");
      token_id_t tt_prefix_n    = tcp->token_id("prefix_nest");
      token_id_t tt_none        = tcp->token_id("none");

      full_name_id_t fni_seed        = tcp->full_name_id_of("Seed", "0");
      full_name_id_t fni_rule        = tcp->full_name_id_of("Rule", "0");
      full_name_id_t fni_rule_prec   = tcp->full_name_id_of("RulePrecedence", "0");
      full_name_id_t fni_deriv_0     = tcp->full_name_id_of("Derivation", "0");
      full_name_id_t fni_deriv_1     = tcp->full_name_id_of("Derivation", "1");
      full_name_id_t fni_deriv_2     = tcp->full_name_id_of("Derivation", "2");
      full_name_id_t fni_deriv_3     = tcp->full_name_id_of("Derivation", "3");
      full_name_id_t fni_regex       = tcp->full_name_id_of("Regex", "0");
      full_name_id_t fni_nonterm     = tcp->full_name_id_of("Nonterminal", "0");
      full_name_id_t fni_term_0      = tcp->full_name_id_of("Terminal", "0");
      full_name_id_t fni_term_1      = tcp->full_name_id_of("Terminal", "1");
      full_name_id_t fni_prim_0      = tcp->full_name_id_of("Primary", "0");
      full_name_id_t fni_prim_1      = tcp->full_name_id_of("Primary", "1");
      full_name_id_t fni_prim_2      = tcp->full_name_id_of("Primary", "2");
      full_name_id_t fni_atom_0      = tcp->full_name_id_of("Atom", "0");
      full_name_id_t fni_atom_1      = tcp->full_name_id_of("Atom", "1");
      full_name_id_t fni_suffix      = tcp->full_name_id_of("Suffix", "0");
      full_name_id_t fni_axe_nest    = tcp->full_name_id_of("AxeNest", "0");
      full_name_id_t fni_axe_list    = tcp->full_name_id_of("AxeList", "0");
      full_name_id_t fni_axe_level   = tcp->full_name_id_of("AxeLevel", "0");
      full_name_id_t fni_axe_op_type = tcp->full_name_id_of("AxeOpType", "0");
      full_name_id_t fni_axe_assoc   = tcp->full_name_id_of("AxeAssoc", "0");
      full_name_id_t fni_axe_ops     = tcp->full_name_id_of("AxeOps", "0");
      full_name_id_t fni_axe_op      = tcp->full_name_id_of("AxeOp", "0");

      seed_parse_tree_nursery_t(shared_ptr_t<const tokenization_t> tokenization)
        : parse_tree_nursery_t(tokenization)
      {
      }

      expected_t<parse_tree_sub_t> axe_op_type()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_axe_op_type);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 &&
                               (token_id_by() == tt_prefix || token_id_by() == tt_prefix_n ||
                                token_id_by() == tt_infix || token_id_by() == tt_ternary ||
                                token_id_by() == tt_postfix || token_id_by() == tt_postfix_n),
                           "Expected one of \"prefix\" \"prefix_nest\" \"infix\" \"ternary\" "
                           "\"postfix\" \"postfix_nest\"");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_op()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_axe_op);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 &&
                               (token_id_by() == tt_none || token_data_by()->category == STRING),
                           "Expected one of \"ltr\" \"rtl\" \"flat\"");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_ops()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_axe_ops);
        gg_rule.sub += SILVA_EXPECT_FWD(axe_op_type());
        while (auto result = axe_op()) {
          gg_rule.sub += *std::move(result);
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_assoc()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_axe_assoc);
        SILVA_EXPECT_PARSE(
            num_tokens_left() >= 1 &&
                (token_id_by() == tt_ltr || token_id_by() == tt_rtl || token_id_by() == tt_flat),
            "Expected one of \"ltr\" \"rtl\" \"flat\"");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_level()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_axe_level);
        gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_eq, "Expected '='");
        token_index += 1;
        gg_rule.sub += SILVA_EXPECT_FWD(axe_assoc());
        while (auto result = axe_ops()) {
          gg_rule.sub += *std::move(result);
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_nest()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_axe_nest);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 3 && token_id_by() == tt_nest &&
                               token_data_by(1)->category == STRING &&
                               token_data_by(2)->category == STRING,
                           "Expected [\"primary_nest\" string string] sequence");
        token_index += 3;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_list()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_axe_list);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_brack_open,
                           "Expected '['");
        token_index += 1;
        while (num_tokens_left() >= 1 && token_id_by() == tt_dash) {
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(axe_level());
        }
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_brack_close,
                           "Expected ']'");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> regex()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_regex);
        SILVA_EXPECT_PARSE(token_data_by()->category == STRING, "Expected string");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> terminal()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        if (num_tokens_left() >= 4 && token_id_by(0) == tt_id_regex &&
            token_id_by(1) == tt_paren_open) {
          gg_rule.set_rule_name(fni_term_0);
          token_index += 2;
          gg_rule.sub += SILVA_EXPECT_FWD(regex());
          SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_paren_close,
                             "Expected ')'");
          token_index += 1;
        }
        else {
          gg_rule.set_rule_name(fni_term_1);
          SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "No more tokens when looking for Terminal,1");
          SILVA_EXPECT_PARSE(token_data_by()->category == STRING ||
                                 token_id_by() == tt_identifier || token_id_by() == tt_operator ||
                                 token_id_by() == tt_string || token_id_by() == tt_number ||
                                 token_id_by() == tt_any,
                             "Expected string or one of \"identifier\" \"operator\" \"string\" "
                             "\"number\" \"any\"");
          token_index += 1;
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> nonterminal()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_nonterm);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_data_by()->category == IDENTIFIER &&
                               !token_data_by()->str.empty() &&
                               std::isupper(token_data_by()->str.front()),
                           "Expected nonterminal");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> rule_precedence()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_rule_prec);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_data_by()->category == NUMBER,
                           "Expected number");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> primary()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "Expected primary expression");
        if (token_id_by() == tt_paren_open) {
          token_index += 1;
          gg_rule.set_rule_name(fni_prim_0);
          index_t num_atoms = 0;
          while (num_tokens_left() >= 1) {
            if (auto result = atom(); result) {
              num_atoms += 1;
              gg_rule.sub += *std::move(result);
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
              break;
            }
          }
          SILVA_EXPECT_PARSE(num_atoms >= 1, "Expected at least one atom");
          SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_paren_close,
                             "Expected ')'");
          token_index += 1;
        }
        else {
          if (auto result_1 = terminal(); result_1) {
            gg_rule.sub += *std::move(result_1);
            gg_rule.set_rule_name(fni_prim_1);
          }
          else {
            SILVA_EXPECT_FWD_IF(std::move(result_1), MAJOR);
            if (auto result_2 = nonterminal(); result_2) {
              gg_rule.sub += *std::move(result_2);
              gg_rule.set_rule_name(fni_prim_2);
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result_2), MAJOR);
              SILVA_EXPECT_PARSE(false, "Could not parse primary expression");
            }
          }
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> suffix()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "No tokens left when operator expected");
        SILVA_EXPECT_PARSE(token_id_by() == tt_qmark || token_id_by() == tt_star ||
                               token_id_by() == tt_plus || token_id_by() == tt_emark ||
                               token_id_by() == tt_amper,
                           "Expected one of \"?\" \"*\" \"+\" \"!\" \"&\"");
        gg_rule.set_rule_name(fni_suffix);
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> atom()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "No tokens left when looking for Atom");
        if (token_id_by() == tt_major_error) {
          gg_rule.set_rule_name(fni_atom_0);
          token_index += 1;
        }
        else {
          gg_rule.set_rule_name(fni_atom_1);
          gg_rule.sub += SILVA_EXPECT_FWD(primary());
          if (num_tokens_left() >= 1) {
            if (auto result = suffix(); result) {
              gg_rule.sub += *std::move(result);
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
            }
          }
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> derivation()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "No tokens left when parsing Derivation");
        if (token_id_by() == tt_eq) {
          token_index += 1;
          gg_rule.set_rule_name(fni_deriv_0);
          index_t atom_count = 0;
          while (true) {
            if (auto result = atom(); result) {
              gg_rule.sub += *std::move(result);
              atom_count += 1;
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
              break;
            }
          }
          SILVA_EXPECT_PARSE(atom_count >= 1, "Could not parse at least one Atom");
        }
        else if (token_id_by() == tt_eq_choice) {
          token_index += 1;
          gg_rule.set_rule_name(fni_deriv_1);
          index_t terminal_count = 0;
          while (true) {
            if (auto result = terminal(); result) {
              gg_rule.sub += *std::move(result);
              terminal_count += 1;
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
              break;
            }
          }
          SILVA_EXPECT_PARSE(terminal_count >= 1, "Could not parse at least one Terminal");
        }
        else if (token_id_by() == tt_eq_alias) {
          token_index += 1;
          gg_rule.set_rule_name(fni_deriv_2);
          gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
          SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_comma, "Expected ','");
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(rule_precedence());
        }
        else if (token_id_by() == tt_eq_axe) {
          token_index += 1;
          gg_rule.set_rule_name(fni_deriv_3);
          gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
          if (auto result = axe_nest(); result) {
            gg_rule.sub += *std::move(result);
          }
          gg_rule.sub += SILVA_EXPECT_FWD(axe_list());
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> rule()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_rule);
        gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
        if (num_tokens_left() >= 1 && token_id_by() == tt_comma) {
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(rule_precedence());
        }
        gg_rule.sub += SILVA_EXPECT_FWD(derivation());
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> seed()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_name(fni_seed);
        while (num_tokens_left() >= 1) {
          SILVA_EXPECT_PARSE(token_id_by() == tt_dash, "Expected '-'");
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(rule());
        }
        return gg_rule.release();
      }
    };
  }

  expected_t<unique_ptr_t<parse_tree_t>> seed_parse(shared_ptr_t<const tokenization_t> tokenization)
  {
    expected_traits_t expected_traits{.materialize_fwd = true};
    impl::seed_parse_tree_nursery_t nursery(std::move(tokenization));
    SILVA_EXPECT_FWD(nursery.seed());
    return {std::make_unique<parse_tree_t>(std::move(nursery.retval))};
  }

  unique_ptr_t<parse_root_t> seed_parse_root(token_context_ptr_t tcp)
  {
    return SILVA_EXPECT_ASSERT(parse_root_t::create(tcp, "seed.seed", string_t{seed_seed}));
  }
}
