#include "seed_tokenizer.hpp"

namespace silva::seed::impl {
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

    expected_t<matcher_t> prefix_atom(const parse_tree_span_t pts_pa)
    {
      ;
      return {};
    }

    expected_t<matcher_t> atom(const parse_tree_span_t pts_atom)
    {
      ;
      return {};
    }

    expected_t<array_t<impl::rule_t>> defn(const parse_tree_span_t pts_defn)
    {
      array_t<impl::rule_t> retval;
      const index_t n_prefix = pts_defn.count_children_with(ni_prefix_atom);
      if (n_prefix == 0) {
        retval.emplace_back();
      }
      else {
        retval.emplace_back();
        auto [it, end] = pts_defn.children_range();
        while (it != end) {
          const auto pts_aa = pts_defn.sub_tree_span_at(it.pos);
          if (pts_aa[0].rule_name == ni_prefix_atom) {
          }
          else if (pts_aa[0].rule_name == ni_atom) {
          }
          else {
            SILVA_EXPECT(false, BROKEN_SEED);
          }
          ++it;
        }
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
