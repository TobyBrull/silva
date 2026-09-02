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

    expected_t<void> skip()
    {
      skip_off_side();
      return {};
    }

    expected_t<parse_tree_node_t> identifier_camel_case()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE(lexicon.ni_id_camel,
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
      SILVA_EXPECT(num_fragments_left() == 0 ||
                       !is_fragment_category_id_continue(fragment_category_by()),
                   MINOR);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> identifier_pascal_case()
    {
      auto ss = stake();
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
      SILVA_EXPECT(num_fragments_left() == 0 ||
                       !is_fragment_category_id_continue(fragment_category_by()),
                   MINOR);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> identifier_macro_case()
    {
      auto ss = stake();
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
      SILVA_EXPECT(num_fragments_left() == 0 ||
                       !is_fragment_category_id_continue(fragment_category_by()),
                   MINOR);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> string()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_string, true);
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_string, STRING);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> number_uint_dec()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_num_uint_dec, true);
      SILVA_EXPECT_PARSE(lexicon.ni_number,
                         num_fragments_left() >= 1 && fragment_category_by() == DIGIT,
                         "expected fragment with category DIGIT; got {}",
                         fragment_category_by());
      fragment_index += 1;
      while (num_fragments_left() >= 1 && fragment_category_by() == DIGIT) {
        fragment_index += 1;
      }
      return ss.commit();
    }

    expected_t<parse_tree_node_t> number_plus_minus()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_num_pm, true);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> number_integer_decimal()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_num_int_dec, true);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_num_int_dec, number_plus_minus()));
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_num_int_dec, number_uint_dec()));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> number_integer()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_num_integer, true);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_num_integer, number_integer_decimal()));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> number()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_number, true);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_number, number_integer()));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> newline()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_newline, NEWLINE);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> indent()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_indent, INDENT);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> dedent()
    {
      auto ss = stake();
      SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(lexicon.ni_dedent, DEDENT);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> frag_name()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_frag_name, true);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_frag_name, identifier_macro_case()));
      return ss.commit();
    }
    expected_t<parse_tree_node_t> rule_name()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_rule_name, true);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule_name, identifier_pascal_case()));
      return ss.commit();
    }
    expected_t<parse_tree_node_t> token_category_name()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_token_cat_name, true);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_token_cat_name, identifier_camel_case()));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> literal(const fragmented_token_t& ft)
    {
      auto ss = stake();
      ss.add_proto_node(SILVA_EXPECT_FWD(parse_literal(ft)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> literal_node(const fragmented_token_t& ft)
    {
      auto ss = stake();
      ss.add_proto_node(SILVA_EXPECT_FWD(parse_literal(ft)));
      ss.create_node(name_id_literal, true);
      return ss.commit();
    }

    expected_t<parse_tree_node_t> keyword()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_keyword, true);
      error_nursery_t error_nursery;
      for (const auto& ft: {lexicon.ti_eps, lexicon.ti_language, lexicon.ti_commit}) {
        auto result = parse_literal(ft);
        if (result) {
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      return std::unexpected(std::move(error_nursery)
                                 .finish_short(error_level_t::MINOR,
                                               "[{}] {}",
                                               fragment_location_at(orig_frag_idx),
                                               lexicon.name_id_wrap(lexicon.ni_keyword)));
    }

    expected_t<parse_tree_node_t> terminal()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_term, false);
      error_nursery_t error_nursery;
      {
        auto result = literal(lexicon.ti_literals_of);
        if (result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_term, nonterminal()));
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = keyword();
        if (result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = string();
        if (result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = frag_name();
        if (result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
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

    expected_t<parse_tree_node_t> name()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      error_nursery_t error_nursery;
      {
        auto result = rule_name();
        if (result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = token_category_name();
        if (result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      return std::unexpected(std::move(error_nursery)
                                 .finish_short(error_level_t::MINOR,
                                               "[{}] {}",
                                               fragment_location_at(orig_frag_idx),
                                               lexicon.name_id_wrap(lexicon.ni_name)));
    }

    expected_t<parse_tree_node_t> nonterminal()
    {
      auto ss_rule = stake();

      ss_rule.create_node(lexicon.ni_nt, false);
      {
        if (auto result = literal_node(lexicon.ti_dot); result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
        }
      }

      while (true) {
        auto ss_local = stake();
        {
          auto result = name();
          if (!result) {
            break;
          }
          ss_local.add_proto_node(*result);
        }
        {
          auto result = literal_node(lexicon.ti_dot);
          if (!result) {
            break;
          }
          SILVA_EXPECT_FWD(skip());
          ss_local.add_proto_node(*result);
        }
        ss_rule.add_proto_node(ss_local.commit());
      }

      {
        auto result = name();
        if (result) {
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        return std::unexpected(std::move(result).error());
      }
    }

    expected_t<parse_tree_node_t> axe_op()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_axe_op, true);
      error_nursery_t error_nursery;
      {
        auto result = string();
        if (result) {
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = parse_literal(lexicon.ti_concat);
        if (result) {
          ss_rule.add_proto_node(*result);
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
      ss_rule.create_node(lexicon.ni_axe_op_type, true);
      error_nursery_t error_nursery;
      for (const auto& ft: {lexicon.ti_prefix_n,
                            lexicon.ti_prefix,
                            lexicon.ti_infix_flat,
                            lexicon.ti_infix,
                            lexicon.ti_ternary,
                            lexicon.ti_postfix_n,
                            lexicon.ti_postfix}) {
        auto result = parse_literal(ft);
        if (result) {
          ss_rule.add_proto_node(*result);
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
      ss_rule.create_node(lexicon.ni_axe_ops, false);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_ops, axe_op_type()));
      SILVA_EXPECT_FWD(skip());
      {
        if (auto result = literal(lexicon.ti_right_arrow); result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_ops, nonterminal()));
        }
      }
      while (auto result = axe_op()) {
        SILVA_EXPECT_FWD(skip());
        ss_rule.add_proto_node(*result);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe_assoc()
    {
      auto ss_rule                = stake();
      const index_t orig_frag_idx = fragment_index;
      ss_rule.create_node(lexicon.ni_axe_assoc, true);
      error_nursery_t error_nursery;
      for (const auto& ft: {lexicon.ti_ltr, lexicon.ti_rtl}) {
        auto result = parse_literal(ft);
        if (result) {
          ss_rule.add_proto_node(*result);
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
      ss_rule.create_node(lexicon.ni_axe_level, false);
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_level, rule_name()));
        SILVA_EXPECT_FWD(skip());
      }
      {
        ss_rule.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_level, literal(lexicon.ti_equal)));
        SILVA_EXPECT_FWD(skip());
      }
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe_level, axe_assoc()));
        SILVA_EXPECT_FWD(skip());
      }
      while (auto result = axe_ops()) {
        ss_rule.add_proto_node(*result);
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> axe()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_axe, false);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe, nonterminal()));
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, newline()));
        SILVA_EXPECT_FWD(skip());
      }
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, indent()));
        SILVA_EXPECT_FWD(skip());
      }
      while (num_fragments_left() >= 1 && fragment_category_by() != DEDENT) {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_axe, axe_level()));
        {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, newline()));
          SILVA_EXPECT_FWD(skip());
        }
      }
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, dedent()));
        SILVA_EXPECT_FWD(skip());
      }
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
    const auto axe_text = find_subsection(seed_str, "= axe ", "    Atom = no_node");
    const auto axe_frag = SILVA_EXPECT_ASSERT(fragmentize(sfp, "seed.axe", string_t{axe_text}));
    impl::base_parse_tree_nursery_t nursery(axe_frag, lexicon);
    SILVA_EXPECT_ASSERT(nursery.init(nursery.lexicon.ni_axe, nursery.lexicon));
    SILVA_EXPECT_ASSERT(nursery.axe());
    parse_tree_ptr_t pt = SILVA_EXPECT_ASSERT(std::move(nursery).finish());
    auto retval         = SILVA_EXPECT_ASSERT(axe_create(sfp, lexicon.ni_expr, pt->span()));
    {
      hash_set_t<name_id_t> atom_set;
      atom_set.insert(lexicon.ni_atom);
      atom_set.insert(lexicon.ni_quantifier);
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
      {
        ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, literal(lexicon.ti_paren_open)));
        SILVA_EXPECT_FWD(skip());
      }
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, expr()));
      {
        ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, literal(lexicon.ti_paren_close)));
        SILVA_EXPECT_FWD(skip());
      }
      return ss.commit();
    }

    expected_t<parse_tree_node_t> alternation()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_alternation, false);
      {
        ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, literal(lexicon.ti_brack_open)));
        SILVA_EXPECT_FWD(skip());
      }
      index_t count = 0;
      while (true) {
        {
          if (auto result = terminal(); result) {
            ss.add_proto_node(*result);
            count += 1;
            continue;
          }
        }
        {
          if (auto result = nonterminal(); result) {
            ss.add_proto_node(*result);
            count += 1;
            continue;
          }
        }
        break;
      }
      SILVA_EXPECT_PARSE(lexicon.ni_atom,
                         count >= 1,
                         "expected at least one Terminal or Nonterminal inside '[' ']'");
      {
        ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_atom, literal(lexicon.ti_brack_close)));
        SILVA_EXPECT_FWD(skip());
      }
      return ss.commit();
    }

    expected_t<parse_tree_node_t> atom()
    {
      auto ss                     = stake();
      const index_t orig_frag_idx = fragment_index;
      error_nursery_t error_nursery;
      {
        auto result = terminal();
        if (result) {
          ss.add_proto_node(*result);
          return ss.commit();
        }
        error_nursery.add_child_error(std::move(result).error());
      }
      {
        auto result = nonterminal();
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
      {
        auto result = alternation();
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

    expected_t<parse_tree_node_t> quantifier()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_quantifier, false);
      {
        ss_rule.add_proto_node(SILVA_EXPECT_FWD(number()));
        SILVA_EXPECT_FWD(skip());
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> any_rule(const name_id_t rule_name)
    {
      if (rule_name == lexicon.ni_atom) {
        return atom();
      }
      else if (rule_name == lexicon.ni_quantifier) {
        return quantifier();
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
      ss_rule.create_node(lexicon.ni_qualifier, true);
      error_nursery_t error_nursery;
      for (const auto& ft: {
               lexicon.ti_no_node,
               lexicon.ti_no_whitespace,
               lexicon.ti_lit_nodes,
           }) {
        auto result = parse_literal(ft);
        if (result) {
          ss_rule.add_proto_node(*result);
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
      const auto skip_dg = axe_t::skip_delegate_t::make<&base_parse_tree_nursery_t::skip>(
          static_cast<base_parse_tree_nursery_t*>(this));
      ss.add_proto_node(
          SILVA_EXPECT_PARSE_FWD(lexicon.ni_expr,
                                 seed_expr_axe.apply(*this, lexicon.ni_expr, false, dg, skip_dg)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> scope_impl()
    {
      auto ss_rule = stake();
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, newline()));
        SILVA_EXPECT_FWD(skip());
      }
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, indent()));
        SILVA_EXPECT_FWD(skip());
      }
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
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, dedent()));
        SILVA_EXPECT_FWD(skip());
      }
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> scope()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_scope, false);
      ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, nonterminal()));
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_scope, literal(lexicon.ti_colon)));
        SILVA_EXPECT_FWD(skip());
      }
      ss_rule.add_proto_node(SILVA_EXPECT_FWD(scope_impl()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> language()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_language, false);
      {
        ss_rule.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(lexicon.ni_language, literal(lexicon.ti_language)));
        SILVA_EXPECT_FWD(skip());
      }
      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_language, rule_name()));
        SILVA_EXPECT_FWD(skip());
      }
      {
        ss_rule.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(lexicon.ni_language, literal(lexicon.ti_colon)));
        SILVA_EXPECT_FWD(skip());
      }
      ss_rule.add_proto_node(SILVA_EXPECT_FWD(scope_impl()));
      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> here()
    {
      auto ss = stake();
      ss.create_node(lexicon.ni_here, true);
      ss.add_proto_node(SILVA_EXPECT_FWD(parse_literal(lexicon.ti_here)));
      return ss.commit();
    }

    expected_t<parse_tree_node_t> rule()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_rule, false);

      {
        bool matched_here = false;
        if (auto result = here(); result) {
          SILVA_EXPECT_FWD(skip());
          ss_rule.add_proto_node(*result);
          matched_here = true;
        }
        if (!matched_here) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, nonterminal()));
        }
      }

      {
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, literal(lexicon.ti_equal)));
        SILVA_EXPECT_FWD(skip());
      }

      while (auto qual = qualifier()) {
        SILVA_EXPECT_FWD(skip());
        ss_rule.add_proto_node(*qual);
      }

      {
        bool matched_axe = false;
        {
          if (auto result = literal(lexicon.ti_axe); result) {
            SILVA_EXPECT_FWD(skip());
            ss_rule.add_proto_node(*result);
            matched_axe = true;
          }
        }
        if (matched_axe) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, axe()));
        }
        else {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, expr()));
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(lexicon.ni_rule, newline()));
          SILVA_EXPECT_FWD(skip());
        }
      }

      return ss_rule.commit();
    }

    expected_t<parse_tree_node_t> seed()
    {
      auto ss_rule = stake();
      ss_rule.create_node(lexicon.ni_seed, false);
      while (num_fragments_left() >= 1 && fragment_category_by() != LANG_END) {
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
      SILVA_EXPECT_FWD(nursery.skip());
      SILVA_EXPECT_FWD(nursery.seed());
      SILVA_EXPECT(nursery.fragment_index + 1 == fs.end,
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
