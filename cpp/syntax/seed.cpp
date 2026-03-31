#include "seed.hpp"

#include "fragmentization.hpp"
#include "seed_axe.hpp"
#include "seed_tokenizer.hpp"

#include "canopy/expected.hpp"

namespace silva::seed::impl {
  struct base_parse_tree_nursery_t : public parse_tree_nursery_t {
    const lexicon_t& lexicon;

    base_parse_tree_nursery_t(tokenization_ptr_t tp, const lexicon_t& lexicon)
      : parse_tree_nursery_t(tp), lexicon(lexicon)
    {
    }

    expected_t<parse_tree_node_t> terminal()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_term);
      if (num_tokens_left() >= 2 && token_id_by(0) == lexicon.ti_keywords_of) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_term, nonterminal()));
      }
      else {
        SILVA_EXPECT_PARSE(lexicon.ni_term, num_tokens_left() >= 1, "no tokens left");
        SILVA_EXPECT_PARSE(lexicon.ni_term,
                           token_category_by() == lexicon.ti_string ||
                               token_category_by() == lexicon.ti_token_cat_name ||
                               token_id_by() == lexicon.ti_any || token_id_by() == lexicon.ti_eps ||
                               token_id_by() == lexicon.ti_eof,
                           "unexpected {}",
                           sfp->token_id_wrap(token_id_by()));
        token_index += 1;
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> nonterminal_base()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_nt_base);
      SILVA_EXPECT_PARSE(lexicon.ni_nt_base, num_tokens_left() >= 1, "no tokens left");
      SILVA_EXPECT_PARSE(lexicon.ni_nt_base,
                         token_id_by() == lexicon.ti_up || token_id_by() == lexicon.ti_here ||
                             token_id_by() == lexicon.ti_silva ||
                             token_category_by() == lexicon.ti_rule_name,
                         "unexpected {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> nonterminal()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_nt);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_nt, nonterminal_base()));
      while (num_tokens_left() >= 2 && token_id_by() == lexicon.ti_dot) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_nt, nonterminal_base()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_op()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_op);
      SILVA_EXPECT_PARSE(lexicon.ni_axe_op, num_tokens_left() >= 1, "no tokens left");
      SILVA_EXPECT_PARSE(lexicon.ni_axe_op,
                         token_category_by() == lexicon.ti_string ||
                             token_id_by() == lexicon.ti_concat,
                         "expected {} or string, got {}",
                         sfp->token_id_wrap(lexicon.ti_concat),
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_op_type()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_op_type);
      SILVA_EXPECT_PARSE(lexicon.ni_axe_op_type, num_tokens_left() >= 1, "no tokens left");
      SILVA_EXPECT_PARSE(
          lexicon.ni_axe_op_type,
          token_id_by() == lexicon.ti_atom_nest || token_id_by() == lexicon.ti_prefix ||
              token_id_by() == lexicon.ti_prefix_n || token_id_by() == lexicon.ti_infix ||
              token_id_by() == lexicon.ti_infix_flat || token_id_by() == lexicon.ti_ternary ||
              token_id_by() == lexicon.ti_postfix || token_id_by() == lexicon.ti_postfix_n,
          "expected one of [ atom_nest prefix prefix_nest infix infix_flat ternary "
          "postfix postfix_nest ], got {}",
          sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_ops()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_ops);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_ops, axe_op_type()));
      if (num_tokens_left() > 0 && token_id_by() == lexicon.ti_right_arrow) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_ops, nonterminal()));
      }
      while (auto result = axe_op()) {
        ss_rule.add_proto_node(*result);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_assoc()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_assoc);
      SILVA_EXPECT_PARSE(lexicon.ni_axe_assoc,
                         num_tokens_left() >= 1 &&
                             (token_id_by() == lexicon.ti_nest || token_id_by() == lexicon.ti_ltr ||
                              token_id_by() == lexicon.ti_rtl),
                         "expected one of [ nest ltr rtl ], got {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_level()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_level);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_level, nonterminal_base()));
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_axe_level, lexicon.ti_equal);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_level, axe_assoc()));
      while (auto result = axe_ops()) {
        ss_rule.add_proto_node(*result);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe, nonterminal()));
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_axe, lexicon.ti_brack_open);
      while (num_tokens_left() >= 1 && token_id_by() == lexicon.ti_dash) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe, axe_level()));
      }
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_axe, lexicon.ti_brack_close);
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_matcher()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_matcher);
      SILVA_EXPECT_PARSE(lexicon.ni_tok_matcher, num_tokens_left() >= 1, "no tokens left");
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_tok_matcher, lexicon.ti_frag_name);
      if (num_tokens_left() >= 2 && token_id_by() == lexicon.ti_slash &&
          token_category_by(1) == lexicon.ti_string) {
        token_index += 2;
      }
      if (num_tokens_left() >= 2 && token_id_by() == lexicon.ti_backslash &&
          token_category_by(1) == lexicon.ti_string) {
        token_index += 2;
      }
      if (num_tokens_left() >= 2 && token_id_by() == lexicon.ti_pipe &&
          token_category_by(1) == lexicon.ti_string) {
        token_index += 2;
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_item()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_item);
      SILVA_EXPECT_PARSE(lexicon.ni_tok_item, num_tokens_left() >= 1, "no more tokens in input");
      if (token_category_by() == lexicon.ti_string) {
        token_index += 1;
      }
      else {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_tok_item, tokenizer_matcher()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_list()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_list);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok_list, lexicon.ti_brack_open);
      while (auto result = tokenizer_item()) {
        ss_rule.add_proto_node(*result);
      }
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok_list, lexicon.ti_brack_close);
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_prefix_item()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_prefix_item);
      SILVA_EXPECT_PARSE(lexicon.ni_tok_prefix_item, num_tokens_left() >= 1, "no tokens left");
      if (token_id_by() == lexicon.ti_brack_open) {
        ss_rule.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(lexicon.ni_tok_prefix_item, tokenizer_list()));
      }
      else {
        ss_rule.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(lexicon.ni_tok_prefix_item, tokenizer_item()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_defn()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_defn);
      while (auto result = tokenizer_prefix_item()) {
        ss_rule.add_proto_node(*result);
      }
      if (num_tokens_left() >= 1 && token_id_by() == lexicon.ti_triple_colon) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_tok_defn, tokenizer_item()));
        while (auto result = tokenizer_item()) {
          ss_rule.add_proto_node(*result);
        }
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_include_rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_inc_rule);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok_inc_rule, lexicon.ti_include);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok_inc_rule, lexicon.ti_tokenizer);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_tok_inc_rule, nonterminal_base()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_ignore_rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_ign_rule);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok_ign_rule, lexicon.ti_ignore);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_tok_ign_rule, tokenizer_defn()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer_token_rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok_tok_rule);
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_tok_tok_rule, lexicon.ti_token_cat_name);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok_tok_rule, lexicon.ti_equal);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_tok_tok_rule, tokenizer_defn()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> tokenizer()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_tok);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok, lexicon.ti_brack_open);
      while (num_tokens_left() >= 1 && token_id_by() == lexicon.ti_dash) {
        token_index += 1;
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;
        {
          auto result = tokenizer_include_rule();
          if (result) {
            ss_rule.add_proto_node(*result);
            continue;
          }
          error_nursery.add_child_error(std::move(result).error());
        }
        {
          auto result = tokenizer_ignore_rule();
          if (result) {
            ss_rule.add_proto_node(*result);
            continue;
          }
          error_nursery.add_child_error(std::move(result).error());
        }
        {
          auto result = tokenizer_token_rule();
          if (result) {
            ss_rule.add_proto_node(*result);
            continue;
          }
          error_nursery.add_child_error(std::move(result).error());
        }
        return std::unexpected(std::move(error_nursery)
                                   .finish_short(error_level_t::MINOR,
                                                 "[{}] {}",
                                                 token_location_at(orig_token_index),
                                                 sfp->name_id_wrap(lexicon.ni_tok)));
      }
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_tok, lexicon.ti_brack_close);
      return ss_rule.commit();
    }
  };

  string_view_t find_subsection(string_view_t needle, string_view_t haystack)
  {
    const size_t pos = haystack.find(needle);
    SILVA_ASSERT(pos != string_view_t::npos);
    const string_view_t rest = seed_str.substr(pos + needle.size());
    const size_t pos2        = rest.find("]\n");
    SILVA_ASSERT(pos2 != string_view_t::npos);
    const string_view_t retval = rest.substr(0, pos2 + 2);
    return retval;
  }

  axe_t make_bootstrap_seed_expr_axe(syntax_farm_ptr_t sfp,
                                     const lexicon_t& lexicon,
                                     tokenizer_farm_t& tf)
  {
    const auto axe_text = find_subsection("- Expr = axe ", seed_str);
    const auto axe_frag = SILVA_EXPECT_ASSERT(fragmentize(sfp, "seed.axe", string_t{axe_text}));
    const auto axe_tok  = SILVA_EXPECT_ASSERT(tf.apply(axe_frag, lexicon.ti_r_seed));
    impl::base_parse_tree_nursery_t nursery(axe_tok, lexicon);
    SILVA_EXPECT_ASSERT(nursery.axe());
    parse_tree_ptr_t pt     = sfp->add(std::move(nursery).finish());
    const name_id_t ni_expr = sfp->name_id_of("Seed", "Expr");
    auto retval             = SILVA_EXPECT_ASSERT(axe_create(sfp, ni_expr, pt->span()));
    return retval;
  }

  struct seed_parse_tree_nursery_t : public base_parse_tree_nursery_t {
    axe_t& seed_expr_axe;

    seed_parse_tree_nursery_t(tokenization_ptr_t tp, const lexicon_t& lexicon, axe_t& seed_expr_axe)
      : base_parse_tree_nursery_t(tp, lexicon), seed_expr_axe(seed_expr_axe)
    {
    }

    expected_t<parse_tree_node_t> nonterminal_maybe_var()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_nt_maybe_var);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, nonterminal()));
      if (num_tokens_left() >= 2 && token_id_by() == lexicon.ti_right_arrow &&
          token_category_by(1) == lexicon.ti_var_name) {
        token_index += 2;
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
                                               sfp->name_id_wrap(lexicon.ni_atom)));
    }

    expected_t<parse_tree_node_t> function()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_func);
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_func, lexicon.ti_func_name);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_func, lexicon.ti_paren_open);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_func, function_args()));
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_func, lexicon.ti_paren_close);
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> function_arg()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_func_arg);
      SILVA_EXPECT_PARSE(lexicon.ni_term, num_tokens_left() >= 1, "no tokens left");
      if (token_category_by() == lexicon.ti_var_name) {
        token_index += 1;
      }
      else {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_func, expr()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> function_args()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_func_args);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_func_args, function_arg()));
      while (num_tokens_left() >= 2 && token_id_by() == lexicon.ti_comma) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_FWD(function_arg(), MAJOR));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> any_rule(const name_id_t rule_name)
    {
      if (rule_name == lexicon.ni_atom) {
        return atom();
      }
      else if (rule_name == lexicon.ni_expr) {
        return expr();
      }
      else {
        SILVA_EXPECT(false, MAJOR, "unexpected rule {}", sfp->name_id_wrap(rule_name));
      }
    }

    expected_t<parse_tree_node_t> alias()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_alias);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_alias, expr()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> expr()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE(lexicon.ni_expr, num_tokens_left() >= 1, "no more tokens in input");
      const auto dg = axe_t::parse_delegate_t::make<&seed_parse_tree_nursery_t::any_rule>(this);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_expr, seed_expr_axe.apply(*this, dg)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_rule);
      const index_t orig_token_index = token_index;
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, nonterminal()));
      SILVA_EXPECT_PARSE(lexicon.ni_rule, num_tokens_left() >= 2, "not enough tokens in input");
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_rule, lexicon.ti_equal);
      const token_id_t op_ti = token_id_by();
      if (op_ti == lexicon.ti_brack_open) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, seed()));
        SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_rule, lexicon.ti_brack_close)
      }
      else if (op_ti == lexicon.ti_tokenizer) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, tokenizer()));
      }
      else if (op_ti == lexicon.ti_alias) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, alias()));
      }
      else if (op_ti == lexicon.ti_axe) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, axe()));
      }
      else {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, expr()));
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> seed()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_seed);
      while (num_tokens_left() >= 1 && token_id_by() == lexicon.ti_dash) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_seed, rule()));
      }
      return ss_rule.commit();
    }
  };
}

namespace silva::seed {
  struct bootstrap_interpreter_t::impl_t {
    syntax_farm_ptr_t sfp;
    lexicon_t lexicon;
    axe_t seed_expr_axe;
    tokenizer_farm_t tokenizer_farm;

    impl_t(syntax_farm_ptr_t sfp) : sfp(sfp), lexicon(sfp), tokenizer_farm(sfp)
    {
      tokenizer_farm = make_bootstrap_tokenizer_farm(sfp);
      seed_expr_axe  = impl::make_bootstrap_seed_expr_axe(sfp, lexicon, tokenizer_farm);
    }

    expected_t<parse_tree_ptr_t> parse(fragment_span_t fs)
    {
      tokenization_ptr_t tp = SILVA_EXPECT_FWD(tokenizer_farm.apply(fs, lexicon.ti_r_seed));
      impl::seed_parse_tree_nursery_t nursery(tp, lexicon, seed_expr_axe);
      SILVA_EXPECT_FWD(nursery.seed());
      SILVA_EXPECT(nursery.token_index == tp->tokens.size(),
                   MINOR,
                   "could not parse entire text; stopped at {}",
                   nursery.token_location_by());
      return tp->sfp->add(std::move(nursery).finish());
    }
  };

  bootstrap_interpreter_t::bootstrap_interpreter_t(syntax_farm_ptr_t sfp)
    : impl(std::make_unique<impl_t>(std::move(sfp)))
  {
  }

  bootstrap_interpreter_t::~bootstrap_interpreter_t() = default;

  const lexicon_t& bootstrap_interpreter_t::lexicon() const
  {
    return impl->lexicon;
  }
  const tokenizer_farm_t& bootstrap_interpreter_t::tokenizer_farm() const
  {
    return impl->tokenizer_farm;
  }

  expected_t<parse_tree_ptr_t> bootstrap_interpreter_t::parse(fragment_span_t fs)
  {
    return impl->parse(std::move(fs));
  }
}
