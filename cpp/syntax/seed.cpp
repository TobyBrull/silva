#include "seed.hpp"

#include "fragmentization.hpp"
#include "seed_axe.hpp"

#include "canopy/expected.hpp"
#include "syntax/parse_tree_nursery.hpp"

namespace silva::seed::impl {
  using enum fragment_category_t;

  struct base_parse_tree_nursery_t : public parse_tree_nursery_t {
    const lexicon_t& lexicon;

    base_parse_tree_nursery_t(fragmentization_ptr_t fp, const lexicon_t& lexicon)
      : parse_tree_nursery_t(fp), lexicon(lexicon)
    {
    }

    expected_t<token_t> literal_token(const fragmented_token_t ft)
    {
      auto ts         = token_stake(ft.ti);
      const index_t n = ft.items.size();
      SILVA_EXPECT_PARSE(ft.ti, num_fragments_left() >= n, "not enough fragments left");
      for (index_t i = 0; i < n; ++i) {
        SILVA_EXPECT(ft.items[i].codepoint == fp->get_unique_codepoint(fragment_index), MINOR);
        fragment_index += 1;
      }
      return ts.commit();
    }

    expected_t<token_t> identifier()
    {
      auto ts = token_stake(lexicon.ni_id);
      SILVA_EXPECT_PARSE(lexicon.ni_id,
                         is_fragment_category_id_start(fragment_category_by()),
                         "expected fragment with category ID_START");
      fragment_index += 1;
      while (num_fragments_left() >= 1 &&
             is_fragment_category_id_continue(fragment_category_by())) {
        fragment_index += 1;
      }
      return ts.commit();
    }

    expected_t<token_t> identifier_snake_case()
    {
      auto ts = token_stake(lexicon.ni_id_snake);
      SILVA_EXPECT_PARSE(lexicon.ni_id_snake,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER,
                         "expected fragment with category ID_LOWER");
      fragment_index += 1;
      while (num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER) {
        fragment_index += 1;
      }
      while (num_fragments_left() >= 2 && fragment_unique_codepoint_or_zero_by(0) == U'_' &&
             fragment_category_by(1) == ID_LOWER) {
        fragment_index += 2;
        while (num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER) {
          fragment_index += 1;
        }
      }
      return ts.commit();
    }

    expected_t<token_t> identifier_pascal_case()
    {
      auto ts = token_stake(lexicon.ni_id_pascal);
      SILVA_EXPECT_PARSE(lexicon.ni_id_pascal,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_UPPER,
                         "expected fragment with category ID_UPPER");
      fragment_index += 1;
      SILVA_EXPECT_PARSE(lexicon.ni_id_pascal,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER,
                         "expected fragment with category ID_LOWER");
      fragment_index += 1;
      while (num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER) {
        fragment_index += 1;
      }
      while (num_fragments_left() >= 2 && fragment_category_by(0) == ID_UPPER &&
             fragment_category_by(1) == ID_LOWER) {
        fragment_index += 2;
        while (num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER) {
          fragment_index += 1;
        }
      }
      return ts.commit();
    }

    expected_t<token_t> identifier_macro_case()
    {
      auto ts = token_stake(lexicon.ni_id_macro);
      SILVA_EXPECT_PARSE(lexicon.ni_id_macro,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_UPPER,
                         "expected fragment with category ID_UPPER");
      fragment_index += 1;
      while (num_fragments_left() >= 1 && fragment_category_by() == ID_UPPER) {
        fragment_index += 1;
      }
      while (num_fragments_left() >= 2 && fragment_unique_codepoint_or_zero_by(0) == U'_' &&
             fragment_category_by(1) == ID_UPPER) {
        fragment_index += 2;
        while (num_fragments_left() >= 1 && fragment_category_by() == ID_UPPER) {
          fragment_index += 1;
        }
      }
      return ts.commit();
    }

    expected_t<token_t> string()
    {
      auto ts = token_stake(lexicon.ni_string);
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_string, STRING);
      return ts.commit();
    }

    expected_t<token_t> frag_name()
    {
      auto ts = token_stake(lexicon.ni_frag_name);
      ts.add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_frag_name, identifier_macro_case()));
      return ts.commit();
    }
    expected_t<token_t> rule_name()
    {
      auto ts = token_stake(lexicon.ni_rule_name);
      ts.add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule_name, identifier_pascal_case()));
      return ts.commit();
    }
    expected_t<token_t> token_category_name()
    {
      auto ts = token_stake(lexicon.ni_token_cat_name);
      ts.add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_token_cat_name, identifier_snake_case()));
      return ts.commit();
    }

    expected_t<parse_tree_node_t> terminal()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_term);
      error_nursery_t error_nursery;
      for (const auto& ft: {lexicon.ti_any, lexicon.ti_eps, lexicon.ti_eof}) {
        auto result = literal_token(ft);
        if (result) {
          add_token(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = string();
        if (result) {
          add_token(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = frag_name();
        if (result) {
          add_token(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      return std::unexpected(std::move(error_nursery)
                                 .finish_short(error_level_t::MINOR,
                                               "[{}] {}",
                                               fragment_location_at(orig_frag_idx),
                                               lexicon.name_id_wrap(lexicon.ni_atom)));
    }

    expected_t<parse_tree_node_t> nonterminal()
    {
      auto ss_rule = stake();

      ss_rule.create_node(lexicon.ni_nt);
      {
        auto result = literal_token(lexicon.ti_dot);
        (void)result;
      }

      while (true) {
        auto ss_local = stake();
        auto result_1 = rule_name();
        if (!result_1) {
          break;
        }
        auto result_2 = literal_token(lexicon.ti_dot);
        if (!result_2) {
          break;
        }
        ss_rule.add_proto_node(ss_local.commit());
      }

      error_nursery_t error_nursery;
      {
        auto result = rule_name();
        if (result) {
          add_token(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = token_category_name();
        if (result) {
          add_token(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      return std::unexpected(std::move(error_nursery)
                                 .finish_short(error_level_t::MINOR,
                                               "[{}] {}",
                                               fragment_location_at(fragment_index),
                                               lexicon.name_id_wrap(lexicon.ni_atom)));
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
          token_id_by() == lexicon.ti_prefix || token_id_by() == lexicon.ti_prefix_n ||
              token_id_by() == lexicon.ti_infix || token_id_by() == lexicon.ti_infix_flat ||
              token_id_by() == lexicon.ti_ternary || token_id_by() == lexicon.ti_postfix ||
              token_id_by() == lexicon.ti_postfix_n,
          "expected one of [ prefix prefix_nest infix infix_flat ternary postfix postfix_nest ], "
          "got {}",
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
                             (token_id_by() == lexicon.ti_ltr || token_id_by() == lexicon.ti_rtl),
                         "expected one of [ nest ltr rtl ], got {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_level()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_level);
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_axe_level, lexicon.ti_rule_name);
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
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_axe, lexicon.ti_newline);
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_axe, lexicon.ti_indent);
      while (num_tokens_left() >= 1 && token_category_by() != lexicon.ti_dedent) {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe, axe_level()));
        SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_axe, lexicon.ti_newline);
      }
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_axe, lexicon.ti_dedent);
      return ss_rule.commit();
    }
  };

  string_view_t
  find_subsection(const string_view_t haystack, const string_view_t start, const string_view_t end)
  {
    const size_t pos_start = haystack.find(start);
    SILVA_ASSERT(pos_start != string_view_t::npos);
    const size_t pos_from = pos_start + start.size();

    const size_t pos_to = haystack.find(end);
    SILVA_ASSERT(pos_to != string_view_t::npos);
    SILVA_ASSERT(pos_from < pos_to);
    const string_view_t retval = haystack.substr(pos_from, pos_to - pos_from);
    return retval;
  }

  axe_t make_bootstrap_seed_expr_axe(syntax_farm_ptr_t sfp, const lexicon_t& lexicon)
  {
    const auto axe_text = find_subsection(seed_str, "⊙ = axe ", "    Atom = alias");
    const auto axe_frag = SILVA_EXPECT_ASSERT(fragmentize(sfp, "seed.axe", string_t{axe_text}));
    const auto axe_tok  = SILVA_EXPECT_ASSERT(tf.apply(axe_frag, lexicon.ti_r_seed));
    impl::base_parse_tree_nursery_t nursery(axe_tok, lexicon);
    SILVA_EXPECT_ASSERT(nursery.axe());
    parse_tree_ptr_t pt = sfp->add(std::move(nursery).finish());
    auto retval         = SILVA_EXPECT_ASSERT(axe_create(sfp, lexicon.ni_expr, pt->span()));
    {
      hash_set_t<name_id_t> atom_set;
      atom_set.insert(lexicon.ni_atom);
      SILVA_ASSERT(retval.compile(lexicon, atom_set));
    }
    return retval;
  }

  struct seed_parse_tree_nursery_t : public base_parse_tree_nursery_t {
    axe_t& seed_expr_axe;

    seed_parse_tree_nursery_t(tokenization_ptr_t tp, const lexicon_t& lexicon, axe_t& seed_expr_axe)
      : base_parse_tree_nursery_t(tp, lexicon), seed_expr_axe(seed_expr_axe)
    {
    }

    expected_t<parse_tree_node_t> expr_parens()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_atom, lexicon.ti_paren_open);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, expr()));
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_atom, lexicon.ti_paren_close);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> atom()
    {
      auto ss                        = stake();
      const index_t orig_token_index = token_index;
      error_nursery_t error_nursery;
      {
        auto result = nonterminal();
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
        auto result = expr_parens();
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
                                               lexicon.name_id_wrap(lexicon.ni_atom)));
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
        SILVA_EXPECT(false, MAJOR, "unexpected rule {}", lexicon.name_id_wrap(rule_name));
      }
    }

    expected_t<parse_tree_node_t> qualifier()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_qualifier);
      SILVA_EXPECT_PARSE(lexicon.ni_qualifier, num_tokens_left() >= 1, "no tokens left");
      SILVA_EXPECT_PARSE(lexicon.ni_qualifier,
                         token_id_by() == lexicon.ti_alias ||
                             token_id_by() == lexicon.ti_no_whitespace,
                         "expected qualifier 'alias' or 'no_whitespace', got {}",
                         sfp->token_id_wrap(token_id_by()));
      token_index += 1;
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> expr()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE(lexicon.ni_expr, num_tokens_left() >= 1, "no more tokens in input");
      const auto dg = axe_t::parse_delegate_t::make<&seed_parse_tree_nursery_t::any_rule>(this);
      ss.add_proto_node(
          SILVA_EXPECT_PARSE_FWD(lexicon.ni_expr, seed_expr_axe.apply(*this, lexicon.ni_expr, dg)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> scope_impl()
    {
      auto ss_rule = stake();
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_scope, lexicon.ti_newline);
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_scope, lexicon.ti_indent);
      while (num_tokens_left() >= 1 && token_category_by() != lexicon.ti_dedent) {
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;
        {
          auto result = scope();
          if (result) {
            ss_rule.add_proto_node(*result);
            continue;
          }
          error_nursery.add_child_error(std::move(result).error());
        }
        {
          auto result = rule();
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
                                                 lexicon.name_id_wrap(lexicon.ni_scope)));
      }
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_scope, lexicon.ti_dedent);
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> scope()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_scope);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, nonterminal()));
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_scope, lexicon.ti_colon);
      ss_rule.add_proto_node(SILVA_EXPECT_FWD(scope_impl()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> language()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_language);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_language, lexicon.ti_language);
      SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_language, lexicon.ti_rule_name);
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_language, lexicon.ti_colon);
      ss_rule.add_proto_node(SILVA_EXPECT_FWD(scope_impl()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_rule);
      const index_t orig_token_index = token_index;
      if (num_tokens_left() >= 1 && token_id_by() == lexicon.ti_here) {
        token_index += 1;
      }
      else {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, nonterminal()));
      }
      SILVA_EXPECT_PARSE(lexicon.ni_rule, num_tokens_left() >= 2, "not enough tokens in input");
      SILVA_EXPECT_PARSE_TOKEN_ID(lexicon.ni_rule, lexicon.ti_equal);
      if (num_tokens_left() >= 1 && token_id_by() == lexicon.ti_axe) {
        token_index += 1;
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, axe()));
      }
      else {
        while (num_tokens_left() >= 1 &&
               (token_id_by() == lexicon.ti_alias || token_id_by() == lexicon.ti_no_whitespace)) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, qualifier()));
        }
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, expr()));
        SILVA_EXPECT_PARSE_TOKEN_CATEGORY(lexicon.ni_rule, lexicon.ti_newline);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> seed()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_seed);
      while (num_tokens_left() >= 1 && token_category_by() != lexicon.ti_dedent) {
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;
        {
          auto result = tokenizer();
          if (result) {
            ss_rule.add_proto_node(*result);
            continue;
          }
          error_nursery.add_child_error(std::move(result).error());
        }
        {
          auto result = language();
          if (result) {
            ss_rule.add_proto_node(*result);
            continue;
          }
          error_nursery.add_child_error(std::move(result).error());
        }
        {
          auto result = scope();
          if (result) {
            ss_rule.add_proto_node(*result);
            continue;
          }
          error_nursery.add_child_error(std::move(result).error());
        }
        {
          auto result = rule();
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
                                                 lexicon.name_id_wrap(lexicon.ni_seed)));
      }
      return ss_rule.commit();
    }
  };
}

namespace silva::seed {
  struct bootstrap_interpreter_t::impl_t {
    syntax_farm_ptr_t sfp;
    const lexicon_t& lexicon;
    axe_t seed_expr_axe;

    impl_t(syntax_farm_ptr_t sfp) : sfp(sfp), lexicon(sfp->get_lexicon<lexicon_t>())
    {
      seed_expr_axe = impl::make_bootstrap_seed_expr_axe(sfp, lexicon);
    }

    expected_t<parse_tree_ptr_t> parse(fragment_span_t fs)
    {
      tokenization_ptr_t tp = SILVA_EXPECT_FWD(tokenizer_farm.apply(fs, lexicon.ti_r_seed));
      impl::seed_parse_tree_nursery_t nursery(tp, lexicon, seed_expr_axe);
      SILVA_EXPECT_FWD(nursery.seed());
      SILVA_EXPECT(nursery.token_index == tp->size(),
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

  expected_t<parse_tree_ptr_t> bootstrap_interpreter_t::parse(fragment_span_t fs)
  {
    return impl->parse(std::move(fs));
  }
}
