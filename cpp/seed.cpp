#include "seed.hpp"

#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

namespace silva {
  using enum token_category_t;
  using enum seed_rule_t;

  namespace impl {
    struct seed_parse_tree_nursery_t : public parse_tree_nursery_t {
      token_info_index_t tt_dash        = token_context_get_index("-");
      token_info_index_t tt_comma       = token_context_get_index(",");
      token_info_index_t tt_paren_open  = token_context_get_index("(");
      token_info_index_t tt_paren_close = token_context_get_index(")");
      token_info_index_t tt_eq          = token_context_get_index("=");
      token_info_index_t tt_eq_choice   = token_context_get_index("=~");
      token_info_index_t tt_eq_alias    = token_context_get_index("=>");
      token_info_index_t tt_eq_axe      = token_context_get_index("=%");
      token_info_index_t tt_brack_open  = token_context_get_index("[");
      token_info_index_t tt_brack_close = token_context_get_index("]");
      token_info_index_t tt_qmark       = token_context_get_index("?");
      token_info_index_t tt_star        = token_context_get_index("*");
      token_info_index_t tt_plus        = token_context_get_index("+");
      token_info_index_t tt_emark       = token_context_get_index("!");
      token_info_index_t tt_amper       = token_context_get_index("&");
      token_info_index_t tt_major_error = token_context_get_index("major_error");
      token_info_index_t tt_identifier  = token_context_get_index("identifier");
      token_info_index_t tt_id_regex    = token_context_get_index("identifier_regex");
      token_info_index_t tt_operator    = token_context_get_index("operator");
      token_info_index_t tt_string      = token_context_get_index("string");
      token_info_index_t tt_number      = token_context_get_index("number");
      token_info_index_t tt_any         = token_context_get_index("any");
      token_info_index_t tt_nest        = token_context_get_index("nest");
      token_info_index_t tt_ltr         = token_context_get_index("ltr");
      token_info_index_t tt_rtl         = token_context_get_index("rtl");
      token_info_index_t tt_flat        = token_context_get_index("flat");
      token_info_index_t tt_postfix     = token_context_get_index("postfix");
      token_info_index_t tt_postfix_n   = token_context_get_index("postfix_nest");
      token_info_index_t tt_infix       = token_context_get_index("infix");
      token_info_index_t tt_ternary     = token_context_get_index("ternary");
      token_info_index_t tt_prefix      = token_context_get_index("prefix");
      token_info_index_t tt_prefix_n    = token_context_get_index("prefix_nest");
      token_info_index_t tt_none        = token_context_get_index("none");

      seed_parse_tree_nursery_t(const tokenization_t* tokenization)
        : parse_tree_nursery_t(tokenization, const_ptr_unowned(seed_parse_root_primordial()))
      {
      }

      expected_t<parse_tree_sub_t> axe_op_type()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(AXE_OP_TYPE));
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
        gg_rule.set_rule_index(to_int(AXE_OP));
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 &&
                               (token_id_by() == tt_none || token_data_by()->category == STRING),
                           "Expected one of \"ltr\" \"rtl\" \"flat\"");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_ops()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(AXE_OPS));
        gg_rule.sub += SILVA_EXPECT_FWD(axe_op_type());
        while (auto result = axe_op()) {
          gg_rule.sub += *std::move(result);
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_assoc()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(AXE_ASSOC));
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
        gg_rule.set_rule_index(to_int(AXE_LEVEL));
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
        gg_rule.set_rule_index(to_int(AXE_NEST));
        SILVA_EXPECT_PARSE(num_tokens_left() >= 3 && token_id_by() == tt_nest &&
                               token_data_by(1)->category == STRING &&
                               token_data_by(2)->category == STRING,
                           "Expected [\"nest\" string string] sequence");
        token_index += 3;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_list()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(AXE_LIST));
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
        gg_rule.set_rule_index(to_int(REGEX));
        SILVA_EXPECT_PARSE(token_data_by()->category == STRING, "Expected string");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> terminal()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        if (num_tokens_left() >= 4 && token_id_by(0) == tt_id_regex &&
            token_id_by(1) == tt_paren_open) {
          gg_rule.set_rule_index(to_int(TERMINAL_0));
          token_index += 2;
          gg_rule.sub += SILVA_EXPECT_FWD(regex());
          SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_paren_close,
                             "Expected ')'");
          token_index += 1;
        }
        else {
          gg_rule.set_rule_index(to_int(TERMINAL_1));
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
        gg_rule.set_rule_index(to_int(NONTERMINAL));
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
        gg_rule.set_rule_index(to_int(seed_rule_t::RULE_PRECEDENCE));
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
          gg_rule.set_rule_index(to_int(PRIMARY_0));
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
            gg_rule.set_rule_index(to_int(PRIMARY_1));
          }
          else {
            SILVA_EXPECT_FWD_IF(std::move(result_1), MAJOR);
            if (auto result_2 = nonterminal(); result_2) {
              gg_rule.sub += *std::move(result_2);
              gg_rule.set_rule_index(to_int(PRIMARY_2));
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
        gg_rule.set_rule_index(to_int(SUFFIX));
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> atom()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "No tokens left when looking for Atom");
        if (token_id_by() == tt_major_error) {
          gg_rule.set_rule_index(to_int(ATOM_0));
          token_index += 1;
        }
        else {
          gg_rule.set_rule_index(to_int(ATOM_1));
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
          gg_rule.set_rule_index(to_int(DERIVATION_0));
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
          gg_rule.set_rule_index(to_int(DERIVATION_1));
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
          gg_rule.set_rule_index(to_int(DERIVATION_2));
          gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
          SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_comma, "Expected ','");
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(rule_precedence());
        }
        else if (token_id_by() == tt_eq_axe) {
          token_index += 1;
          gg_rule.set_rule_index(to_int(DERIVATION_3));
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
        gg_rule.set_rule_index(to_int(RULE));
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
        gg_rule.set_rule_index(to_int(SEED));
        while (num_tokens_left() >= 1) {
          SILVA_EXPECT_PARSE(token_id_by() == tt_dash, "Expected '-'");
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(rule());
        }
        return gg_rule.release();
      }
    };
  }

  expected_t<parse_tree_t> seed_parse(const tokenization_t* tokenization)
  {
    expected_traits_t expected_traits{.materialize_fwd = true};
    impl::seed_parse_tree_nursery_t nursery(std::move(tokenization));
    SILVA_EXPECT_FWD(nursery.seed());
    return {std::move(nursery.retval)};
  }

  namespace impl {
    const tokenization_t* seed_seed_tokenization()
    {
      static const tokenization_t* seed_seed_tokenization =
          SILVA_EXPECT_ASSERT(token_context_make("seed.seed", string_t{seed_seed}));
      return seed_seed_tokenization;
    }
  }

  const parse_root_t* seed_parse_root()
  {
    static const parse_root_t retval =
        SILVA_EXPECT_ASSERT(parse_root_t::create(impl::seed_seed_tokenization()));
    return &retval;
  }

  const parse_root_t* seed_parse_root_primordial()
  {
    static const parse_tree_t parse_tree{.tokenization = impl::seed_seed_tokenization()};
    const auto make_rule = [&](const string_view_t nonterminal,
                               const index_t precedence) -> parse_root_t::rule_t {
      parse_root_t::rule_t retval;
      retval.token_id   = token_context_get_index(nonterminal);
      retval.name       = nonterminal;
      retval.precedence = precedence;
      return retval;
    };
    static const parse_root_t parse_root{
        .seed_parse_tree = const_ptr_unowned(&parse_tree),
        .rules =
            {
                // clang-format off
                make_rule("Seed", 0),
                make_rule("Rule", 0),
                make_rule("RulePrecedence", 0),
                make_rule("Derivation", 0),
                make_rule("Derivation", 1),
                make_rule("Derivation", 2),
                make_rule("Derivation", 3),
                make_rule("Atom", 0),
                make_rule("Atom", 1),
                make_rule("Suffix", 0),
                make_rule("Primary", 0),
                make_rule("Primary", 1),
                make_rule("Primary", 2),
                make_rule("Nonterminal", 0),
                make_rule("Terminal", 0),
                make_rule("Terminal", 1),
                make_rule("Regex", 0),
                make_rule("AxeNest", 0),
                make_rule("AxeList", 0),
                make_rule("AxeLevel", 0),
                make_rule("AxeOpType", 0),
                make_rule("AxeAssoc", 0),
                make_rule("AxeOps", 0),
                make_rule("AxeOp", 0),
                // clang-format on
            },
    };
    return &parse_root;
  }
}
