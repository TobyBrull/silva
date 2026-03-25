#include "seed.hpp"

#include "seed_axe.hpp"

#include "canopy/expected.hpp"

using enum silva::token_category_old_t;

namespace silva::seed::impl {
  struct axe_parse_tree_nursery_t : public parse_tree_nursery_t {
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

    axe_parse_tree_nursery_t(tokenization_ptr_t tp) : parse_tree_nursery_t(tp) {}

    expected_t<parse_tree_node_t> token_category()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_tok_cat);
      SILVA_EXPECT_PARSE(ni_tok_cat, num_tokens_left() >= 1, "no more tokens in input");
      const auto& tstr = token_data_by()->str;
      SILVA_EXPECT_PARSE(ni_tok_cat,
                         token_data_by()->category_old == IDENTIFIER && !tstr.empty() &&
                             std::islower(tstr.front()),
                         "unexpected {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> terminal()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_term);
      if (num_tokens_left() >= 3 &&
          (token_id_by(0) == ti_identifier || token_id_by(0) == ti_operator) &&
          token_id_by(1) == ti_regex && token_data_by(2)->category_old == STRING) {
        token_index += 3;
      }
      else if (num_tokens_left() >= 2 && token_id_by(0) == ti_keywords_of) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_term, nonterminal()));
      }
      else {
        SILVA_EXPECT_PARSE(ni_term, num_tokens_left() >= 1, "no more tokens in input");
        if (token_data_by()->category_old == STRING || token_id_by() == ti_identifier ||
            token_id_by() == ti_operator || token_id_by() == ti_string ||
            token_id_by() == ti_number || token_id_by() == ti_any || token_id_by() == ti_eps ||
            token_id_by() == ti_eof) {
          token_index += 1;
        }
        else {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_term, token_category()));
        }
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> nonterminal_base()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_nt_base);
      SILVA_EXPECT_PARSE(ni_nt_base, num_tokens_left() >= 1, "no more tokens in input");
      const bool is_cap_id = token_data_by()->category_old == IDENTIFIER &&
          !token_data_by()->str.empty() && std::isupper(token_data_by()->str.front());
      const bool is_name_id =
          token_id_by() == ti_up || token_id_by() == ti_here || token_id_by() == ti_silva;
      SILVA_EXPECT_PARSE(ni_nt_base,
                         is_cap_id || is_name_id,
                         "unexpected {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> nonterminal()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_nt);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_nt, nonterminal_base()));
      while (num_tokens_left() >= 2 && token_id_by() == ti_dot) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_nt, nonterminal_base()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_op()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_axe_op);
      SILVA_EXPECT_PARSE(
          ni_axe_op,
          num_tokens_left() >= 1 &&
              (token_id_by() == ti_concat || token_data_by()->category_old == STRING),
          "expected {} or string, got {}",
          sfp->token_id_wrap(ti_concat),
          sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_op_type()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_axe_op_type);
      SILVA_EXPECT_PARSE(ni_axe_op_type,
                         num_tokens_left() >= 1 &&
                             (token_id_by() == ti_atom_nest || token_id_by() == ti_prefix ||
                              token_id_by() == ti_prefix_n || token_id_by() == ti_infix ||
                              token_id_by() == ti_infix_flat || token_id_by() == ti_ternary ||
                              token_id_by() == ti_postfix || token_id_by() == ti_postfix_n),
                         "expected one of [ atom_nest prefix prefix_nest infix infix_flat ternary "
                         "postfix postfix_nest ], got {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_ops()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_axe_ops);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe_ops, axe_op_type()));
      if (num_tokens_left() > 0 && token_id_by() == ti_right_arrow) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe_ops, nonterminal()));
      }
      while (auto result = axe_op()) {
        ss_rule.add_proto_node(*result);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_assoc()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_axe_assoc);
      SILVA_EXPECT_PARSE(
          ni_axe_assoc,
          num_tokens_left() >= 1 &&
              (token_id_by() == ti_nest || token_id_by() == ti_ltr || token_id_by() == ti_rtl),
          "expected one of [ nest ltr rtl ], got {}",
          sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_level()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_axe_level);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe_level, nonterminal_base()));
      SILVA_EXPECT_PARSE_TOKEN_ID(ni_axe_level, ti_equal);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe_level, axe_assoc()));
      while (auto result = axe_ops()) {
        ss_rule.add_proto_node(*result);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_axe);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe, nonterminal()));
      SILVA_EXPECT_PARSE_TOKEN_ID(ni_axe, ti_brack_open);
      while (num_tokens_left() >= 1 && token_id_by() == ti_dash) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe, axe_level()));
      }
      SILVA_EXPECT_PARSE_TOKEN_ID(ni_axe, ti_brack_close);
      return ss_rule.commit();
    }
  };

  axe_t create_axe_expr(syntax_farm_ptr_t sfp)
  {
    const string_view_t axe_defn = [] -> string_view_t {
      const string_view_t start_str = "- Expr = axe ";
      const size_t fi_begin         = seed_str.find(start_str);
      SILVA_ASSERT(fi_begin != string_view_t::npos);
      const string_view_t axe_defn_str_with_rest = seed_str.substr(fi_begin + start_str.size());
      const size_t fi_end                        = axe_defn_str_with_rest.find(']');
      SILVA_ASSERT(fi_end != string_view_t::npos);
      const string_view_t retval = axe_defn_str_with_rest.substr(0, fi_end + 1);
      // fmt::print("\n\n|{}|\n\n", retval);
      return retval;
    }();
    auto tt = SILVA_EXPECT_ASSERT(tokenize(sfp, "seed.axe", axe_defn));
    impl::axe_parse_tree_nursery_t nursery(tt);
    SILVA_EXPECT_ASSERT(nursery.axe());
    parse_tree_ptr_t pt     = sfp->add(std::move(nursery).finish());
    const name_id_t ni_expr = sfp->name_id_of("Seed", "Expr");
    auto retval             = SILVA_EXPECT_ASSERT(axe_create(sfp, ni_expr, pt->span()));
    return retval;
  }

  struct seed_parse_tree_nursery_t : public axe_parse_tree_nursery_t {
    axe_t the_axe;

    seed_parse_tree_nursery_t(tokenization_ptr_t tp)
      : axe_parse_tree_nursery_t(tp), the_axe(create_axe_expr(tp->sfp))
    {
    }

    expected_t<parse_tree_node_t> nonterminal_maybe_var()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_nt_maybe_var);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_atom, nonterminal()));
      if (num_tokens_left() >= 2 && token_id_by() == ti_right_arrow) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_atom, variable()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> atom()
    {
      auto ss                        = stake();
      const index_t orig_token_index = token_index;
      error_nursery_t error_nursery;

      {
        auto result = nonterminal_maybe_var();
        if (result) {
          ss.add_proto_node(*result);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }

      {
        auto result = function();
        if (result) {
          ss.add_proto_node(*result);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }

      {
        auto result = terminal();
        if (result) {
          ss.add_proto_node(*result);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }

      return std::unexpected(std::move(error_nursery)
                                 .finish_short(error_level_t::MINOR,
                                               "[{}] {}",
                                               token_location_at(orig_token_index),
                                               sfp->name_id_wrap(ni_atom)));
    }

    expected_t<parse_tree_node_t> variable()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_var);
      SILVA_EXPECT_PARSE(ni_var, num_tokens_left() >= 1, "no more tokens in input");
      const auto& tstr = token_data_by()->str;
      SILVA_EXPECT_PARSE(ni_var,
                         token_data_by()->category_old == IDENTIFIER && tstr.size() >= 3 &&
                             std::islower(tstr.front()) && tstr.ends_with("_v"),
                         "unexpected {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> function()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_func);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_func, function_name()));
      SILVA_EXPECT_PARSE_TOKEN_ID(ni_func, ti_paren_open);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_func, function_args()));
      SILVA_EXPECT_PARSE_TOKEN_ID(ni_func, ti_paren_close);
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> function_name()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_func_name);
      SILVA_EXPECT_PARSE(ni_func_name, num_tokens_left() >= 1, "no more tokens in input");
      const auto& tstr = token_data_by()->str;
      SILVA_EXPECT_PARSE(ni_func_name,
                         token_data_by()->category_old == IDENTIFIER && tstr.size() >= 3 &&
                             std::islower(tstr.front()) && tstr.ends_with("_f"),
                         "unexpected {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> function_arg()
    {
      auto ss                        = stake();
      const index_t orig_token_index = token_index;
      error_nursery_t error_nursery;

      {
        auto result = variable();
        if (result) {
          ss.add_proto_node(*result);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }

      {
        auto result = expr();
        if (result) {
          ss.add_proto_node(*result);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }

      return std::unexpected(std::move(error_nursery)
                                 .finish_short(error_level_t::MINOR,
                                               "[{}] {}",
                                               token_location_at(orig_token_index),
                                               sfp->name_id_wrap(ni_func_arg)));
    }

    expected_t<parse_tree_node_t> function_args()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_func_args);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_func_args, function_arg()));
      while (num_tokens_left() >= 2 && token_id_by() == ti_comma) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_func_args, function_arg()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> any_rule(const name_id_t rule_name)
    {
      if (rule_name == ni_atom) {
        return atom();
      }
      else if (rule_name == ni_expr) {
        return expr();
      }
      else {
        SILVA_EXPECT(false, MAJOR, "unexpected rule {}", sfp->name_id_wrap(rule_name));
      }
    }

    expected_t<parse_tree_node_t> alias()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_alias);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_alias, expr()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> expr()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE(ni_expr, num_tokens_left() >= 1, "no more tokens in input");
      const auto dg = axe_t::parse_delegate_t::make<&seed_parse_tree_nursery_t::any_rule>(this);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_expr, the_axe.apply(*this, dg)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_rule);
      const index_t orig_token_index = token_index;
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, nonterminal()));
      SILVA_EXPECT_PARSE(ni_rule, num_tokens_left() >= 2, "not enough tokens in input");
      SILVA_EXPECT_PARSE_TOKEN_ID(ni_rule, ti_equal);
      const token_id_t op_ti = token_id_by();
      if (op_ti == ti_brack_open) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, seed()));
        SILVA_EXPECT_PARSE_TOKEN_ID(ni_rule, ti_brack_close)
      }
      else if (op_ti == ti_alias) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, alias()));
      }
      else if (op_ti == ti_axe) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, axe()));
      }
      else {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, expr()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> seed()
    {
      auto ss_rule = stake();
      ss_rule.create_node(ni_seed);
      while (num_tokens_left() >= 1 && token_id_by() == ti_dash) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_seed, rule()));
      }
      return ss_rule.commit();
    }
  };
}

namespace silva::seed {
  expected_t<parse_tree_ptr_t> parse(tokenization_ptr_t tp)
  {
    impl::seed_parse_tree_nursery_t nursery(tp);
    SILVA_EXPECT_FWD(nursery.seed());
    return tp->sfp->add(std::move(nursery).finish());
  }
}
