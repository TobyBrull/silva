#include "seed_tokenizer.hpp"

#include "seed.lexicon.hpp"

using enum silva::fragment_category_t;
using enum silva::seed::impl::case_mask_t;

namespace silva::seed::impl {
  expected_t<case_mask_t> compute_case_mask(const string_view_t identifier)
  {
    using enum case_mask_t;
    if (identifier.empty()) {
      return EMPTY;
    }
    const auto [first_cp, next_idx] = SILVA_EXPECT_FWD(unicode::utf8_decode_one(identifier));

    // Return "false" for '-' and '_'
    const auto is_lower = [&](const unicode::codepoint_t cp) {
      const codepoint_category_t cc = codepoint_category_table[cp];
      return cc == codepoint_category_t::XID_Lowercase;
    };
    const auto is_upper = [&](const unicode::codepoint_t cp) {
      const codepoint_category_t cc = codepoint_category_table[cp];
      return cc == codepoint_category_t::XID_Uppercase;
    };

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

  void pretty_write_impl(const matcher_t& matcher, byte_sink_t* bs)
  {
    silva::pretty_write(matcher.category, bs);
    bs->write_str(" ");
    silva::pretty_write(std::to_underlying(matcher.case_mask), bs);
    bs->write_str(" ");
    silva::pretty_write(matcher.prefix, bs);
    bs->write_str(" ");
    silva::pretty_write(matcher.postfix, bs);
  }
  void pretty_write_impl(const rule_t& rule, byte_sink_t* bs)
  {
    silva::pretty_write(rule.token_category_name, bs);
    bs->write_str(" = ");
    for (const auto& x: rule.prefix_matchers) {
      silva::pretty_write(x, bs);
    }
    bs->write_str(" ::: ");
    for (const auto& x: rule.repeat_matchers) {
      silva::pretty_write(x, bs);
    }
  }

  struct tokenizer_create_nursery_t {
    syntax_farm_ptr_t sfp;
    const lexicon_t& lexicon;
    token_id_t tokenizer_name = token_id_none;

    tokenizer_create_nursery_t(syntax_farm_ptr_t sfp,
                               const lexicon_t& lexicon,
                               const token_id_t tokenizer_name)
      : sfp(sfp), lexicon(lexicon), tokenizer_name(tokenizer_name)
    {
    }

    expected_t<tuple_t<fragment_category_t, case_mask_t>>
    fragment_category_from_token_id(const token_id_t ti)
    {
      if (ti == lexicon.ti_WHITESPACE) {
        return {{WHITESPACE, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_COMMENT) {
        return {{COMMENT, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_NUMBER) {
        return {{NUMBER, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_STRING) {
        return {{STRING, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_INDENT) {
        return {{INDENT, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_DEDENT) {
        return {{DEDENT, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_NEWLINE) {
        return {{NEWLINE, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_PARENTHESIS) {
        return {{PARENTHESIS, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_OPERATOR) {
        return {{OPERATOR, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_IDENTIFIER) {
        return {{IDENTIFIER, case_mask_t::ANY}};
      }
      else if (ti == lexicon.ti_IDENTIFIER_SILVA_CASE) {
        return {{IDENTIFIER, case_mask_t::SILVA_CASE}};
      }
      else if (ti == lexicon.ti_IDENTIFIER_SNAKE_CASE) {
        return {{IDENTIFIER, case_mask_t::SNAKE_CASE}};
      }
      else if (ti == lexicon.ti_IDENTIFIER_CAMEL_CASE) {
        return {{IDENTIFIER, case_mask_t::CAMEL_CASE}};
      }
      else if (ti == lexicon.ti_IDENTIFIER_PASCAL_CASE) {
        return {{IDENTIFIER, case_mask_t::PASCAL_CASE}};
      }
      else if (ti == lexicon.ti_IDENTIFIER_MACRO_CASE) {
        return {{IDENTIFIER, case_mask_t::MACRO_CASE}};
      }
      else if (ti == lexicon.ti_IDENTIFIER_UPPER_CASE) {
        return {{IDENTIFIER, case_mask_t::UPPER_CASE}};
      }
      else if (ti == lexicon.ti_IDENTIFIER_LOWER_CASE) {
        return {{IDENTIFIER, case_mask_t::LOWER_CASE}};
      }
      else {
        SILVA_EXPECT(false, MINOR);
      }
    }

    expected_t<array_t<matcher_t>> item(const parse_tree_span_t pts_item)
    {
      array_t<matcher_t> matchers;
      if (pts_item[0].num_children == 0) {
        const token_id_t tid       = pts_item.first_token_id();
        const token_info_t& ti     = sfp->token_infos[tid];
        const string_t item_str    = SILVA_EXPECT_FWD(ti.contained_string());
        const string_t item_str_nl = item_str + "\n";
        const auto fp     = SILVA_EXPECT_FWD(fragmentize_unique("", std::move(item_str_nl)));
        const auto& frags = fp->fragments;
        SILVA_EXPECT(frags.size() > 3, MINOR, "empty string not supported in tokenizer");
        SILVA_EXPECT(frags[0].category == LANG_BEGIN, MINOR);
        SILVA_EXPECT(frags[frags.size() - 1].category == LANG_END, MINOR);
        SILVA_EXPECT(frags[frags.size() - 2].category == NEWLINE, MINOR);
        for (index_t i = 1; i < frags.size() - 2; ++i) {
          const index_t frag_start = frags[i].location.byte_offset;
          const index_t frag_end   = frags[i + 1].location.byte_offset;
          const string_t exact     = item_str_nl.substr(frag_start, frag_end - frag_start);
          matchers.push_back(matcher_t{
              .category = frags[i].category,
              .prefix   = exact,
              .postfix  = exact,
          });
        }
      }
      else {
        const auto [c1]               = SILVA_EXPECT_FWD(pts_item.get_children<1>());
        const parse_tree_span_t pts_m = pts_item.sub_tree_span_at(c1);
        SILVA_EXPECT(pts_m[0].rule_name == lexicon.ni_tok_matcher, BROKEN_SEED);
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
              SILVA_EXPECT_FWD(sfp->token_infos[tokens[t_idx + 1]].contained_string());
          if (tokens[t_idx] == lexicon.ti_slash) {
            SILVA_EXPECT(!had_prefix, BROKEN_SEED);
            mm.prefix  = str;
            had_prefix = true;
          }
          else if (tokens[t_idx] == lexicon.ti_backslash) {
            SILVA_EXPECT(!had_postfix, BROKEN_SEED);
            mm.postfix  = str;
            had_postfix = true;
          }
          else if (tokens[t_idx] == lexicon.ti_pipe) {
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

    expected_t<array_t<array_t<matcher_t>>> prefix_item(const parse_tree_span_t pts_pa)
    {
      array_t<array_t<matcher_t>> alternatives;
      const auto [c1] = SILVA_EXPECT_FWD(pts_pa.get_children<1>());
      if (pts_pa[c1].rule_name == lexicon.ni_tok_item) {
        auto matchers = SILVA_EXPECT_FWD(item(pts_pa.sub_tree_span_at(c1)));
        alternatives.push_back(std::move(matchers));
      }
      else {
        SILVA_EXPECT(pts_pa[c1].rule_name == lexicon.ni_tok_list, BROKEN_SEED);
        const auto pts_list = pts_pa.sub_tree_span_at(c1);
        auto [it, end]      = pts_list.children_range();
        while (it != end) {
          const auto pts_item_child = pts_list.sub_tree_span_at(it.pos);
          SILVA_EXPECT(pts_item_child[0].rule_name == lexicon.ni_tok_item, BROKEN_SEED);
          auto matchers = SILVA_EXPECT_FWD(item(pts_item_child));
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
      index_t num_prefix_items = 0;
      while (it != end) {
        const auto pts_child = pts_defn.sub_tree_span_at(it.pos);
        if (pts_child[0].rule_name == lexicon.ni_tok_prefix_item) {
          auto alternatives = SILVA_EXPECT_FWD(prefix_item(pts_child));
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
          num_prefix_items += 1;
        }
        else if (pts_child[0].rule_name == lexicon.ni_tok_item) {
          array_t<matcher_t> matchers = SILVA_EXPECT_FWD(item(pts_child));
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
      if (num_prefix_items == 0) {
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
      const auto [c1]                    = SILVA_EXPECT_FWD(pts_rule.get_children<1>());
      const token_id_t included_tok_name = pts_rule.sub_tree_span_at(c1).first_token_id();
      retval.rules.push_back(rule_t{.token_category_name = included_tok_name});
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
      const auto [c1]      = SILVA_EXPECT_FWD(pts_rule.get_children<1>());
      const token_id_t tcn = pts_rule.first_token_id();
      auto new_rules       = SILVA_EXPECT_FWD(defn(pts_rule.sub_tree_span_at(c1)));
      for (auto& new_rule: new_rules) {
        new_rule.token_category_name = tcn;
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
        if (pts_rule[0].rule_name == lexicon.ni_tok_inc_rule) {
          SILVA_EXPECT_FWD(include_rule(pts_rule));
        }
        else if (pts_rule[0].rule_name == lexicon.ni_tok_ign_rule) {
          SILVA_EXPECT_FWD(ignore_rule(pts_rule));
        }
        else if (pts_rule[0].rule_name == lexicon.ni_tok_tok_rule) {
          SILVA_EXPECT_FWD(token_rule(pts_rule));
        }
        else {
          SILVA_EXPECT(false, BROKEN_SEED);
        }
        ++it;
      }
      return {};
    }

    tokenizer_t retval;
  };
}

namespace silva::seed {
  void pretty_write_impl(const tokenizer_t& tokenizer, byte_sink_t* bs)
  {
    bs->write_str("tokenizer [\n");
    for (const auto& rule: tokenizer.rules) {
      bs->write_str("  ");
      silva::pretty_write(rule, bs);
      bs->write_str("\n");
    }
    bs->write_str("]\n");
  }
  void pretty_write_impl(const tokenizer_farm_t& tf, byte_sink_t* bs)
  {
    for (const auto& [ti, tt]: tf.tokenizers) {
      silva::pretty_write(ti, bs);
      bs->write_str(" = ");
      silva::pretty_write(tt, bs);
      bs->write_str("\n");
    }
  }

  tokenizer_farm_t::tokenizer_farm_t(syntax_farm_ptr_t sfp) : sfp(sfp) {}

  expected_t<void> tokenizer_farm_t::add(const token_id_t tokenizer_name, parse_tree_span_t pts)
  {
    SILVA_EXPECT(pts.tp->sfp == sfp, MAJOR);
    SILVA_EXPECT(!tokenizers.contains(tokenizer_name), MINOR);
    lexicon_t lexicon(sfp);
    impl::tokenizer_create_nursery_t nursery(sfp, lexicon, tokenizer_name);
    SILVA_EXPECT_FWD(nursery.run(pts));
    const auto [it, inserted] = tokenizers.emplace(tokenizer_name, std::move(nursery.retval));
    SILVA_EXPECT(inserted, ASSERT);
    return {};
  }

  expected_t<void> tokenizer_farm_t::cache_tokenizer(const token_id_t tokenizer_name)
  {
    if (cached_tokenizers.contains(tokenizer_name)) {
      return {};
    }

    array_t<impl::rule_t> rules;
    const auto insert_rules = [&](this const auto& self,
                                  const token_id_t tokenizer_name) -> expected_t<void> {
      const auto it = tokenizers.find(tokenizer_name);
      SILVA_EXPECT(it != tokenizers.end(),
                   MINOR,
                   "could not find tokenizer {}",
                   sfp->token_id_wrap(tokenizer_name));
      const auto& tt = it->second;
      for (const auto& rule: tt.rules) {
        if (rule.prefix_matchers.empty()) {
          SILVA_EXPECT_FWD(self(rule.token_category_name),
                           "when processing rules of tokenizer {}",
                           sfp->token_id_wrap(tokenizer_name));
        }
        else {
          rules.push_back(rule);
        }
      }
      return {};
    };
    SILVA_EXPECT_FWD(insert_rules(tokenizer_name));

    const auto [it, inserted] = cached_tokenizers.emplace(tokenizer_name,
                                                          tokenizer_t{
                                                              .rules = std::move(rules),
                                                          });
    SILVA_EXPECT(inserted, ASSERT);
    return {};
  }

  expected_t<tokenization_ptr_t> tokenizer_farm_t::apply(fragment_span_t fs,
                                                         const token_id_t tokenizer_name)
  {
    fragmentization_ptr_t fp = fs.fp;
    SILVA_EXPECT(fp->sfp == sfp, MAJOR);

    SILVA_EXPECT_FWD(cache_tokenizer(tokenizer_name));
    const auto& rules = cached_tokenizers.at(tokenizer_name).rules;

    auto retval      = std::make_unique<tokenization_t>();
    retval->sfp      = sfp;
    retval->filepath = fp->filepath;
    retval->fs       = fs;

    const index_t n = fp->fragments.size();
    SILVA_EXPECT(n >= 2, MINOR);
    SILVA_EXPECT(fp->fragments.front().category == LANG_BEGIN, MINOR);
    SILVA_EXPECT(fp->fragments.back().category == LANG_END, MINOR);

    index_t frag_idx       = 1;
    const index_t frag_end = n - 1;

    while (frag_idx < frag_end) {
      if (fp->fragments[frag_idx].category == LANG_BEGIN) {
        const index_t old_frag_idx = frag_idx;

        frag_idx = SILVA_EXPECT_FWD(fp->advance_language(frag_idx));

        const index_t language_idx = retval->languages.size();
        const index_t token_idx    = retval->size();
        retval->languages.emplace(token_idx, fragment_span_t{fp, old_frag_idx, frag_idx});
        retval->tokens.push_back(token_id_language);
        retval->categories.push_back(token_id_language);
        retval->locations.push_back(fp->fragments[old_frag_idx].location);
        continue;
      }

      bool matched = false;
      for (const impl::rule_t& rule: rules) {
        index_t cursor = frag_idx;

        bool prefix_ok = true;
        for (const impl::matcher_t& pm: rule.prefix_matchers) {
          if (cursor >= frag_end || !SILVA_EXPECT_FWD(pm.matches(cursor, *fp))) {
            prefix_ok = false;
            break;
          }
          ++cursor;
        }
        if (!prefix_ok) {
          continue;
        }

        while (cursor < frag_end) {
          bool any_match = false;
          for (const impl::matcher_t& rm: rule.repeat_matchers) {
            if (SILVA_EXPECT_FWD(rm.matches(cursor, *fp))) {
              ++cursor;
              any_match = true;
              break;
            }
          }
          if (!any_match) {
            break;
          }
        }

        if (rule.token_category_name != token_id_none) {
          const index_t token_text_start = fp->fragments[frag_idx].location.byte_offset;
          const index_t token_text_end =
              (cursor < n) ? fp->fragments[cursor].location.byte_offset : fp->source_code.size();
          const string_view_t token_text =
              string_view_t{fp->source_code}.substr(token_text_start,
                                                    token_text_end - token_text_start);

          const auto tid = sfp->token_id(token_text);
          retval->tokens.push_back(tid);
          retval->categories.push_back(rule.token_category_name);
          retval->locations.push_back(fp->fragments[frag_idx].location);
        }

        frag_idx = cursor;
        matched  = true;
        break;
      }
      SILVA_EXPECT(matched,
                   MINOR,
                   "no tokenizer rule matches at {}",
                   fp->fragments[frag_idx].location);
    }

    return sfp->add(std::move(retval));
  }

  expected_t<tokenization_ptr_t>
  tokenizer_farm_t::apply_text(filepath_t filepath, string_t text, const token_id_t tokenizer_name)
  {
    auto fp = SILVA_EXPECT_FWD(fragmentize(sfp, std::move(filepath), std::move(text)));
    return apply(fp, tokenizer_name);
  }

  tokenizer_farm_t make_bootstrap_tokenizer_farm(syntax_farm_ptr_t sfp)
  {
    using impl::matcher_t;
    using impl::rule_t;

    lexicon_t lexicon(sfp);
    tokenizer_farm_t retval(sfp);

    const token_id_t ti_ignore = token_id_none;

    const matcher_t m_whitespace = {.category = WHITESPACE};
    const matcher_t m_comment    = {.category = COMMENT};
    const matcher_t m_indent     = {.category = INDENT};
    const matcher_t m_dedent     = {.category = DEDENT};
    const matcher_t m_newline    = {.category = NEWLINE};
    const matcher_t m_number     = {.category = NUMBER};
    const matcher_t m_string     = {.category = STRING};
    const matcher_t m_operator   = {.category = OPERATOR};
    const matcher_t m_paren      = {.category = PARENTHESIS};
    const matcher_t m_id         = {.category = IDENTIFIER};
    const matcher_t m_id_pascal  = {.category = IDENTIFIER, .case_mask = PASCAL_CASE};
    const matcher_t m_id_snake   = {.category = IDENTIFIER, .case_mask = SNAKE_CASE};
    const matcher_t m_id_macro   = {.category = IDENTIFIER, .case_mask = MACRO_CASE};
    const matcher_t m_concat   = {.category = IDENTIFIER, .prefix = "concat", .postfix = "concat"};
    const matcher_t m_but_then = {.category = IDENTIFIER,
                                  .prefix   = "but_then",
                                  .postfix  = "but_then"};
    const matcher_t m_x        = {.category = IDENTIFIER, .prefix = "x", .postfix = "x"};
    const matcher_t m_p        = {.category = IDENTIFIER, .prefix = "p", .postfix = "p"};
    const matcher_t m_uscore   = {.category = IDENTIFIER, .prefix = "_", .postfix = "_"};

    {
      tokenizer_t tok;
      tok.rules = {
          rule_t{.token_category_name = ti_ignore, .prefix_matchers = {m_whitespace}},
          rule_t{.token_category_name = ti_ignore, .prefix_matchers = {m_comment}},
          rule_t{.token_category_name = lexicon.ti_number, .prefix_matchers = {m_number}},
          rule_t{.token_category_name = lexicon.ti_string, .prefix_matchers = {m_string}},
          rule_t{.token_category_name = lexicon.ti_operator, .prefix_matchers = {m_paren}},
          rule_t{
              .token_category_name = lexicon.ti_operator,
              .prefix_matchers     = {m_operator},
              .repeat_matchers     = {m_operator},
          },
          rule_t{.token_category_name = lexicon.ti_identifier, .prefix_matchers = {m_id}},
      };
      retval.tokenizers.emplace(lexicon.ti_r_defaults, std::move(tok));
    }
    {
      tokenizer_t tok;
      tok.rules = {
          rule_t{.token_category_name = lexicon.ti_indent, .prefix_matchers = {m_indent}},
          rule_t{.token_category_name = lexicon.ti_dedent, .prefix_matchers = {m_dedent}},
          rule_t{.token_category_name = lexicon.ti_newline, .prefix_matchers = {m_newline}},
          rule_t{.token_category_name = lexicon.ti_r_defaults},
      };
      retval.tokenizers.emplace(lexicon.ti_r_offside, std::move(tok));
    }
    {
      tokenizer_t tok;
      tok.rules = {
          rule_t{.token_category_name = ti_ignore, .prefix_matchers = {m_indent}},
          rule_t{.token_category_name = ti_ignore, .prefix_matchers = {m_dedent}},
          rule_t{.token_category_name = ti_ignore, .prefix_matchers = {m_newline}},
          rule_t{.token_category_name = lexicon.ti_r_defaults},
      };
      retval.tokenizers.emplace(lexicon.ti_r_freeform, std::move(tok));
    }
    {
      tokenizer_t tok;
      tok.rules = {
          rule_t{.token_category_name = lexicon.ti_frag_name, .prefix_matchers = {m_id_macro}},
          rule_t{.token_category_name = lexicon.ti_rule_name, .prefix_matchers = {m_id_pascal}},
          rule_t{.token_category_name = lexicon.ti_func_name,
                 .prefix_matchers     = {{.category  = IDENTIFIER,
                                          .case_mask = SNAKE_CASE,
                                          .postfix   = "_f"}}},
          rule_t{.token_category_name = lexicon.ti_token_cat_name, .prefix_matchers = {m_id_snake}},
          rule_t{.token_category_name = lexicon.ti_r_freeform},
      };
      retval.tokenizers.emplace(lexicon.ti_r_seed, std::move(tok));
    }
    {
      tokenizer_t tok;
      tok.rules = {
          rule_t{.token_category_name = lexicon.ti_r_freeform},
      };
      retval.tokenizers.emplace(lexicon.ti_r_fern, std::move(tok));
    }

    return {std::move(retval)};
  }
}
