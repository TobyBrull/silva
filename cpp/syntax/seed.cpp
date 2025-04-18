#include "seed.hpp"

#include "canopy/expected.hpp"
#include "parse_tree_nursery.hpp"
#include "seed_engine.hpp"
#include "tokenization.hpp"

namespace silva {
  using enum token_category_t;

  parse_axe::parse_axe_t create_parse_axe_expr(token_catalog_ptr_t tcp)
  {
    using namespace parse_axe;
    using enum assoc_t;
    vector_t<parse_axe_level_desc_t> level_descs;
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tcp->token_id("Parens"),
        .assoc     = NEST,
        .opers     = {atom_nest_t{*tcp->token_id("("), *tcp->token_id(")")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tcp->token_id("Prefix"),
        .assoc     = RIGHT_TO_LEFT,
        .opers =
            {
                prefix_t{*tcp->token_id("not")},
            },
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tcp->token_id("Postfix"),
        .assoc     = LEFT_TO_RIGHT,
        .opers =
            {
                postfix_t{*tcp->token_id("?")},
                postfix_t{*tcp->token_id("*")},
                postfix_t{*tcp->token_id("+")},
            },
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tcp->token_id("Concat"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{
                .token_id = *tcp->token_id("concat"),
                .concat   = true,
                .flatten  = true,
        }},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tcp->token_id("And"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{.token_id = *tcp->token_id("but_then"), .flatten = true}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tcp->token_id("Or"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{.token_id = *tcp->token_id("|"), .flatten = true}},
    });
    const name_id_t fni_expr = tcp->name_id_of("Seed", "Expr");
    auto retval              = SILVA_EXPECT_ASSERT(parse_axe_create(tcp, fni_expr, level_descs));
    return retval;
  }

  namespace impl {
    struct seed_parse_tree_nursery_t : public parse_tree_nursery_t {
      token_id_t tt_dot         = *tcp->token_id(".");
      token_id_t tt_dash        = *tcp->token_id("-");
      token_id_t tt_equal       = *tcp->token_id("=");
      token_id_t tt_axe         = *tcp->token_id("=/");
      token_id_t tt_alias       = *tcp->token_id("=>");
      token_id_t tt_brack_open  = *tcp->token_id("[");
      token_id_t tt_brack_close = *tcp->token_id("]");
      token_id_t tt_identifier  = *tcp->token_id("identifier");
      token_id_t tt_regex       = *tcp->token_id("/");
      token_id_t tt_up          = *tcp->token_id("p");
      token_id_t tt_silva       = *tcp->token_id("_");
      token_id_t tt_here        = *tcp->token_id("x");
      token_id_t tt_operator    = *tcp->token_id("operator");
      token_id_t tt_string      = *tcp->token_id("string");
      token_id_t tt_number      = *tcp->token_id("number");
      token_id_t tt_any         = *tcp->token_id("any");
      token_id_t tt_eof         = *tcp->token_id("end_of_file");
      token_id_t tt_nest        = *tcp->token_id("nest");
      token_id_t tt_ltr         = *tcp->token_id("ltr");
      token_id_t tt_rtl         = *tcp->token_id("rtl");
      token_id_t tt_atom_nest   = *tcp->token_id("atom_nest");
      token_id_t tt_postfix     = *tcp->token_id("postfix");
      token_id_t tt_postfix_n   = *tcp->token_id("postfix_nest");
      token_id_t tt_infix       = *tcp->token_id("infix");
      token_id_t tt_infix_flat  = *tcp->token_id("infix_flat");
      token_id_t tt_ternary     = *tcp->token_id("ternary");
      token_id_t tt_prefix      = *tcp->token_id("prefix");
      token_id_t tt_prefix_n    = *tcp->token_id("prefix_nest");
      token_id_t tt_concat      = *tcp->token_id("concat");
      token_id_t tt_keywords_of = *tcp->token_id("keywords_of");

      name_id_t fni_seed        = tcp->name_id_of("Seed");
      name_id_t fni_rule        = tcp->name_id_of(fni_seed, "Rule");
      name_id_t fni_expr_or_a   = tcp->name_id_of(fni_seed, "ExprOrAlias");
      name_id_t fni_expr        = tcp->name_id_of(fni_seed, "Expr");
      name_id_t fni_atom        = tcp->name_id_of(fni_seed, "Atom");
      name_id_t fni_axe         = tcp->name_id_of(fni_seed, "Axe");
      name_id_t fni_axe_level   = tcp->name_id_of(fni_axe, "Level");
      name_id_t fni_axe_assoc   = tcp->name_id_of(fni_axe, "Assoc");
      name_id_t fni_axe_ops     = tcp->name_id_of(fni_axe, "Ops");
      name_id_t fni_axe_op_type = tcp->name_id_of(fni_axe, "OpType");
      name_id_t fni_axe_op      = tcp->name_id_of(fni_axe, "Op");
      name_id_t fni_nt          = tcp->name_id_of(fni_seed, "Nonterminal");
      name_id_t fni_nt_base     = tcp->name_id_of(fni_nt, "Base");
      name_id_t fni_term        = tcp->name_id_of(fni_seed, "Terminal");

      parse_axe::parse_axe_t seed_parse_axe;

      seed_parse_tree_nursery_t(shared_ptr_t<const tokenization_t> tokenization)
        : parse_tree_nursery_t(tokenization)
        , seed_parse_axe(create_parse_axe_expr(tokenization->context->ptr()))
      {
      }

      expected_t<parse_tree_node_t> terminal()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_term);
        if (num_tokens_left() >= 3 &&
            (token_id_by(0) == tt_identifier || token_id_by(0) == tt_operator) &&
            token_id_by(1) == tt_regex && token_data_by(2)->category == STRING) {
          token_index += 3;
        }
        else if (num_tokens_left() >= 2 && token_id_by(0) == tt_keywords_of) {
          token_index += 1;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_term, nonterminal()));
        }
        else {
          SILVA_EXPECT_PARSE(fni_term,
                             num_tokens_left() >= 1,
                             "No more tokens when looking for Terminal");
          SILVA_EXPECT_PARSE(fni_term,
                             token_data_by()->category == STRING ||
                                 token_id_by() == tt_identifier || token_id_by() == tt_operator ||
                                 token_id_by() == tt_string || token_id_by() == tt_number ||
                                 token_id_by() == tt_any || token_id_by() == tt_eof,
                             "Expected Terminal");
          token_index += 1;
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> nonterminal_base()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_nt_base);
        SILVA_EXPECT_PARSE(fni_nt_base, num_tokens_left() >= 1, "Expected Nonterminal.Base");
        const bool is_cap_id = token_data_by()->category == IDENTIFIER &&
            !token_data_by()->str.empty() && std::isupper(token_data_by()->str.front());
        const bool is_name_id =
            token_id_by() == tt_up || token_id_by() == tt_here || token_id_by() == tt_silva;
        SILVA_EXPECT_PARSE(fni_nt_base, is_cap_id || is_name_id, "Expected Nonterminal.Base");
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> nonterminal()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_nt);
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_nt, nonterminal_base()));
        while (num_tokens_left() >= 2 && token_id_by() == tt_dot) {
          token_index += 1;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_nt, nonterminal_base()));
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_op()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_axe_op);
        SILVA_EXPECT_PARSE(fni_axe_op,
                           num_tokens_left() >= 1 &&
                               (token_id_by() == tt_concat || token_data_by()->category == STRING),
                           "Expected 'concat' or string");
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_op_type()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_axe_op_type);
        SILVA_EXPECT_PARSE(
            fni_axe_op_type,
            num_tokens_left() >= 1 &&
                (token_id_by() == tt_atom_nest || token_id_by() == tt_prefix ||
                 token_id_by() == tt_prefix_n || token_id_by() == tt_infix ||
                 token_id_by() == tt_infix_flat || token_id_by() == tt_ternary ||
                 token_id_by() == tt_postfix || token_id_by() == tt_postfix_n),
            "Expected one of [ 'atom_nest' 'prefix' 'prefix_nest' 'infix' 'infix_flat' 'ternary' "
            "'postfix' 'postfix_nest' ]");
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_ops()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_axe_ops);
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_axe_ops, axe_op_type()));
        while (auto result = axe_op()) {
          ss_rule.add_proto_node(*result);
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_assoc()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_axe_assoc);
        SILVA_EXPECT_PARSE(
            fni_axe_assoc,
            num_tokens_left() >= 1 &&
                (token_id_by() == tt_nest || token_id_by() == tt_ltr || token_id_by() == tt_rtl),
            "Expected one of [ 'nest' 'ltr' 'rtl' ]");
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_level()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_axe_level);
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_axe_level, nonterminal_base()));
        SILVA_EXPECT_PARSE(fni_axe_level,
                           num_tokens_left() >= 1 && token_id_by() == tt_equal,
                           "Expected '='");
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_axe_level, axe_assoc()));
        while (auto result = axe_ops()) {
          ss_rule.add_proto_node(*result);
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_axe);
        SILVA_EXPECT_PARSE(fni_axe,
                           num_tokens_left() >= 1 && token_id_by() == tt_axe,
                           "Expected '['");
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_axe, nonterminal()));
        SILVA_EXPECT_PARSE(fni_axe,
                           num_tokens_left() >= 1 && token_id_by() == tt_brack_open,
                           "Expected '['");
        token_index += 1;
        while (num_tokens_left() >= 1 && token_id_by() == tt_dash) {
          token_index += 1;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_axe, axe_level()));
        }
        SILVA_EXPECT_PARSE(fni_axe,
                           num_tokens_left() >= 1 && token_id_by() == tt_brack_close,
                           "Expected ']'");
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> atom()
      {
        auto ss                        = stake();
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;

        auto result_1 = nonterminal();
        if (result_1) {
          ss.add_proto_node(*result_1);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result_1).error());

        auto result_2 = terminal();
        if (result_2) {
          ss.add_proto_node(*result_2);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result_2).error());

        return std::unexpected(std::move(error_nursery)
                                   .finish_short(error_level_t::MINOR,
                                                 "[{}] {}",
                                                 token_position_at(orig_token_index),
                                                 tcp->name_id_wrap(fni_atom)));
      }

      expected_t<parse_tree_node_t> expr()
      {
        auto ss = stake();
        SILVA_EXPECT_PARSE(fni_expr, num_tokens_left() >= 1, "No tokens left when parsing Expr");
        using atom_delegate_t    = delegate_t<expected_t<parse_tree_node_t>()>;
        const auto atom_delegate = atom_delegate_t::make<&seed_parse_tree_nursery_t::atom>(this);
        ss.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(fni_expr, seed_parse_axe.apply(*this, fni_atom, atom_delegate)));
        return ss.commit();
      }

      expected_t<parse_tree_node_t> expr_or_alias()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_expr_or_a);
        SILVA_EXPECT_PARSE(fni_expr_or_a,
                           num_tokens_left() >= 1,
                           "No more tokens left when parsing ExprOrAlias");
        const token_id_t op_ti = token_id_by();
        SILVA_EXPECT_PARSE(fni_expr_or_a,
                           op_ti == tt_equal || op_ti == tt_alias,
                           "Expected one of [ '=' '=>' ]");
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_expr_or_a, expr()));
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> rule()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_rule);
        const index_t orig_token_index = token_index;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_rule, nonterminal_base()));
        SILVA_EXPECT_PARSE(fni_rule,
                           num_tokens_left() >= 1,
                           "No more tokens left when parsing Rule");
        const token_id_t op_ti = token_id_by();
        SILVA_EXPECT_PARSE(fni_rule,
                           op_ti == tt_equal || op_ti == tt_alias || op_ti == tt_axe ||
                               op_ti == tt_brack_open,
                           "Expected one of [ '=' '=>' '=/' ]");
        if (op_ti == tt_equal && num_tokens_left() >= 2 && token_id_by(1) == tt_brack_open) {
          token_index += 2;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_rule, seed()));
          SILVA_EXPECT_PARSE(fni_rule,
                             num_tokens_left() >= 1 && token_id_by() == tt_brack_close,
                             "Expected ']'");
          token_index += 1;
        }
        else if (op_ti == tt_equal || op_ti == tt_alias) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_rule, expr_or_alias()));
        }
        else if (op_ti == tt_axe) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_rule, axe()));
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> seed()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_seed);
        while (num_tokens_left() >= 1 && token_id_by() == tt_dash) {
          token_index += 1;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_seed, rule()));
        }
        return ss_rule.commit();
      }
    };
  }

  expected_t<unique_ptr_t<parse_tree_t>> seed_parse(shared_ptr_t<const tokenization_t> tokenization)
  {
    expected_traits_t expected_traits{.materialize_fwd = true};
    impl::seed_parse_tree_nursery_t nursery(std::move(tokenization));
    SILVA_EXPECT_FWD(nursery.seed());
    return {std::make_unique<parse_tree_t>(std::move(nursery).finish())};
  }

  unique_ptr_t<seed_engine_t> seed_seed_engine(token_catalog_ptr_t tcp)
  {
    auto retval = std::make_unique<seed_engine_t>(tcp);
    SILVA_EXPECT_ASSERT(retval->add_complete_file("seed.seed", seed_seed));
    return retval;
  }
}
