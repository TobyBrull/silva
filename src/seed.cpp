#include "seed.hpp"

#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"

namespace silva {

  namespace impl {
    struct seed_nursery_t : public parse_tree_nursery_t {
      optional_t<token_id_t> tt_dash        = lookup_token("-");
      optional_t<token_id_t> tt_comma       = lookup_token(",");
      optional_t<token_id_t> tt_paren_open  = lookup_token("(");
      optional_t<token_id_t> tt_paren_close = lookup_token(")");
      optional_t<token_id_t> tt_brack_open  = lookup_token("{");
      optional_t<token_id_t> tt_brack_close = lookup_token("}");
      optional_t<token_id_t> tt_equal       = lookup_token("=");
      optional_t<token_id_t> tt_qmark       = lookup_token("?");
      optional_t<token_id_t> tt_star        = lookup_token("*");
      optional_t<token_id_t> tt_plus        = lookup_token("+");
      optional_t<token_id_t> tt_emark       = lookup_token("!");
      optional_t<token_id_t> tt_amper       = lookup_token("&");
      optional_t<token_id_t> tt_identifier  = lookup_token("identifier");
      optional_t<token_id_t> tt_operator    = lookup_token("operator");
      optional_t<token_id_t> tt_string      = lookup_token("string");
      optional_t<token_id_t> tt_number      = lookup_token("number");
      optional_t<token_id_t> tt_any         = lookup_token("any");

      seed_nursery_t(const_ptr_t<tokenization_t> tokenization)
        : parse_tree_nursery_t(std::move(tokenization),
                               const_ptr_unowned(seed_parse_root_primordial()))
      {
      }

      expected_t<parse_tree_sub_t> terminal()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(std::to_underlying(seed_rule_t::TERMINAL));

        SILVA_EXPECT_FMT(num_tokens_left() >= 1, "Expected string");
        SILVA_EXPECT_FMT(
            token_data()->category == token_category_t::STRING || token_id() == tt_identifier ||
                token_id() == tt_operator || token_id() == tt_string || token_id() == tt_number ||
                token_id() == tt_any,
            "Expected string or one of \"identifier\" \"operator\" \"string\" \"number\" \"any\"");
        token_index += 1;

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> nonterminal()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(std::to_underlying(seed_rule_t::NONTERMINAL));

        SILVA_EXPECT_FMT(num_tokens_left() >= 1 &&
                             token_data()->category == token_category_t::IDENTIFIER,
                         "Expected identifier");
        token_index += 1;

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> rule_precedence()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(std::to_underlying(seed_rule_t::RULE_PRECEDENCE));

        SILVA_EXPECT_FMT(num_tokens_left() >= 1 &&
                             token_data()->category == token_category_t::NUMBER,
                         "Expected number");
        token_index += 1;

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> primary()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};

        SILVA_EXPECT_FMT(num_tokens_left() >= 1, "Expected primary expression");

        if (token_id() == tt_paren_open) {
          token_index += 1;
          gg_rule.set_rule_index(std::to_underlying(seed_rule_t::PRIMARY_0));

          index_t num_atoms = 0;
          while (num_tokens_left() >= 1) {
            auto x = atom();
            if (!x) {
              break;
            }
            else {
              num_atoms += 1;
              gg_rule.sub += x.value();
            }
          }
          SILVA_EXPECT_FMT(num_atoms >= 1, "Expected at least one atom");

          SILVA_EXPECT_FMT(num_tokens_left() >= 1 && token_id() == tt_paren_close, "Expected ')'");
          token_index += 1;
        }
        else {
          if (auto result_1 = terminal(); result_1) {
            gg_rule.sub += result_1.value();
            gg_rule.set_rule_index(std::to_underlying(seed_rule_t::PRIMARY_1));
          }
          else if (auto result_2 = nonterminal(); result_2) {
            gg_rule.sub += result_2.value();
            gg_rule.set_rule_index(std::to_underlying(seed_rule_t::PRIMARY_2));
          }
          else {
            SILVA_UNEXPECTED_FMT("Could not parse primary expression");
          }
        }

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> suffix()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};

        SILVA_EXPECT_FMT(num_tokens_left() >= 1, "Expected operator");
        SILVA_EXPECT_FMT(token_id() == tt_qmark || token_id() == tt_star || token_id() == tt_plus ||
                             token_id() == tt_emark || token_id() == tt_amper,
                         "Expected one of \"?\" \"*\" \"+\" \"!\" \"&\"");
        gg_rule.set_rule_index(std::to_underlying(seed_rule_t::SUFFIX));
        token_index += 1;

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> atom()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(std::to_underlying(seed_rule_t::ATOM));

        gg_rule.sub += SILVA_TRY(primary());

        if (num_tokens_left() >= 1) {
          if (auto x = suffix(); x) {
            gg_rule.sub += x.value();
          }
        }

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> expr()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};

        if (num_tokens_left() >= 1 && token_id() == tt_brack_open) {
          token_index += 1;
          gg_rule.set_rule_index(std::to_underlying(seed_rule_t::EXPR_0));

          index_t terminal_count = 0;
          while (num_tokens_left() >= 1 && token_id() != tt_brack_close) {
            auto result = terminal();
            if (result) {
              gg_rule.sub += result.value();
              terminal_count += 1;
            }
            else {
              break;
            }
          }

          SILVA_EXPECT_FMT(terminal_count >= 1, "Could not parse any terminals");
          SILVA_EXPECT_FMT(num_tokens_left() >= 1 && token_id() == tt_brack_close, "Expected '}}'");
          token_index += 1;
        }
        else {
          gg_rule.set_rule_index(std::to_underlying(seed_rule_t::EXPR_1));

          while (num_tokens_left() >= 1) {
            auto x = atom();
            if (!x) {
              break;
            }
            else {
              gg_rule.sub += x.value();
            }
          }
        }

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> rule()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(std::to_underlying(seed_rule_t::RULE));

        gg_rule.sub += SILVA_TRY(nonterminal());

        if (num_tokens_left() >= 1 && token_id() == tt_comma) {
          token_index += 1;
          gg_rule.sub += SILVA_TRY(rule_precedence());
        }

        SILVA_EXPECT_FMT(num_tokens_left() >= 1 && token_id() == tt_equal, "Expected ',' or '='");
        token_index += 1;

        gg_rule.sub += SILVA_TRY(expr());

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> seed()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(std::to_underlying(seed_rule_t::SEED));

        while (num_tokens_left() >= 1) {
          SILVA_EXPECT_FMT(token_id() == tt_dash, "Expected '-'");
          token_index += 1;
          gg_rule.sub += SILVA_TRY(rule());
        }

        return gg_rule.release();
      }
    };
  }

  expected_t<parse_tree_t> seed_parse(const_ptr_t<tokenization_t> tokenization)
  {
    impl::seed_nursery_t seed_nursery(std::move(tokenization));
    SILVA_TRY(seed_nursery.seed());
    return {std::move(seed_nursery.retval)};
  }

  const parse_root_t* seed_parse_root()
  {
    static const parse_root_t retval = [&] {
      auto tokenization = SILVA_TRY_ASSERT(tokenize(const_ptr_unowned(&seed_seed_source_code)));
      auto fern_seed_pt = SILVA_TRY_ASSERT(seed_parse(to_unique_ptr(std::move(tokenization))));
      auto retval = SILVA_TRY_ASSERT(parse_root_t::create(to_unique_ptr(std::move(fern_seed_pt))));
      return std::move(retval);
    }();
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
            },
    };
    return &retval;
  }
}
