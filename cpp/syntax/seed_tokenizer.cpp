#include "seed_tokenizer.hpp"

namespace silva::seed::impl {

  using enum fragment_category_t;

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
    const token_id_t ti_PARENTHESES            = *swp->token_id("PARENTHESES");
    const token_id_t ti_LANGUAGE               = *swp->token_id("LANGUAGE");

    const token_id_t ti_prefix  = *swp->token_id("/");
    const token_id_t ti_postfix = *swp->token_id("\\");
    const token_id_t ti_exact   = *swp->token_id("|");

    expected_t<tuple_t<fragment_category_t, case_mask_t>>
    fragment_category_from_token_id(const token_id_t ti)
    {
      if (ti == ti_WHITESPACE) {
        return {{WHITESPACE, case_mask_t::INVALID}};
      }
      else if (ti == ti_COMMENT) {
        return {{COMMENT, case_mask_t::INVALID}};
      }
      else if (ti == ti_NUMBER) {
        return {{NUMBER, case_mask_t::INVALID}};
      }
      else if (ti == ti_STRING) {
        return {{STRING, case_mask_t::INVALID}};
      }
      else if (ti == ti_INDENT) {
        return {{INDENT, case_mask_t::INVALID}};
      }
      else if (ti == ti_DEDENT) {
        return {{DEDENT, case_mask_t::INVALID}};
      }
      else if (ti == ti_NEWLINE) {
        return {{NEWLINE, case_mask_t::INVALID}};
      }
      else if (ti == ti_OPERATOR) {
        return {{OPERATOR, case_mask_t::INVALID}};
      }
      else if (ti == ti_IDENTIFIER) {
        return {{IDENTIFIER, case_mask_t::INVALID}};
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
      else if (ti == ti_PARENTHESES) {
        return {{PAREN_LEFT, case_mask_t::INVALID}};
      }
      else if (ti == ti_LANGUAGE) {
        return {{LANG_BEGIN, case_mask_t::INVALID}};
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
          const auto& str = swp->token_infos[tokens[t_idx + 1]].str;
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
  expected_t<tokenization_ptr_t> tokenizer_t::apply(syntax_ward_ptr_t,
                                                    const fragmentization_t&) const
  {
    return {};
  }

  expected_t<tokenizer_t>
  tokenizer_create(syntax_ward_ptr_t swp, const name_id_t tokenizer_name, parse_tree_span_t pts)
  {
    impl::tokenizer_create_nursery_t nursery(swp, tokenizer_name);
    SILVA_EXPECT_FWD(nursery.run(pts));
    return std::move(nursery.retval);
  }
}
