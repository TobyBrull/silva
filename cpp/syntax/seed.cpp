#include "seed.hpp"

#include "canopy/expected.hpp"

#include "syntax.hpp"

namespace silva {
  using enum token_category_t;

  namespace impl {
    struct seed_axe_parse_tree_nursery_t : public parse_tree_nursery_t {
      token_id_t ti_dot         = *swp->token_id(".");
      token_id_t ti_comma       = *swp->token_id(",");
      token_id_t ti_dash        = *swp->token_id("-");
      token_id_t ti_equal       = *swp->token_id("=");
      token_id_t ti_axe         = *swp->token_id("=/");
      token_id_t ti_alias       = *swp->token_id("=>");
      token_id_t ti_right_arrow = *swp->token_id("->");
      token_id_t ti_brack_open  = *swp->token_id("[");
      token_id_t ti_brack_close = *swp->token_id("]");
      token_id_t ti_paren_open  = *swp->token_id("(");
      token_id_t ti_paren_close = *swp->token_id(")");
      token_id_t ti_identifier  = *swp->token_id("identifier");
      token_id_t ti_regex       = *swp->token_id("/");
      token_id_t ti_up          = *swp->token_id("p");
      token_id_t ti_silva       = *swp->token_id("_");
      token_id_t ti_here        = *swp->token_id("x");
      token_id_t ti_operator    = *swp->token_id("operator");
      token_id_t ti_string      = *swp->token_id("string");
      token_id_t ti_number      = *swp->token_id("number");
      token_id_t ti_any         = *swp->token_id("any");
      token_id_t ti_eof         = *swp->token_id("end_of_file");
      token_id_t ti_nest        = *swp->token_id("nest");
      token_id_t ti_ltr         = *swp->token_id("ltr");
      token_id_t ti_rtl         = *swp->token_id("rtl");
      token_id_t ti_atom_nest   = *swp->token_id("atom_nest");
      token_id_t ti_postfix     = *swp->token_id("postfix");
      token_id_t ti_postfix_n   = *swp->token_id("postfix_nest");
      token_id_t ti_infix       = *swp->token_id("infix");
      token_id_t ti_infix_flat  = *swp->token_id("infix_flat");
      token_id_t ti_ternary     = *swp->token_id("ternary");
      token_id_t ti_prefix      = *swp->token_id("prefix");
      token_id_t ti_prefix_n    = *swp->token_id("prefix_nest");
      token_id_t ti_concat      = *swp->token_id("concat");
      token_id_t ti_keywords_of = *swp->token_id("keywords_of");

      name_id_t ni_seed          = swp->name_id_of("Seed");
      name_id_t ni_rule          = swp->name_id_of(ni_seed, "Rule");
      name_id_t ni_expr_or_a     = swp->name_id_of(ni_seed, "ExprOrAlias");
      name_id_t ni_axe_with_atom = swp->name_id_of(ni_seed, "AxeWithAtom");
      name_id_t ni_expr          = swp->name_id_of(ni_seed, "Expr");
      name_id_t ni_atom          = swp->name_id_of(ni_seed, "Atom");
      name_id_t ni_var           = swp->name_id_of(ni_seed, "Variable");
      name_id_t ni_func          = swp->name_id_of(ni_seed, "Function");
      name_id_t ni_func_name     = swp->name_id_of(ni_func, "Name");
      name_id_t ni_func_arg      = swp->name_id_of(ni_func, "Arg");
      name_id_t ni_func_args     = swp->name_id_of(ni_func, "Args");
      name_id_t ni_axe           = swp->name_id_of(ni_seed, "Axe");
      name_id_t ni_axe_level     = swp->name_id_of(ni_axe, "Level");
      name_id_t ni_axe_assoc     = swp->name_id_of(ni_axe, "Assoc");
      name_id_t ni_axe_ops       = swp->name_id_of(ni_axe, "Ops");
      name_id_t ni_axe_op_type   = swp->name_id_of(ni_axe, "OpType");
      name_id_t ni_axe_op        = swp->name_id_of(ni_axe, "Op");
      name_id_t ni_nt_maybe_var  = swp->name_id_of(ni_seed, "NonterminalMaybeVar");
      name_id_t ni_nt            = swp->name_id_of(ni_seed, "Nonterminal");
      name_id_t ni_nt_base       = swp->name_id_of(ni_nt, "Base");
      name_id_t ni_term          = swp->name_id_of(ni_seed, "Terminal");

      seed_axe_parse_tree_nursery_t(tokenization_ptr_t tp) : parse_tree_nursery_t(tp) {}

      expected_t<parse_tree_node_t> terminal()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_term);
        if (num_tokens_left() >= 3 &&
            (token_id_by(0) == ti_identifier || token_id_by(0) == ti_operator) &&
            token_id_by(1) == ti_regex && token_data_by(2)->category == STRING) {
          token_index += 3;
        }
        else if (num_tokens_left() >= 2 && token_id_by(0) == ti_keywords_of) {
          token_index += 1;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_term, nonterminal()));
        }
        else {
          SILVA_EXPECT_PARSE(ni_term, num_tokens_left() >= 1, "no more tokens in input");
          SILVA_EXPECT_PARSE(ni_term,
                             token_data_by()->category == STRING ||
                                 token_id_by() == ti_identifier || token_id_by() == ti_operator ||
                                 token_id_by() == ti_string || token_id_by() == ti_number ||
                                 token_id_by() == ti_any || token_id_by() == ti_eof,
                             "unexpected {}",
                             swp->token_id_wrap(token_id_by()));
          token_index += 1;
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> nonterminal_base()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_nt_base);
        SILVA_EXPECT_PARSE(ni_nt_base, num_tokens_left() >= 1, "no more tokens in input");
        const bool is_cap_id = token_data_by()->category == IDENTIFIER &&
            !token_data_by()->str.empty() && std::isupper(token_data_by()->str.front());
        const bool is_name_id =
            token_id_by() == ti_up || token_id_by() == ti_here || token_id_by() == ti_silva;
        SILVA_EXPECT_PARSE(ni_nt_base,
                           is_cap_id || is_name_id,
                           "unexpected {}",
                           swp->token_id_wrap(token_id_by()));
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
        SILVA_EXPECT_PARSE(ni_axe_op,
                           num_tokens_left() >= 1 &&
                               (token_id_by() == ti_concat || token_data_by()->category == STRING),
                           "expected {} or string, got {}",
                           swp->token_id_wrap(ti_concat),
                           swp->token_id_wrap(token_id_by()));
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_op_type()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_axe_op_type);
        SILVA_EXPECT_PARSE(
            ni_axe_op_type,
            num_tokens_left() >= 1 &&
                (token_id_by() == ti_atom_nest || token_id_by() == ti_prefix ||
                 token_id_by() == ti_prefix_n || token_id_by() == ti_infix ||
                 token_id_by() == ti_infix_flat || token_id_by() == ti_ternary ||
                 token_id_by() == ti_postfix || token_id_by() == ti_postfix_n),
            "expected one of [ atom_nest prefix prefix_nest infix infix_flat ternary "
            "postfix postfix_nest ], got {}",
            swp->token_id_wrap(token_id_by()));
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_ops()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_axe_ops);
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe_ops, axe_op_type()));
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
            swp->token_id_wrap(token_id_by()));
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
        SILVA_EXPECT_PARSE_TOKEN_ID(ni_axe, ti_brack_open);
        while (num_tokens_left() >= 1 && token_id_by() == ti_dash) {
          token_index += 1;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe, axe_level()));
        }
        SILVA_EXPECT_PARSE_TOKEN_ID(ni_axe, ti_brack_close);
        return ss_rule.commit();
      }
    };
  }

  seed_axe_t create_seed_axe_expr(syntax_ward_ptr_t swp)
  {
    const string_view_t axe_defn = seed_seed.substr(222, 283);
    // fmt::print("\n\n|{}|\n\n", axe_defn);
    auto tt = SILVA_EXPECT_ASSERT(tokenize(swp, "seed.axe", axe_defn));
    impl::seed_axe_parse_tree_nursery_t nursery(tt);
    SILVA_EXPECT_ASSERT(nursery.axe());
    parse_tree_ptr_t pt     = swp->add(std::move(nursery).finish());
    const name_id_t ni_expr = swp->name_id_of("Seed", "Expr");
    auto retval             = SILVA_EXPECT_ASSERT(seed_axe_create(swp, ni_expr, pt->span()));
    return retval;
  }

  namespace impl {
    struct seed_parse_tree_nursery_t : public seed_axe_parse_tree_nursery_t {
      seed_axe_t seed_seed_axe;

      seed_parse_tree_nursery_t(tokenization_ptr_t tp)
        : seed_axe_parse_tree_nursery_t(tp), seed_seed_axe(create_seed_axe_expr(tp->swp))
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
          auto result = terminal();
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

        return std::unexpected(std::move(error_nursery)
                                   .finish_short(error_level_t::MINOR,
                                                 "[{}] {}",
                                                 token_position_at(orig_token_index),
                                                 swp->name_id_wrap(ni_atom)));
      }

      expected_t<parse_tree_node_t> variable()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_var);
        SILVA_EXPECT_PARSE(ni_var, num_tokens_left() >= 1, "no more tokens in input");
        const auto& tstr = token_data_by()->str;
        SILVA_EXPECT_PARSE(ni_var,
                           token_data_by()->category == IDENTIFIER && tstr.size() >= 3 &&
                               std::islower(tstr.front()) && tstr.ends_with("_v"),
                           "unexpected {}",
                           swp->token_id_wrap(token_id_by()));
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
                           token_data_by()->category == IDENTIFIER && tstr.size() >= 3 &&
                               std::islower(tstr.front()) && tstr.ends_with("_f"),
                           "unexpected {}",
                           swp->token_id_wrap(token_id_by()));
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> function_arg()
      {
        auto ss                        = stake();
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;

        {
          auto result = expr();
          if (result) {
            ss.add_proto_node(*result);
            return ss.commit();
          }
          error_nursery.add_child_error(std::move(result).error());
        }

        {
          auto result = variable();
          if (result) {
            ss.add_proto_node(*result);
            return ss.commit();
          }
          error_nursery.add_child_error(std::move(result).error());
        }

        return std::unexpected(std::move(error_nursery)
                                   .finish_short(error_level_t::MINOR,
                                                 "[{}] {}",
                                                 token_position_at(orig_token_index),
                                                 swp->name_id_wrap(ni_func_arg)));
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

      expected_t<parse_tree_node_t> expr()
      {
        auto ss = stake();
        SILVA_EXPECT_PARSE(ni_expr, num_tokens_left() >= 1, "no more tokens in input");
        using atom_delegate_t    = delegate_t<expected_t<parse_tree_node_t>()>;
        const auto atom_delegate = atom_delegate_t::make<&seed_parse_tree_nursery_t::atom>(this);
        ss.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(ni_expr, seed_seed_axe.apply(*this, ni_atom, atom_delegate)));
        return ss.commit();
      }

      expected_t<parse_tree_node_t> expr_or_alias()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_expr_or_a);
        SILVA_EXPECT_PARSE(ni_expr_or_a, num_tokens_left() >= 1, "no more tokens in input");
        const token_id_t op_ti = token_id_by();
        SILVA_EXPECT_PARSE(ni_expr_or_a,
                           op_ti == ti_equal || op_ti == ti_alias,
                           "expected one of [ = => ], got {}",
                           swp->token_id_wrap(op_ti));
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_expr_or_a, expr()));
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> axe_with_atom()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_axe_with_atom);
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe_with_atom, nonterminal()));
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_axe_with_atom, axe()));
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> rule()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_rule);
        const index_t orig_token_index = token_index;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, nonterminal()));
        SILVA_EXPECT_PARSE(ni_rule, num_tokens_left() >= 1, "no more tokens in input");
        const token_id_t op_ti = token_id_by();
        SILVA_EXPECT_PARSE(ni_rule,
                           op_ti == ti_equal || op_ti == ti_alias || op_ti == ti_axe,
                           "expected one of [ = => =/ ], got {}",
                           swp->token_id_wrap(token_id_by()));
        if (op_ti == ti_equal && num_tokens_left() >= 2 && token_id_by(1) == ti_brack_open) {
          token_index += 2;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, seed()));
          SILVA_EXPECT_PARSE_TOKEN_ID(ni_rule, ti_brack_close)
        }
        else if (op_ti == ti_equal || op_ti == ti_alias) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, expr_or_alias()));
        }
        else if (op_ti == ti_axe) {
          token_index += 1;
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(ni_rule, axe_with_atom()));
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

  expected_t<parse_tree_ptr_t> seed_parse(tokenization_ptr_t tp)
  {
    impl::seed_parse_tree_nursery_t nursery(tp);
    SILVA_EXPECT_FWD(nursery.seed());
    return tp->swp->add(std::move(nursery).finish());
  }

  string_t name_id_style_t::absolute(const name_id_t target_fni) const
  {
    if (target_fni == name_id_root) {
      return swp->token_infos[root].str;
    }
    const name_info_t& fni = swp->name_infos[target_fni];
    return absolute(fni.parent_name) + swp->token_infos[separator].str +
        swp->token_infos[fni.base_name].str;
  }

  string_t name_id_style_t::relative(const name_id_t current_fni, const name_id_t target_fni) const
  {
    const name_id_t lca = swp->name_id_lca(current_fni, target_fni);

    string_t first_part;
    {
      name_id_t curr = current_fni;
      while (curr != lca) {
        if (!first_part.empty()) {
          first_part += swp->token_infos[separator].str;
        }
        first_part += swp->token_infos[parent].str;
        curr = swp->name_infos[curr].parent_name;
      }
    }

    string_t second_part;
    {
      name_id_t curr = target_fni;
      while (curr != lca) {
        if (!second_part.empty()) {
          second_part = swp->token_infos[separator].str + second_part;
        }
        const name_info_t* fni = &swp->name_infos[curr];
        second_part            = swp->token_infos[fni->base_name].str + second_part;
        curr                   = swp->name_infos[curr].parent_name;
      }
    }
    if (!first_part.empty() && !second_part.empty()) {
      return first_part + swp->token_infos[separator].str + second_part;
    }
    else if (first_part.empty() && second_part.empty()) {
      return swp->token_infos[current].str;
    }
    else {
      return first_part + second_part;
    }
  }

  expected_t<token_id_t> name_id_style_t::derive_base_name(const name_id_t scope_name,
                                                           const parse_tree_span_t pts_nt) const
  {
    const auto& s_node = pts_nt[0];
    SILVA_EXPECT(s_node.rule_name == ni_nonterminal_base && s_node.num_children == 0,
                 MINOR,
                 "expected Nonterminal.Base");
    const token_id_t retval = pts_nt.tp->tokens[s_node.token_begin];
    return retval;
  }

  expected_t<name_id_t>
  name_id_style_t::derive_relative_name(const name_id_t scope_name,
                                        const parse_tree_span_t pts_nt_base) const
  {
    name_id_t retval     = scope_name;
    const auto base_name = SILVA_EXPECT_FWD(derive_base_name(scope_name, pts_nt_base));
    if (base_name == current) {
      return scope_name;
    }
    else {
      return swp->name_id(scope_name, base_name);
    }
    return retval;
  }

  expected_t<name_id_t> name_id_style_t::derive_name(const name_id_t scope_name,
                                                     const parse_tree_span_t pts_nt) const
  {
    name_id_t retval = scope_name;
    SILVA_EXPECT(pts_nt[0].rule_name == ni_nonterminal, MINOR, "expected Nonterminal");
    for (const auto [child_node_index, child_index]: pts_nt.children_range()) {
      const auto& s_node = pts_nt[child_node_index];
      SILVA_EXPECT(s_node.rule_name == ni_nonterminal_base, MINOR, "expected Nonterminal.Base");
      const token_id_t base = pts_nt.tp->tokens[s_node.token_begin];
      if (base == root) {
        SILVA_EXPECT(child_index == 0, MINOR, "Root node may only appear as first element");
        retval = name_id_root;
      }
      else if (base == current) {
        ;
      }
      else if (base == parent) {
        retval = swp->name_infos[retval].parent_name;
      }
      else {
        retval = swp->name_id(retval, base);
      }
    }
    return retval;
  }
}
