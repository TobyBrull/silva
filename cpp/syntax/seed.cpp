#include "seed.hpp"

#include "fragmentization.hpp"
#include "seed_axe.hpp"

#include "canopy/expected.hpp"
#include "syntax/parse_tree_nursery.hpp"

namespace silva::seed::impl {
  using enum fragment_category_t;

  struct base_parse_tree_nursery_t : public parse_tree_nursery_t {
    const lexicon_t& lexicon;

    base_parse_tree_nursery_t(fragment_span_t fs, const lexicon_t& lexicon)
      : parse_tree_nursery_t(fs), lexicon(lexicon)
    {
    }

    void skip_off_side()
    {
      while (num_fragments_left() >= 1 &&
             (fragment_category_by() == SPACE || fragment_category_by() == LINEFEED ||
              fragment_category_by() == COMMENT || fragment_category_by() == WHITESPACE)) {
        fragment_index += 1;
      }
    }

    void skip() { skip_off_side(); }

    expected_t<token_t> literal_token(const fragmented_token_t ft)
    {
      skip();
      return literal_fragmented_token(ft);
    }

    expected_t<token_t> identifier()
    {
      skip();
      auto ts = token_stake(lexicon.ni_id);
      SILVA_EXPECT_PARSE(lexicon.ni_id,
                         is_fragment_category_id_start(fragment_category_by()),
                         "expected fragment with category ID_START; got {}",
                         fragment_category_by());
      fragment_index += 1;
      while (num_fragments_left() >= 1 &&
             is_fragment_category_id_continue(fragment_category_by())) {
        fragment_index += 1;
      }
      return ts.commit();
    }

    expected_t<token_t> identifier_snake_case()
    {
      skip();
      auto ts = token_stake(lexicon.ni_id_snake);
      SILVA_EXPECT_PARSE(lexicon.ni_id_snake,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER,
                         "expected fragment with category ID_LOWER; got {}",
                         fragment_category_by());
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
      skip();
      auto ts = token_stake(lexicon.ni_id_pascal);
      SILVA_EXPECT_PARSE(lexicon.ni_id_pascal,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_UPPER,
                         "expected fragment with category ID_UPPER; got {}",
                         fragment_category_by());
      fragment_index += 1;
      SILVA_EXPECT_PARSE(lexicon.ni_id_pascal,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_LOWER,
                         "expected fragment with category ID_LOWER; got {}",
                         fragment_category_by());
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
      skip();
      auto ts = token_stake(lexicon.ni_id_macro);
      SILVA_EXPECT_PARSE(lexicon.ni_id_macro,
                         num_fragments_left() >= 1 && fragment_category_by() == ID_UPPER,
                         "expected fragment with category ID_UPPER; got {}",
                         fragment_category_by());
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
      skip();
      auto ts = token_stake(lexicon.ni_string);
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_string, STRING);
      return ts.commit();
    }

    expected_t<token_t> number()
    {
      skip();
      auto ts = token_stake(lexicon.ni_number);
      SILVA_EXPECT_PARSE(lexicon.ni_number,
                         num_fragments_left() >= 1 && fragment_category_by() == DIGIT,
                         "expected fragment with category DIGIT; got {}",
                         fragment_category_by());
      fragment_index += 1;
      while (num_fragments_left() >= 1 &&
             (fragment_category_by() == DIGIT || fragment_category_by() == ID_LOWER ||
              fragment_category_by() == ID_UPPER ||
              fragment_unique_codepoint_or_zero_by() == U'.' ||
              fragment_unique_codepoint_or_zero_by() == U'\'' ||
              fragment_unique_codepoint_or_zero_by() == U'+' ||
              fragment_unique_codepoint_or_zero_by() == U'-')) {
        fragment_index += 1;
      }
      return ts.commit();
    }

    expected_t<token_t> frag_name()
    {
      skip();
      auto ts = token_stake(lexicon.ni_frag_name);
      ts.add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_frag_name, identifier_macro_case()));
      return ts.commit();
    }
    expected_t<token_t> rule_name()
    {
      skip();
      auto ts = token_stake(lexicon.ni_rule_name);
      ts.add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule_name, identifier_pascal_case()));
      return ts.commit();
    }
    expected_t<token_t> token_category_name()
    {
      skip();
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
      for (const auto& ft: {lexicon.ti_eps, lexicon.ti_eof}) {
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
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_axe_op);
      error_nursery_t error_nursery;
      {
        auto result = string();
        if (result) {
          add_token(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = literal_token(lexicon.ti_concat);
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
                                               lexicon.name_id_wrap(lexicon.ni_axe_op)));
    }

    expected_t<parse_tree_node_t> axe_op_type()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_axe_op_type);
      error_nursery_t error_nursery;
      for (const auto& ft: {lexicon.ti_prefix_n,
                            lexicon.ti_prefix,
                            lexicon.ti_infix_flat,
                            lexicon.ti_infix,
                            lexicon.ti_ternary,
                            lexicon.ti_postfix_n,
                            lexicon.ti_postfix}) {
        auto result = literal_token(ft);
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
                                               lexicon.name_id_wrap(lexicon.ni_axe_op_type)));
    }

    expected_t<parse_tree_node_t> axe_ops()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_ops);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_ops, axe_op_type()));
      {
        auto result = literal_token(lexicon.ti_right_arrow);
        if (result) {
          add_token(*result);
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_ops, nonterminal()));
        }
      }
      while (auto result = axe_op()) {
        ss_rule.add_proto_node(*result);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_assoc()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_axe_assoc);
      error_nursery_t error_nursery;
      for (const auto& ft: {lexicon.ti_ltr, lexicon.ti_rtl}) {
        auto result = literal_token(ft);
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
                                               lexicon.name_id_wrap(lexicon.ni_axe_assoc)));
    }

    expected_t<parse_tree_node_t> axe_level()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe_level);
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_level, rule_name()));
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_level, literal_token(lexicon.ti_equal)));
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
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe, nonterminal()));
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_axe, NEWLINE);
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_axe, INDENT);
      while (num_fragments_left() >= 1 && fragment_category_by() != DEDENT) {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe, axe_level()));
        SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_axe, NEWLINE);
      }
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_axe, DEDENT);
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
    impl::base_parse_tree_nursery_t nursery(axe_frag, lexicon);
    SILVA_EXPECT_ASSERT(nursery.init(nursery.lexicon.ni_axe, nursery.lexicon));
    SILVA_EXPECT_ASSERT(nursery.axe());
    parse_tree_ptr_t pt = SILVA_EXPECT_ASSERT(std::move(nursery).finish());
    auto retval         = SILVA_EXPECT_ASSERT(axe_create(sfp, lexicon.ni_expr, pt->span()));
    {
      hash_set_t<name_id_t> atom_set;
      atom_set.insert(lexicon.ni_atom);
      atom_set.insert(lexicon.ni_oper);
      SILVA_ASSERT(retval.compile(lexicon, atom_set));
    }
    return retval;
  }

  struct seed_parse_tree_nursery_t : public base_parse_tree_nursery_t {
    axe_t& seed_expr_axe;

    seed_parse_tree_nursery_t(fragment_span_t fs, const lexicon_t& lexicon, axe_t& seed_expr_axe)
      : base_parse_tree_nursery_t(fs, lexicon), seed_expr_axe(seed_expr_axe)
    {
    }

    expected_t<parse_tree_node_t> expr_parens()
    {
      auto ss = stake();
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, literal_token(lexicon.ti_paren_open)));
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, expr()));
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, literal_token(lexicon.ti_paren_close)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> atom()
    {
      auto ss                     = stake();
      const index_t orig_frag_idx = fragment_index;
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
                                               fragment_location_at(orig_frag_idx),
                                               lexicon.name_id_wrap(lexicon.ni_atom)));
    }

    expected_t<token_t> expr_oper()
    {
      skip();
      auto ts = token_stake(lexicon.ni_oper);
      for (const auto& ft: {lexicon.ti_not, lexicon.ti_but_then}) {
        auto result = literal_token(ft);
        if (result) {
          ts.add_token(*result);
          return ts.commit();
        }
      }
      SILVA_EXPECT_PARSE(lexicon.ni_oper,
                         num_fragments_left() >= 1 && fragment_category_by() == OPERATOR,
                         "expected 'not', 'but_then', or a fragment with category OPERATOR");
      fragment_index += 1;
      return ts.commit();
    }

    expected_t<parse_tree_node_t> any_rule(const name_id_t rule_name)
    {
      if (rule_name == lexicon.ni_atom) {
        return atom();
      }
      else if (rule_name == lexicon.ni_oper) {
        add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_oper, expr_oper()));
        return {};
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
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_qualifier);
      error_nursery_t error_nursery;
      for (const auto& ft: {lexicon.ti_alias, lexicon.ti_no_whitespace}) {
        auto result = literal_token(ft);
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
                                               lexicon.name_id_wrap(lexicon.ni_qualifier)));
    }

    expected_t<parse_tree_node_t> expr()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE(lexicon.ni_expr, num_fragments_left() >= 1, "no more fragments in input");
      const auto dg = axe_t::parse_delegate_t::make<&seed_parse_tree_nursery_t::any_rule>(this);
      ss.add_proto_node(
          SILVA_EXPECT_PARSE_FWD(lexicon.ni_expr, seed_expr_axe.apply(*this, lexicon.ni_expr, dg)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> scope_impl()
    {
      auto ss_rule = stake();
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_scope, NEWLINE);
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_scope, INDENT);
      while (num_fragments_left() >= 1 && fragment_category_by() != DEDENT) {
        const index_t orig_frag_idx = fragment_index;
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
                                                 fragment_location_at(orig_frag_idx),
                                                 lexicon.name_id_wrap(lexicon.ni_scope)));
      }
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_scope, DEDENT);
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> scope()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_scope);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, nonterminal()));
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, literal_token(lexicon.ti_colon)));
      ss_rule.add_proto_node(SILVA_EXPECT_FWD(scope_impl()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> language()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_language);
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_language, literal_token(lexicon.ti_language)));
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_language, rule_name()));
      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_language, literal_token(lexicon.ti_colon)));
      ss_rule.add_proto_node(SILVA_EXPECT_FWD(scope_impl()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_rule);

      {
        bool matched_here = false;
        {
          auto result = literal_token(lexicon.ti_here);
          if (result) {
            add_token(*result);
            matched_here = true;
          }
        }
        if (!matched_here) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, nonterminal()));
        }
      }

      add_token(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, literal_token(lexicon.ti_equal)));

      {
        bool matched_axe = false;
        {
          auto result = literal_token(lexicon.ti_axe);
          if (result) {
            add_token(*result);
            matched_axe = true;
          }
        }
        if (matched_axe) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, axe()));
        }
        else {
          while (auto qual = qualifier()) {
            ss_rule.add_proto_node(*qual);
          }
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, expr()));
          SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_rule, NEWLINE);
        }
      }

      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> seed()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_seed);
      while (num_fragments_left() >= 1 && fragment_category_by() != DEDENT) {
        const index_t orig_frag_idx = fragment_index;
        error_nursery_t error_nursery;
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
                                                 fragment_location_at(orig_frag_idx),
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
      SILVA_EXPECT(sfp == fs.fp->sfp, ASSERT);
      impl::seed_parse_tree_nursery_t nursery(fs, lexicon, seed_expr_axe);
      SILVA_EXPECT_ASSERT(nursery.init(nursery.lexicon.ni_axe, nursery.lexicon));
      SILVA_EXPECT_FWD(nursery.seed());
      nursery.skip();
      SILVA_EXPECT(nursery.fragment_index == fs.end,
                   MINOR,
                   "could not parse entire text; stopped at {}",
                   nursery.fragment_location_by());
      return std::move(nursery).finish();
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
