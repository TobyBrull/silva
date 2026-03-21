#include "seed_tokenizer.hpp"

using enum silva::fragment_category_t;

namespace silva::seed::impl {

  expected_t<case_mask_t> compute_case_mask(const string_view_t identifier)
  {
    using enum case_mask_t;
    if (identifier.empty()) {
      return EMPTY;
    }
    const auto [first_cp, next_idx] = SILVA_EXPECT_FWD(unicode::utf8_decode_one(identifier));

    // TODO START: unicode, logic

    // Return "false" for '-' and '_'
    const auto is_lower = [&](const unicode::codepoint_t cp) {
      return std::islower(cp);
    };
    const auto is_upper = [&](const unicode::codepoint_t cp) {
      return std::isupper(cp);
    };

    // TODO END: unicode, logic

    const auto is_lower_or_op = [&](const unicode::codepoint_t cp) {
      return (cp == U'-') || (cp == U'_') || is_lower(cp);
    };
    const auto is_upper_or_op = [&](const unicode::codepoint_t cp) {
      return (cp == U'-') || (cp == U'_') || is_upper(cp);
    };
    const bool has_hyphen     = identifier.contains('-');
    const bool has_underscore = identifier.contains('_');

    const bool are_all_lower       = SILVA_EXPECT_FWD(unicode::all_of(identifier, is_lower));
    const bool are_all_upper       = SILVA_EXPECT_FWD(unicode::all_of(identifier, is_upper));
    const bool are_all_lower_or_op = SILVA_EXPECT_FWD(unicode::all_of(identifier, is_lower_or_op));
    const bool are_all_upper_or_op = SILVA_EXPECT_FWD(unicode::all_of(identifier, is_upper_or_op));

    std::underlying_type_t<case_mask_t> retval = std::to_underlying(EMPTY);
    if (!has_underscore && are_all_lower_or_op) {
      retval |= std::to_underlying(SILVA_CASE);
    }
    if (!has_hyphen && are_all_lower_or_op) {
      retval |= std::to_underlying(SNAKE_CASE);
    }
    if (!has_hyphen && !has_underscore && is_lower(first_cp)) {
      retval |= std::to_underlying(CAMEL_CASE);
    }
    if (!has_hyphen && !has_underscore && is_upper(first_cp)) {
      retval |= std::to_underlying(PASCAL_CASE);
    }
    if (!has_hyphen && are_all_upper_or_op) {
      retval |= std::to_underlying(MACRO_CASE);
    }
    if (are_all_upper) {
      retval |= std::to_underlying(UPPER_CASE);
    }
    if (are_all_lower) {
      retval |= std::to_underlying(LOWER_CASE);
    }
    return static_cast<case_mask_t>(retval);
  };

  expected_t<bool> matcher_t::matches(index_t frag_idx, const fragmentization_t& fr) const
  {
    if (fr.fragments[frag_idx].category != category) {
      return false;
    }
    const string_view_t frag_text = fr.get_fragment_text(frag_idx);
    using enum case_mask_t;
    if (category == IDENTIFIER) {
      if (case_mask != ANY) {
        const case_mask_t frag_cm = SILVA_EXPECT_FWD(compute_case_mask(frag_text));
        if ((std::to_underlying(frag_cm) & std::to_underlying(case_mask)) == 0) {
          return false;
        }
      }
    }
    if (!prefix.empty() && !frag_text.starts_with(prefix)) {
      return false;
    }
    if (!postfix.empty() && !frag_text.ends_with(postfix)) {
      return false;
    }
    return true;
  }

  struct tokenizer_create_nursery_t {
    syntax_ward_ptr_t swp;
    name_id_t tokenizer_name = name_id_root;

    const name_id_t ni_seed        = swp->name_id_of("Seed");
    const name_id_t ni_nt          = swp->name_id_of(ni_seed, "Nonterminal");
    const name_id_t ni_tok         = swp->name_id_of(ni_seed, "Tokenizer");
    const name_id_t ni_inc_rule    = swp->name_id_of(ni_tok, "IncludeRule");
    const name_id_t ni_ign_rule    = swp->name_id_of(ni_tok, "IgnoreRule");
    const name_id_t ni_tok_rule    = swp->name_id_of(ni_tok, "TokenRule");
    const name_id_t ni_prefix_atom = swp->name_id_of(ni_tok, "PrefixAtom");
    const name_id_t ni_atom        = swp->name_id_of(ni_tok, "Atom");
    const name_id_t ni_list        = swp->name_id_of(ni_tok, "List");
    const name_id_t ni_matcher     = swp->name_id_of(ni_tok, "Matcher");

    const token_id_t ti_WHITESPACE             = *swp->token_id("WHITESPACE");
    const token_id_t ti_COMMENT                = *swp->token_id("COMMENT");
    const token_id_t ti_NUMBER                 = *swp->token_id("NUMBER");
    const token_id_t ti_STRING                 = *swp->token_id("STRING");
    const token_id_t ti_INDENT                 = *swp->token_id("INDENT");
    const token_id_t ti_DEDENT                 = *swp->token_id("DEDENT");
    const token_id_t ti_NEWLINE                = *swp->token_id("NEWLINE");
    const token_id_t ti_OPERATOR               = *swp->token_id("OPERATOR");
    const token_id_t ti_IDENTIFIER             = *swp->token_id("IDENTIFIER");
    const token_id_t ti_IDENTIFIER_SILVA_CASE  = *swp->token_id("IDENTIFIER_SILVA_CASE");
    const token_id_t ti_IDENTIFIER_SNAKE_CASE  = *swp->token_id("IDENTIFIER_SNAKE_CASE");
    const token_id_t ti_IDENTIFIER_CAMEL_CASE  = *swp->token_id("IDENTIFIER_CAMEL_CASE");
    const token_id_t ti_IDENTIFIER_PASCAL_CASE = *swp->token_id("IDENTIFIER_PASCAL_CASE");
    const token_id_t ti_IDENTIFIER_MACRO_CASE  = *swp->token_id("IDENTIFIER_MACRO_CASE");
    const token_id_t ti_IDENTIFIER_UPPER_CASE  = *swp->token_id("IDENTIFIER_UPPER_CASE");
    const token_id_t ti_IDENTIFIER_LOWER_CASE  = *swp->token_id("IDENTIFIER_LOWER_CASE");
    const token_id_t ti_LANGUAGE               = *swp->token_id("LANGUAGE");

    const token_id_t ti_prefix  = *swp->token_id("/");
    const token_id_t ti_postfix = *swp->token_id("\\");
    const token_id_t ti_exact   = *swp->token_id("|");

    expected_t<tuple_t<fragment_category_t, case_mask_t>>
    fragment_category_from_token_id(const token_id_t ti)
    {
      if (ti == ti_WHITESPACE) {
        return {{WHITESPACE, case_mask_t::EMPTY}};
      }
      else if (ti == ti_COMMENT) {
        return {{COMMENT, case_mask_t::EMPTY}};
      }
      else if (ti == ti_NUMBER) {
        return {{NUMBER, case_mask_t::EMPTY}};
      }
      else if (ti == ti_STRING) {
        return {{STRING, case_mask_t::EMPTY}};
      }
      else if (ti == ti_INDENT) {
        return {{INDENT, case_mask_t::EMPTY}};
      }
      else if (ti == ti_DEDENT) {
        return {{DEDENT, case_mask_t::EMPTY}};
      }
      else if (ti == ti_NEWLINE) {
        return {{NEWLINE, case_mask_t::EMPTY}};
      }
      else if (ti == ti_OPERATOR) {
        return {{OPERATOR, case_mask_t::EMPTY}};
      }
      else if (ti == ti_IDENTIFIER) {
        return {{IDENTIFIER, case_mask_t::ANY}};
      }
      else if (ti == ti_IDENTIFIER_SILVA_CASE) {
        return {{IDENTIFIER, case_mask_t::SILVA_CASE}};
      }
      else if (ti == ti_IDENTIFIER_SNAKE_CASE) {
        return {{IDENTIFIER, case_mask_t::SNAKE_CASE}};
      }
      else if (ti == ti_IDENTIFIER_CAMEL_CASE) {
        return {{IDENTIFIER, case_mask_t::CAMEL_CASE}};
      }
      else if (ti == ti_IDENTIFIER_PASCAL_CASE) {
        return {{IDENTIFIER, case_mask_t::PASCAL_CASE}};
      }
      else if (ti == ti_IDENTIFIER_MACRO_CASE) {
        return {{IDENTIFIER, case_mask_t::MACRO_CASE}};
      }
      else if (ti == ti_IDENTIFIER_UPPER_CASE) {
        return {{IDENTIFIER, case_mask_t::UPPER_CASE}};
      }
      else if (ti == ti_IDENTIFIER_LOWER_CASE) {
        return {{IDENTIFIER, case_mask_t::LOWER_CASE}};
      }
      else if (ti == ti_LANGUAGE) {
        return {{LANG_BEGIN, case_mask_t::EMPTY}};
      }
      else {
        SILVA_EXPECT(false, MINOR);
      }
    }

    expected_t<array_t<matcher_t>> atom(const parse_tree_span_t pts_atom)
    {
      array_t<matcher_t> matchers;
      if (pts_atom[0].num_children == 0) {
        const token_id_t tid       = pts_atom.first_token_id();
        const token_info_t& ti     = swp->token_infos[tid];
        const string_t atom_str    = SILVA_EXPECT_FWD(ti.contained_string());
        const string_t atom_str_nl = atom_str + "\n";
        const auto fp              = SILVA_EXPECT_FWD(fragmentize("", std::move(atom_str_nl)));
        const auto& frags          = fp->fragments;
        SILVA_EXPECT(frags.size() > 3, MINOR, "empty string not supported in tokenizer");
        SILVA_EXPECT(frags[0].category == LANG_BEGIN, MINOR);
        SILVA_EXPECT(frags[frags.size() - 1].category == LANG_END, MINOR);
        SILVA_EXPECT(frags[frags.size() - 2].category == NEWLINE, MINOR);
        for (index_t i = 1; i < frags.size() - 2; ++i) {
          const index_t frag_start = frags[i].location.byte_offset;
          const index_t frag_end   = frags[i + 1].location.byte_offset;
          const string_t exact     = atom_str_nl.substr(frag_start, frag_end - frag_start);
          matchers.push_back(matcher_t{
              .category = frags[i].category,
              .prefix   = exact,
              .postfix  = exact,
          });
        }
      }
      else {
        const auto [c1]               = SILVA_EXPECT_FWD(pts_atom.get_children<1>());
        const parse_tree_span_t pts_m = pts_atom.sub_tree_span_at(c1);
        SILVA_EXPECT(pts_m[0].rule_name == ni_matcher, BROKEN_SEED);
        const token_id_t frag_name = pts_m.first_token_id();
        const auto [cat, cm]       = SILVA_EXPECT_FWD(fragment_category_from_token_id(frag_name));

        matcher_t mm{
            .category  = cat,
            .case_mask = cm,
        };

        index_t t_idx       = pts_m[0].token_begin + 1;
        const index_t t_end = pts_m[0].token_end;
        const auto& tokens  = pts_m.tp->tokens;
        bool had_prefix     = false;
        bool had_postfix    = false;
        while (t_idx < t_end) {
          SILVA_EXPECT(t_idx + 1 < t_end, BROKEN_SEED);
          const string_t str =
              SILVA_EXPECT_FWD(swp->token_infos[tokens[t_idx + 1]].contained_string());
          if (tokens[t_idx] == ti_prefix) {
            SILVA_EXPECT(!had_prefix, BROKEN_SEED);
            mm.prefix  = str;
            had_prefix = true;
          }
          else if (tokens[t_idx] == ti_postfix) {
            SILVA_EXPECT(!had_postfix, BROKEN_SEED);
            mm.postfix  = str;
            had_postfix = true;
          }
          else if (tokens[t_idx] == ti_exact) {
            SILVA_EXPECT(!had_prefix, BROKEN_SEED);
            SILVA_EXPECT(!had_postfix, BROKEN_SEED);
            mm.prefix   = str;
            mm.postfix  = str;
            had_prefix  = true;
            had_postfix = true;
          }
          else {
            SILVA_EXPECT(false, BROKEN_SEED);
          }
          t_idx += 2;
        }
        matchers.push_back(std::move(mm));
      }
      return {std::move(matchers)};
    }

    expected_t<array_t<array_t<matcher_t>>> prefix_atom(const parse_tree_span_t pts_pa)
    {
      array_t<array_t<matcher_t>> alternatives;
      const auto [c1] = SILVA_EXPECT_FWD(pts_pa.get_children<1>());
      if (pts_pa[c1].rule_name == ni_atom) {
        auto matchers = SILVA_EXPECT_FWD(atom(pts_pa.sub_tree_span_at(c1)));
        alternatives.push_back(std::move(matchers));
      }
      else {
        SILVA_EXPECT(pts_pa[c1].rule_name == ni_list, BROKEN_SEED);
        const auto pts_list = pts_pa.sub_tree_span_at(c1);
        auto [it, end]      = pts_list.children_range();
        while (it != end) {
          const auto pts_atom_child = pts_list.sub_tree_span_at(it.pos);
          SILVA_EXPECT(pts_atom_child[0].rule_name == ni_atom, BROKEN_SEED);
          auto matchers = SILVA_EXPECT_FWD(atom(pts_atom_child));
          alternatives.push_back(std::move(matchers));
          ++it;
        }
      }
      return {std::move(alternatives)};
    }

    expected_t<array_t<impl::rule_t>> defn(const parse_tree_span_t pts_defn)
    {
      array_t<impl::rule_t> retval;
      array_t<matcher_t> repeat_matchers;
      retval.emplace_back();
      auto [it, end]           = pts_defn.children_range();
      index_t num_prefix_atoms = 0;
      while (it != end) {
        const auto pts_child = pts_defn.sub_tree_span_at(it.pos);
        if (pts_child[0].rule_name == ni_prefix_atom) {
          auto alternatives = SILVA_EXPECT_FWD(prefix_atom(pts_child));
          array_t<impl::rule_t> new_retval;
          for (auto& existing_rule: retval) {
            for (auto& alt: alternatives) {
              rule_t new_rule = existing_rule;
              new_rule.prefix_matchers.insert(new_rule.prefix_matchers.end(),
                                              alt.begin(),
                                              alt.end());
              new_retval.push_back(std::move(new_rule));
            }
          }
          retval = std::move(new_retval);
          num_prefix_atoms += 1;
        }
        else if (pts_child[0].rule_name == ni_atom) {
          array_t<matcher_t> matchers = SILVA_EXPECT_FWD(atom(pts_child));
          SILVA_EXPECT(matchers.size() == 1,
                       BROKEN_SEED,
                       "strings in repeat-matchers must fragmentize to exactly one fragment");
          repeat_matchers.push_back(std::move(matchers.front()));
        }
        else {
          SILVA_EXPECT(false, BROKEN_SEED);
        }
        ++it;
      }
      if (num_prefix_atoms == 0) {
        retval.clear();
        for (const auto& rm: repeat_matchers) {
          retval.push_back(rule_t{.prefix_matchers = {rm}});
        }
      }
      for (auto& rule: retval) {
        rule.repeat_matchers = repeat_matchers;
      }
      return {std::move(retval)};
    }

    expected_t<void> include_rule(const parse_tree_span_t pts_rule)
    {
      retval.rules.emplace_back();
      SILVA_EXPECT(pts_rule[0].num_children == 1, BROKEN_SEED);
      const token_id_t included_tokenzier_name = pts_rule.first_token_id();
      // TODO: include named tokenizer
      return {};
    }

    expected_t<void> ignore_rule(const parse_tree_span_t pts_rule)
    {
      const auto [c1] = SILVA_EXPECT_FWD(pts_rule.get_children<1>());
      auto new_rules  = SILVA_EXPECT_FWD(defn(pts_rule.sub_tree_span_at(c1)));
      retval.rules.insert(retval.rules.end(),
                          std::make_move_iterator(new_rules.begin()),
                          std::make_move_iterator(new_rules.end()));
      return {};
    }

    expected_t<void> token_rule(const parse_tree_span_t pts_rule)
    {
      const auto [c1, c2] = SILVA_EXPECT_FWD(pts_rule.get_children<2>());
      const token_id_t tn = pts_rule.sub_tree_span_at(c1).first_token_id();
      auto new_rules      = SILVA_EXPECT_FWD(defn(pts_rule.sub_tree_span_at(c2)));
      for (auto& new_rule: new_rules) {
        new_rule.token_name = tn;
      }
      retval.rules.insert(retval.rules.end(),
                          std::make_move_iterator(new_rules.begin()),
                          std::make_move_iterator(new_rules.end()));
      return {};
    }

    expected_t<void> run(const parse_tree_span_t pts_tokenizer)
    {
      auto [it, end] = pts_tokenizer.children_range();
      while (it != end) {
        const auto pts_rule = pts_tokenizer.sub_tree_span_at(it.pos);
        if (pts_rule[0].rule_name == ni_inc_rule) {
          SILVA_EXPECT_FWD(include_rule(pts_rule));
        }
        else if (pts_rule[0].rule_name == ni_ign_rule) {
          SILVA_EXPECT_FWD(ignore_rule(pts_rule));
        }
        else if (pts_rule[0].rule_name == ni_tok_rule) {
          SILVA_EXPECT_FWD(token_rule(pts_rule));
        }
        else {
          SILVA_EXPECT(false, BROKEN_SEED);
        }
        ++it;
      }
      return {};
    }

    tokenizer_t retval{
        .swp  = swp,
        .name = tokenizer_name,
    };
  };
}

namespace silva::seed {
  expected_t<tokenization_ptr_t> tokenizer_t::apply(syntax_ward_ptr_t swp,
                                                    const fragmentization_t& fr) const
  {
    auto retval      = std::make_unique<tokenization_t>();
    retval->swp      = swp;
    retval->filepath = fr.filepath;

    const index_t n = fr.fragments.size();
    SILVA_EXPECT(n >= 2, MINOR);
    SILVA_EXPECT(fr.fragments.front().category == LANG_BEGIN, MINOR);
    SILVA_EXPECT(fr.fragments.back().category == LANG_END, MINOR);

    index_t frag_idx       = 1;
    const index_t frag_end = n - 1;

    while (frag_idx < frag_end) {
      bool matched = false;
      for (const impl::rule_t& rule: rules) {
        index_t cursor = frag_idx;
        bool prefix_ok = true;
        for (const impl::matcher_t& pm: rule.prefix_matchers) {
          if (cursor >= frag_end || !SILVA_EXPECT_FWD(pm.matches(cursor, fr))) {
            prefix_ok = false;
            break;
          }
          cursor = SILVA_EXPECT_FWD(fr.advance(cursor));
        }
        if (!prefix_ok) {
          continue;
        }

        while (cursor < frag_end) {
          bool any_match = false;
          for (const impl::matcher_t& rm: rule.repeat_matchers) {
            if (SILVA_EXPECT_FWD(rm.matches(cursor, fr))) {
              cursor    = SILVA_EXPECT_FWD(fr.advance(cursor));
              any_match = true;
              break;
            }
          }
          if (!any_match) {
            break;
          }
        }

        if (rule.token_name != token_id_none) {
          const index_t token_text_start = fr.fragments[frag_idx].location.byte_offset;
          const index_t token_text_end =
              (cursor < n) ? fr.fragments[cursor].location.byte_offset : fr.source_code.size();
          const string_view_t token_text =
              string_view_t{fr.source_code}.substr(token_text_start,
                                                   token_text_end - token_text_start);

          const auto tid = SILVA_EXPECT_FWD(swp->token_id_new(token_text));
          retval->tokens.push_back(tid);
          retval->categories.push_back(rule.token_name);
          retval->locations.push_back(fr.fragments[frag_idx].location);
        }

        frag_idx = cursor;
        matched  = true;
        break;
      }
      SILVA_EXPECT(matched,
                   MINOR,
                   "no tokenizer rule matches at {}",
                   fr.fragments[frag_idx].location);
    }

    return swp->add(std::move(retval));
  }

  expected_t<tokenizer_t>
  tokenizer_create(syntax_ward_ptr_t swp, const name_id_t tokenizer_name, parse_tree_span_t pts)
  {
    impl::tokenizer_create_nursery_t nursery(swp, tokenizer_name);
    SILVA_EXPECT_FWD(nursery.run(pts));
    return std::move(nursery.retval);
  }
}
