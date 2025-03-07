#include "seed.hpp"

#include "canopy/expected.hpp"
#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

namespace silva {
  using enum token_category_t;

  parse_axe::parse_axe_t create_parse_axe_expr(token_context_ptr_t tcp)
  {
    using namespace parse_axe;
    using enum assoc_t;
    vector_t<parse_axe_level_desc_t> level_descs;
    level_descs.push_back(parse_axe_level_desc_t{
        .name  = tcp->full_name_id_of("Expr", "Parens"),
        .assoc = NEST,
        .opers = {atom_nest_t{tcp->token_id("("), tcp->token_id(")")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .name  = tcp->full_name_id_of("Expr", "Postfix"),
        .assoc = LEFT_TO_RIGHT,
        .opers =
            {
                postfix_t{tcp->token_id("?")},
                postfix_t{tcp->token_id("*")},
                postfix_t{tcp->token_id("+")},
                postfix_t{tcp->token_id("!")},
                postfix_t{tcp->token_id("&")},
            },
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .name  = tcp->full_name_id_of("Expr", "Concat"),
        .assoc = LEFT_TO_RIGHT,
        .opers = {infix_t{.token_id = token_id_none, .flatten = true}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .name  = tcp->full_name_id_of("Expr", "Alt"),
        .assoc = LEFT_TO_RIGHT,
        .opers = {infix_t{.token_id = tcp->token_id("|"), .flatten = true}},
    });
    auto retval = SILVA_EXPECT_ASSERT(parse_axe_create(tcp, level_descs));
    return retval;
  }

  namespace impl {
    struct seed_parse_tree_nursery_t : public parse_tree_nursery_t {
      token_id_t tt_dash        = tcp->token_id("-");
      token_id_t tt_equal       = tcp->token_id("=");
      token_id_t tt_axe         = tcp->token_id("=/");
      token_id_t tt_alias       = tcp->token_id("=>");
      token_id_t tt_paren_open  = tcp->token_id("(");
      token_id_t tt_paren_close = tcp->token_id(")");
      token_id_t tt_brack_open  = tcp->token_id("[");
      token_id_t tt_brack_close = tcp->token_id("]");
      token_id_t tt_identifier  = tcp->token_id("identifier");
      token_id_t tt_id_regex    = tcp->token_id("identifier_regex");
      token_id_t tt_operator    = tcp->token_id("operator");
      token_id_t tt_string      = tcp->token_id("string");
      token_id_t tt_number      = tcp->token_id("number");
      token_id_t tt_any         = tcp->token_id("any");
      token_id_t tt_nest        = tcp->token_id("nest");
      token_id_t tt_ltr         = tcp->token_id("ltr");
      token_id_t tt_rtl         = tcp->token_id("rtl");
      token_id_t tt_atom_nest   = tcp->token_id("atom_nest");
      token_id_t tt_postfix     = tcp->token_id("postfix");
      token_id_t tt_postfix_n   = tcp->token_id("postfix_nest");
      token_id_t tt_infix       = tcp->token_id("infix");
      token_id_t tt_infix_flat  = tcp->token_id("infix_flat");
      token_id_t tt_ternary     = tcp->token_id("ternary");
      token_id_t tt_prefix      = tcp->token_id("prefix");
      token_id_t tt_prefix_n    = tcp->token_id("prefix_nest");
      token_id_t tt_none        = tcp->token_id("none");

      full_name_id_t fni_seed        = tcp->full_name_id_of("Seed");
      full_name_id_t fni_rule        = tcp->full_name_id_of("Rule");
      full_name_id_t fni_expr        = tcp->full_name_id_of("Expr");
      full_name_id_t fni_atom        = tcp->full_name_id_of("Atom");
      full_name_id_t fni_alias       = tcp->full_name_id_of("Alias");
      full_name_id_t fni_axe         = tcp->full_name_id_of("Axe");
      full_name_id_t fni_axe_level   = tcp->full_name_id_of("AxeLevel");
      full_name_id_t fni_axe_assoc   = tcp->full_name_id_of("AxeAssoc");
      full_name_id_t fni_axe_ops     = tcp->full_name_id_of("AxeOps");
      full_name_id_t fni_axe_op_type = tcp->full_name_id_of("AxeOpType");
      full_name_id_t fni_axe_op      = tcp->full_name_id_of("AxeOp");
      full_name_id_t fni_nonterm     = tcp->full_name_id_of("Nonterminal");
      full_name_id_t fni_term        = tcp->full_name_id_of("Terminal");

      parse_axe::parse_axe_t seed_parse_axe;

      seed_parse_tree_nursery_t(shared_ptr_t<const tokenization_t> tokenization)
        : parse_tree_nursery_t(tokenization)
        , seed_parse_axe(create_parse_axe_expr(tokenization->context->ptr()))
      {
      }

      expected_t<parse_tree_sub_t> terminal()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_term);
        if (num_tokens_left() >= 4 && token_id_by(0) == tt_id_regex &&
            token_id_by(1) == tt_paren_open && token_data_by(2)->category == STRING &&
            token_id_by(3) == tt_paren_close) {
          token_index += 4;
        }
        else {
          SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "No more tokens when looking for Terminal");
          SILVA_EXPECT_PARSE(token_data_by()->category == STRING ||
                                 token_id_by() == tt_identifier || token_id_by() == tt_operator ||
                                 token_id_by() == tt_string || token_id_by() == tt_number ||
                                 token_id_by() == tt_any,
                             "Expected Terminal");
          token_index += 1;
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> nonterminal()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_nonterm);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_data_by()->category == IDENTIFIER &&
                               !token_data_by()->str.empty() &&
                               std::isupper(token_data_by()->str.front()),
                           "Expected Nonterminal");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_op()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_axe_op);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 &&
                               (token_id_by() == tt_none || token_data_by()->category == STRING),
                           "Expected 'none' or string");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_op_type()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_axe_op_type);
        SILVA_EXPECT_PARSE(
            num_tokens_left() >= 1 &&
                (token_id_by() == tt_atom_nest || token_id_by() == tt_prefix ||
                 token_id_by() == tt_prefix_n || token_id_by() == tt_infix ||
                 token_id_by() == tt_infix_flat || token_id_by() == tt_ternary ||
                 token_id_by() == tt_postfix || token_id_by() == tt_postfix_n),
            "Expected one of [ 'atom_nest' 'prefix' 'prefix_nest' 'infix' 'infix_flat' 'ternary' "
            "'postfix' 'postfix_nest' ]");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_ops()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_axe_ops);
        gg_rule.sub += SILVA_EXPECT_FWD(axe_op_type());
        while (auto result = axe_op()) {
          gg_rule.sub += *std::move(result);
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_assoc()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_axe_assoc);
        SILVA_EXPECT_PARSE(
            num_tokens_left() >= 1 &&
                (token_id_by() == tt_nest || token_id_by() == tt_ltr || token_id_by() == tt_rtl),
            "Expected one of [ \"nest\" \"ltr\" \"rtl\" ]");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe_level()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_axe_level);
        gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_equal, "Expected '='");
        token_index += 1;
        gg_rule.sub += SILVA_EXPECT_FWD(axe_assoc());
        while (auto result = axe_ops()) {
          gg_rule.sub += *std::move(result);
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> axe()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_axe);
        gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
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

      expected_t<parse_tree_sub_t> alias()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_alias);
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_brack_open,
                           "Expected '['");
        token_index += 1;
        while (num_tokens_left() >= 1 && token_id_by() != tt_brack_close) {
          gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
        }
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 && token_id_by() == tt_brack_close,
                           "Expected ']'");
        token_index += 1;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> atom()
      {
        auto gg                        = guard();
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;

        auto result_1 = nonterminal();
        if (result_1) {
          gg.sub += std::move(result_1).value();
          return gg.release();
        }
        error_nursery.add_child_error(std::move(result_1).error());

        auto result_2 = terminal();
        if (result_2) {
          gg.sub += std::move(result_2).value();
          return gg.release();
        }
        error_nursery.add_child_error(std::move(result_2).error());

        return std::unexpected(std::move(error_nursery)
                                   .finish_short(error_level_t::MINOR,
                                                 "{} Expected Atom",
                                                 token_position_at(orig_token_index)));
      }

      expected_t<parse_tree_sub_t> expr()
      {
        auto gg = guard();
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1, "No tokens left when parsing Derivation");
        using atom_delegate_t    = delegate_t<expected_t<parse_tree_sub_t>()>;
        const auto atom_delegate = atom_delegate_t::make<&seed_parse_tree_nursery_t::atom>(this);
        gg.sub += SILVA_EXPECT_FWD(seed_parse_axe.apply(*this, fni_atom, atom_delegate));
        return gg.release();
      }

      expected_t<parse_tree_sub_t> rule()
      {
        auto gg_rule = guard_for_rule();
        gg_rule.set_rule_name(fni_rule);
        gg_rule.sub += SILVA_EXPECT_FWD(nonterminal());
        const token_id_t op_ti = token_id_by();
        SILVA_EXPECT_PARSE(num_tokens_left() >= 1 &&
                               (op_ti == tt_equal || op_ti == tt_axe || op_ti == tt_alias),
                           "Expected one of [ '=' '=/' '=>' ]");
        token_index += 1;
        if (op_ti == tt_equal) {
          gg_rule.sub += SILVA_EXPECT_FWD(expr());
        }
        else if (op_ti == tt_axe) {
          gg_rule.sub += SILVA_EXPECT_FWD(axe());
        }
        else {
          gg_rule.sub += SILVA_EXPECT_FWD(alias());
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> seed()
      {
        auto gg_rule = guard_for_rule();
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
