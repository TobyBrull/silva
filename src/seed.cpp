#include "seed.hpp"

#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"

namespace silva {
  using enum token_category_t;
  using enum seed_rule_t;

  namespace impl {
    struct seed_parse_tree_nursery_t : public parse_tree_nursery_t {
      optional_t<token_id_t> tt_dash        = retval.tokenization->lookup_token("-");
      optional_t<token_id_t> tt_comma       = retval.tokenization->lookup_token(",");
      optional_t<token_id_t> tt_paren_open  = retval.tokenization->lookup_token("(");
      optional_t<token_id_t> tt_paren_close = retval.tokenization->lookup_token(")");
      optional_t<token_id_t> tt_brack_open  = retval.tokenization->lookup_token("{");
      optional_t<token_id_t> tt_brack_close = retval.tokenization->lookup_token("}");
      optional_t<token_id_t> tt_equal       = retval.tokenization->lookup_token("=");
      optional_t<token_id_t> tt_qmark       = retval.tokenization->lookup_token("?");
      optional_t<token_id_t> tt_star        = retval.tokenization->lookup_token("*");
      optional_t<token_id_t> tt_plus        = retval.tokenization->lookup_token("+");
      optional_t<token_id_t> tt_emark       = retval.tokenization->lookup_token("!");
      optional_t<token_id_t> tt_amper       = retval.tokenization->lookup_token("&");
      optional_t<token_id_t> tt_identifier  = retval.tokenization->lookup_token("identifier");
      optional_t<token_id_t> tt_id_regex    = retval.tokenization->lookup_token("identifier_regex");
      optional_t<token_id_t> tt_operator    = retval.tokenization->lookup_token("operator");
      optional_t<token_id_t> tt_string      = retval.tokenization->lookup_token("string");
      optional_t<token_id_t> tt_number      = retval.tokenization->lookup_token("number");
      optional_t<token_id_t> tt_any         = retval.tokenization->lookup_token("any");

      seed_parse_tree_nursery_t(const_ptr_t<tokenization_t> tokenization)
        : parse_tree_nursery_t(std::move(tokenization),
                               const_ptr_unowned(seed_parse_root_primordial()))
      {
      }

      expected_t<parse_tree_sub_t> regex()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(REGEX));
        SILVA_EXPECT(token_data()->category == STRING, MINOR);
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> terminal()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        if (num_tokens_left() >= 4 && token_id(0) == tt_id_regex && token_id(1) == tt_paren_open) {
          gg_rule.set_rule_index(to_int(TERMINAL_0));
          token_index += 2;
          gg_rule.sub += SILVA_EXPECT_FWD(regex());
          SILVA_EXPECT(num_tokens_left() >= 1 && token_id() == tt_paren_close, MINOR);
          token_index += 1;
        }
        else {
          gg_rule.set_rule_index(to_int(TERMINAL_1));
          SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "Expected string");
          SILVA_EXPECT(token_data()->category == STRING || token_id() == tt_identifier ||
                           token_id() == tt_operator || token_id() == tt_string ||
                           token_id() == tt_number || token_id() == tt_any,
                       MINOR,
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
        SILVA_EXPECT(num_tokens_left() >= 1 && token_data()->category == IDENTIFIER &&
                         !token_data()->str.empty() && std::isupper(token_data()->str.front()),
                     MINOR,
                     "Expected nonterminal");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> rule_precedence()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(seed_rule_t::RULE_PRECEDENCE));
        SILVA_EXPECT(num_tokens_left() >= 1 && token_data()->category == NUMBER,
                     MINOR,
                     "Expected number");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> primary()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "Expected primary expression");
        if (token_id() == tt_paren_open) {
          token_index += 1;
          gg_rule.set_rule_index(to_int(PRIMARY_0));
          index_t num_atoms = 0;
          while (num_tokens_left() >= 1) {
            if (auto result = atom(); result) {
              num_atoms += 1;
              gg_rule.sub += *result;
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
              break;
            }
          }
          SILVA_EXPECT(num_atoms >= 1, MINOR, "Expected at least one atom");
          SILVA_EXPECT(num_tokens_left() >= 1 && token_id() == tt_paren_close,
                       MINOR,
                       "Expected ')'");
          token_index += 1;
        }
        else {
          if (auto result_1 = terminal(); result_1) {
            gg_rule.sub += *result_1;
            gg_rule.set_rule_index(to_int(PRIMARY_1));
          }
          else {
            SILVA_EXPECT_FWD_IF(std::move(result_1), MAJOR);
            if (auto result_2 = nonterminal(); result_2) {
              gg_rule.sub += *result_2;
              gg_rule.set_rule_index(to_int(PRIMARY_2));
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result_2), MAJOR);
              SILVA_EXPECT(false, MINOR, "Could not parse primary expression");
            }
          }
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> suffix()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "Expected operator");
        SILVA_EXPECT(token_id() == tt_qmark || token_id() == tt_star || token_id() == tt_plus ||
                         token_id() == tt_emark || token_id() == tt_amper,
                     MINOR,
                     "Expected one of \"?\" \"*\" \"+\" \"!\" \"&\"");
        gg_rule.set_rule_index(to_int(SUFFIX));
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> atom()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(ATOM));
        gg_rule.sub += SILVA_EXPECT_FWD(primary());
        if (num_tokens_left() >= 1) {
          if (auto result = suffix(); result) {
            gg_rule.sub += *result;
          }
          else {
            SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
          }
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> expr()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        if (num_tokens_left() >= 1 && token_id() == tt_brack_open) {
          token_index += 1;
          gg_rule.set_rule_index(to_int(EXPR_0));
          index_t terminal_count = 0;
          while (num_tokens_left() >= 1 && token_id() != tt_brack_close) {
            if (auto result = terminal(); result) {
              gg_rule.sub += *result;
              terminal_count += 1;
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
              break;
            }
          }
          SILVA_EXPECT(terminal_count >= 1, MINOR, "Could not parse any terminals");
          SILVA_EXPECT(num_tokens_left() >= 1 && token_id() == tt_brack_close,
                       MINOR,
                       "Expected '}}'");
          token_index += 1;
        }
        else {
          gg_rule.set_rule_index(to_int(EXPR_1));
          while (num_tokens_left() >= 1) {
            if (auto result = atom(); result) {
              gg_rule.sub += *result;
            }
            else {
              SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
              break;
            }
          }
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> rule()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(RULE));
        gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
        if (num_tokens_left() >= 1 && token_id() == tt_comma) {
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(rule_precedence());
        }
        SILVA_EXPECT(num_tokens_left() >= 1 && token_id() == tt_equal,
                     MINOR,
                     "Expected ',' or '='");
        token_index += 1;
        gg_rule.sub += SILVA_EXPECT_FWD(expr());
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> seed()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(SEED));
        while (num_tokens_left() >= 1) {
          SILVA_EXPECT(token_id() == tt_dash, MINOR, "Expected '-'");
          token_index += 1;
          gg_rule.sub += SILVA_EXPECT_FWD(rule());
        }
        return gg_rule.release();
      }
    };
  }

  expected_t<parse_tree_t> seed_parse(const_ptr_t<tokenization_t> tokenization)
  {
    impl::seed_parse_tree_nursery_t nursery(std::move(tokenization));
    SILVA_EXPECT_FWD(nursery.seed());
    return {std::move(nursery.retval)};
  }

  const parse_root_t* seed_parse_root()
  {
    static const parse_root_t retval =
        SILVA_EXPECT_ASSERT(parse_root_t::create(const_ptr_unowned(&seed_seed_source_code)));
    return &retval;
  }

  const parse_root_t* seed_parse_root_primordial()
  {
    static const parse_root_t retval{
        .rules =
            {
                parse_root_t::rule_t{.name = "Seed", .precedence = 0},
                parse_root_t::rule_t{.name = "Rule", .precedence = 0},
                parse_root_t::rule_t{.name = "RulePrecedence", .precedence = 0},
                parse_root_t::rule_t{.name = "Expr", .precedence = 0},
                parse_root_t::rule_t{.name = "Expr", .precedence = 1},
                parse_root_t::rule_t{.name = "Atom", .precedence = 0},
                parse_root_t::rule_t{.name = "Suffix", .precedence = 0},
                parse_root_t::rule_t{.name = "Primary", .precedence = 0},
                parse_root_t::rule_t{.name = "Primary", .precedence = 1},
                parse_root_t::rule_t{.name = "Primary", .precedence = 2},
                parse_root_t::rule_t{.name = "Nonterminal", .precedence = 0},
                parse_root_t::rule_t{.name = "Terminal", .precedence = 0},
                parse_root_t::rule_t{.name = "Terminal", .precedence = 1},
                parse_root_t::rule_t{.name = "Regex", .precedence = 0},
            },
    };
    return &retval;
  }
}
