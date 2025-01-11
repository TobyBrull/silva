#include "seed.hpp"

#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

namespace silva {
  using enum token_category_t;
  using enum seed_rule_t;

  namespace impl {
    struct seed_parse_tree_nursery_t : public parse_tree_nursery_t {
      optional_t<token_id_t> tt_dash        = retval.tokenization->lookup_token("-");
      optional_t<token_id_t> tt_comma       = retval.tokenization->lookup_token(",");
      optional_t<token_id_t> tt_paren_open  = retval.tokenization->lookup_token("(");
      optional_t<token_id_t> tt_paren_close = retval.tokenization->lookup_token(")");
      optional_t<token_id_t> tt_eq          = retval.tokenization->lookup_token("=");
      optional_t<token_id_t> tt_eq_choice   = retval.tokenization->lookup_token("=~");
      optional_t<token_id_t> tt_eq_alias    = retval.tokenization->lookup_token("=>");
      optional_t<token_id_t> tt_eq_op_prec  = retval.tokenization->lookup_token("=%");
      optional_t<token_id_t> tt_brack_open  = retval.tokenization->lookup_token("[");
      optional_t<token_id_t> tt_brack_close = retval.tokenization->lookup_token("]");
      optional_t<token_id_t> tt_qmark       = retval.tokenization->lookup_token("?");
      optional_t<token_id_t> tt_star        = retval.tokenization->lookup_token("*");
      optional_t<token_id_t> tt_plus        = retval.tokenization->lookup_token("+");
      optional_t<token_id_t> tt_emark       = retval.tokenization->lookup_token("!");
      optional_t<token_id_t> tt_amper       = retval.tokenization->lookup_token("&");
      optional_t<token_id_t> tt_major_error = retval.tokenization->lookup_token("major_error");
      optional_t<token_id_t> tt_identifier  = retval.tokenization->lookup_token("identifier");
      optional_t<token_id_t> tt_id_regex    = retval.tokenization->lookup_token("identifier_regex");
      optional_t<token_id_t> tt_operator    = retval.tokenization->lookup_token("operator");
      optional_t<token_id_t> tt_string      = retval.tokenization->lookup_token("string");
      optional_t<token_id_t> tt_number      = retval.tokenization->lookup_token("number");
      optional_t<token_id_t> tt_any         = retval.tokenization->lookup_token("any");
      optional_t<token_id_t> tt_prefix      = retval.tokenization->lookup_token("prefix");
      optional_t<token_id_t> tt_postfix     = retval.tokenization->lookup_token("postfix");
      optional_t<token_id_t> tt_binary      = retval.tokenization->lookup_token("ltr");
      optional_t<token_id_t> tt_n_ary       = retval.tokenization->lookup_token("rtl");

      seed_parse_tree_nursery_t(const_ptr_t<tokenization_t> tokenization)
        : parse_tree_nursery_t(std::move(tokenization),
                               const_ptr_unowned(seed_parse_root_primordial()))
      {
      }

      expected_t<parse_tree_sub_t> op()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(OP));
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_data_by()->category == STRING,
                           "Expected string");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> op_type()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(OP_TYPE));
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 &&
                               (token_id_by() == tt_prefix || token_id_by() == tt_postfix ||
                                token_id_by() == tt_binary || token_id_by() == tt_n_ary),
                           "Expected \"prefix\" \"postfix\" \"ltr\" \"rtl\"");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> op_level()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(OP_LEVEL));
        gg_rule.sub += SILVA_EXPECT_FWD(op_type());
        index_t num_ops = 0;
        while (true) {
          if (auto result = op(); result) {
            num_ops += 1;
            gg_rule.sub += *std::move(result);
          }
          else {
            SILVA_EXPECT_FWD_IF(std::move(result), MAJOR);
            break;
          }
        }
        SILVA_EXPECT_PARSE(num_ops >= 1, "No Ops found");
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> op_list()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(OP_LIST));
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_brack_open,
                           "Expected '['");
        token_index += 1;
        error_nursery_t error_nursery;
        index_t num_levels = 0;
        while (true) {
          if (auto result = op_level(); result) {
            num_levels += 1;
            gg_rule.sub += *std::move(result);
          }
          else {
            error_nursery.add_child_error(SILVA_EXPECT_FWD_IF(std::move(result), MAJOR).error());
            break;
          }
        }
        if (num_levels == 0) {
          return std::unexpected(
              std::move(error_nursery)
                  .finish(error_level_t::MINOR, "{} No OpLevel found", token_position_by()));
        }
        if (num_tokens_left() == 0 || token_id_by() != tt_brack_close) {
          return std::unexpected(
              std::move(error_nursery)
                  .finish(error_level_t::MINOR, "{} Expected ']'", token_position_by()));
        }
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
        else if (token_id_by() == tt_eq_op_prec) {
          token_index += 1;
          gg_rule.set_rule_index(to_int(DERIVATION_3));
          gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
          gg_rule.sub += SILVA_EXPECT_FWD(op_list());
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

  expected_t<parse_tree_t> seed_parse(const_ptr_t<tokenization_t> tokenization)
  {
    expected_traits_t expected_traits{.materialize_fwd = true};
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
    static const tokenization_t tokenization =
        SILVA_EXPECT_ASSERT(tokenize(const_ptr_unowned(&seed_seed_source_code)));
    static const parse_tree_t parse_tree{.tokenization = const_ptr_unowned(&tokenization)};
    const auto make_rule = [&](const string_view_t nonterminal,
                               const index_t precedence) -> parse_root_t::rule_t {
      parse_root_t::rule_t retval;
      retval.token_id   = tokenization.lookup_token(nonterminal).value();
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
                make_rule("OpList", 0),
                make_rule("OpLevel", 0),
                make_rule("OpType", 0),
                make_rule("Op", 0),
                // clang-format on
            },
    };
    return &parse_root;
  }
}
