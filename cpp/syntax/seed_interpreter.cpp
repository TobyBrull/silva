#include "seed_interpreter.hpp"

#include "canopy/array_small.hpp"
#include "canopy/env_context.hpp"
#include "canopy/exec_trace.hpp"
#include "canopy/expected.hpp"
#include "canopy/scope_exit.hpp"
#include "parse_tree.hpp"
#include "parse_tree_nursery.hpp"
#include "seed.hpp"
#include "seed_axe.hpp"
#include "syntax/fragmentization.hpp"

#include <utility>

using enum silva::error_level_t;

namespace silva::seed::impl {
  bool is_string_terminal(const lexicon_t& lexicon, const parse_tree_span_t& pts)
  {
    return pts.rule_name() == lexicon.ni_term && pts.num_children() == 1 &&
        pts.node_at(1).rule_name == lexicon.ni_string;
  }

  bool is_commit_terminal(const lexicon_t& lexicon, const parse_tree_span_t& pts)
  {
    return pts.rule_name() == lexicon.ni_term && pts.num_children() == 1 &&
        pts.node_at(1).rule_name == lexicon.ni_keyword &&
        pts.subspan_at(1).token() == lexicon.ti_commit.token_id;
  }

  struct interpreter_adder_t {
    interpreter_t* se     = nullptr;
    syntax_farm_ptr_t sfp = se->sfp;
    const lexicon_t& lexicon;

    optional_t<token_id_t> current_language_id;

    interpreter_adder_t(interpreter_t* se) : se(se), lexicon(se->bootstrap_interpreter.lexicon()) {}

    expected_t<void> recognize_literal(name_id_t rule_name, const fragmented_token_t& ft)
    {
      while (rule_name.is_valid()) {
        se->scope_to_literals[rule_name].push_back(ft);
        rule_name = sfp->get(rule_name).parent_name;
      }
      return {};
    }

    expected_t<void> register_rule(const name_id_t rule_name,
                                   const parse_tree_span_t& pts,
                                   const bool is_twig_rule     = false,
                                   const bool is_no_node       = false,
                                   const bool is_no_whitespace = false,
                                   const bool is_literal_nodes = false)
    {
      const auto [emplace_it, inserted] = se->rule_exprs.emplace(
          rule_name,
          interpreter_t::rule_expr_data_t{.expr             = pts,
                                          .is_twig_rule     = is_twig_rule,
                                          .is_no_node       = is_no_node,
                                          .is_literal_nodes = is_literal_nodes});
      SILVA_EXPECT(inserted,
                   MINOR,
                   "{} rule {} defined again, previously defined at {}",
                   pts,
                   lexicon.name_id_wrap(rule_name),
                   emplace_it->second.expr);
      return {};
    }

    expected_t<void> handle_concat_expr(const parse_tree_span_t pts_concat)
    {
      SILVA_EXPECT(lexicon.ni_expr_concat.is_parent_of(pts_concat.rule_name(), *sfp), ASSERT);
      optional_t<index_t> commit_after;
      index_t leading_literals = 0;
      bool in_leading_literals = true;
      index_t sub_expr_index   = 0;
      for (const auto sub_pts: pts_concat.children_range()) {
        if (is_commit_terminal(lexicon, sub_pts)) {
          SILVA_EXPECT(!commit_after.has_value(),
                       MINOR,
                       "{} more than one \"commit\" keyword in concatenated expression",
                       pts_concat);
          commit_after = sub_expr_index;
          continue;
        }
        if (in_leading_literals) {
          if (is_string_terminal(lexicon, sub_pts)) {
            leading_literals += 1;
          }
          else {
            in_leading_literals = false;
          }
        }
        sub_expr_index += 1;
      }
      if (leading_literals >= 1) {
        if (!commit_after.has_value()) {
          commit_after = leading_literals;
        }
        else {
          commit_after = std::min(*commit_after, leading_literals);
        }
      }
      const auto [it, inserted] = se->expr_data.emplace(pts_concat,
                                                        interpreter_t::expr_data_t{
                                                            .commit_after = commit_after,
                                                        });
      SILVA_EXPECT(inserted, ASSERT, "{} concatenated expression handled twice", pts_concat);
      return {};
    }

    expected_t<void> handle_rule(const name_id_t scope_name,
                                 const parse_tree_span_t pts_rule,
                                 const bool scope_is_twig_rule)
    {
      SILVA_EXPECT(pts_rule.rule_name() == lexicon.ni_rule, MINOR, "expected Rule");

      auto [it, end] = pts_rule.children_range();
      SILVA_EXPECT(it != end, MINOR, "{} rule must have at least two children", pts_rule);

      name_id_t curr_rule_name;
      bool is_twig_rule = false;
      if ((*it).rule_name() == lexicon.ni_here) {
        curr_rule_name = scope_name;
        is_twig_rule   = scope_is_twig_rule;
      }
      else {
        const auto pts_nt = *it;
        curr_rule_name    = SILVA_EXPECT_FWD(name_id_definition(lexicon, scope_name, pts_nt));
        const auto back_name_pts = SILVA_EXPECT_FWD(pts_nt.iterate_to_child(-1));
        is_twig_rule             = (back_name_pts.rule_name() == lexicon.ni_token_cat_name);
      }
      ++it;
      SILVA_EXPECT(it != end, MINOR, "{} rule must have at least two children", pts_rule);

      bool is_no_node       = false;
      bool is_no_whitespace = false;
      bool is_literal_nodes = false;
      while (it != end && (*it).rule_name() == lexicon.ni_qualifier) {
        const auto pts_qual    = *it;
        const token_id_t q_tok = SILVA_EXPECT_FWD(pts_qual.token());
        if (q_tok == lexicon.ti_no_node.token_id) {
          is_no_node = true;
        }
        else if (q_tok == lexicon.ti_no_whitespace.token_id) {
          is_no_whitespace = true;
        }
        else if (q_tok == lexicon.ti_lit_nodes.token_id) {
          is_literal_nodes = true;
        }
        else {
          SILVA_EXPECT(false,
                       MINOR,
                       "{} unknown qualifier {}",
                       pts_qual,
                       sfp->token_id_wrap(q_tok));
        }
        ++it;
      }

      SILVA_EXPECT(it != end, MINOR, "{} rule must have right-hand side", pts_rule);
      const auto pts_rhs_0 = *it;

      ++it;
      SILVA_EXPECT(it == end, MINOR, "{} rule had too many children", pts_rule);
      SILVA_EXPECT_FWD(register_rule(curr_rule_name,
                                     pts_rhs_0,
                                     is_twig_rule,
                                     is_no_node,
                                     is_no_whitespace,
                                     is_literal_nodes));

      const name_info_t& ni = sfp->get(curr_rule_name);
      if (ni.base_name == lexicon.ti_skip.token_id) {
        SILVA_EXPECT(current_language_id.has_value(),
                     MINOR,
                     "'skip' rule may only be used in language");
        const name_info_t& parent_ni = sfp->get(ni.parent_name);
        SILVA_EXPECT(parent_ni.parent_name == name_id_t{},
                     MINOR,
                     "'skip' rule must not be nested in sub-scope of a language");
        SILVA_EXPECT(parent_ni.base_name == current_language_id.value(), ASSERT);
        se->languages.at(*current_language_id).skip_rule_expr =
            interpreter_t::rule_expr_data_t{.expr = pts_rhs_0, .is_twig_rule = true};
      }

      for (index_t i = 0; i < pts_rhs_0.subtree_size(); ++i) {
        const parse_tree_span_t pts_node = pts_rhs_0.subspan_at(i);
        if (is_string_terminal(lexicon, pts_node)) {
          const token_id_t str_ti = SILVA_EXPECT_FWD(pts_rhs_0.subspan_at(i + 1).token());
          auto ft                 = SILVA_EXPECT_FWD(fragment_token_from_string(sfp, str_ti));
          SILVA_EXPECT_FWD(recognize_literal(curr_rule_name, ft));
          se->string_to_ft[str_ti] = std::move(ft);
        }
        else if (lexicon.ni_expr_concat.is_parent_of(pts_node.rule_name(), *sfp)) {
          SILVA_EXPECT_FWD(handle_concat_expr(pts_node));
        }
      }

      const name_id_t expr_rule_name = pts_rhs_0.rule_name();
      if (expr_rule_name == lexicon.ni_axe) {
        se->axes[curr_rule_name] = SILVA_EXPECT_FWD(axe_create(sfp, curr_rule_name, pts_rhs_0));
        auto [axe_it, axe_end]   = pts_rhs_0.children_range();
        SILVA_EXPECT(axe_it != axe_end, MINOR);
        SILVA_EXPECT((*axe_it).rule_name() == lexicon.ni_nt, MINOR);
        ++axe_it;
        while (axe_it != axe_end) {
          SILVA_EXPECT((*axe_it).rule_name() == lexicon.ni_axe_level, MINOR);
          const auto pts_level          = *axe_it;
          const auto pts_rulename       = SILVA_EXPECT_FWD(pts_level.iterate_to_child(0));
          const token_id_t level_name   = SILVA_EXPECT_FWD(pts_rulename.token());
          const name_id_t axe_rule_name = sfp->name_id(curr_rule_name, level_name);
          SILVA_EXPECT_FWD(register_rule(axe_rule_name, pts_level));
          ++axe_it;
        }
      }

      return {};
    }

    template<typename Iter>
    expected_t<void> handle_scope_impl(const name_id_t scope_name,
                                       Iter it,
                                       const Iter end,
                                       const bool scope_is_twig_rule)
    {
      while (it != end) {
        const auto pts_child = *it;
        const name_id_t cn   = pts_child.rule_name();
        if (cn == lexicon.ni_scope) {
          SILVA_EXPECT_FWD(handle_scope(scope_name, pts_child));
        }
        else if (cn == lexicon.ni_rule) {
          SILVA_EXPECT_FWD(handle_rule(scope_name, pts_child, scope_is_twig_rule));
        }
        else {
          SILVA_EXPECT(false,
                       MINOR,
                       "expected Scope or Rule node; got {}",
                       lexicon.name_id_wrap(cn));
        }
        ++it;
      }
      return {};
    }

    expected_t<void> handle_scope(const name_id_t scope_name, const parse_tree_span_t pts_scope)
    {
      SILVA_EXPECT(pts_scope.rule_name() == lexicon.ni_scope, MINOR, "expected Scope");

      auto [it, end] = pts_scope.children_range();
      SILVA_EXPECT(it != end, MINOR, "{} scope must have at least one child", pts_scope);

      const auto pts_nt = *it;
      const name_id_t curr_scope_name =
          SILVA_EXPECT_FWD(name_id_definition(lexicon, scope_name, pts_nt));
      const auto back_name_pts      = SILVA_EXPECT_FWD(pts_nt.iterate_to_child(-1));
      const bool scope_is_twig_rule = (back_name_pts.rule_name() == lexicon.ni_token_cat_name);
      ++it;
      SILVA_EXPECT(it != end,
                   MINOR,
                   "{} scope must have at least one sub-rule or sub-scope",
                   pts_scope);
      SILVA_EXPECT_FWD(handle_scope_impl(curr_scope_name, it, end, scope_is_twig_rule));
      return {};
    }

    expected_t<void> handle_language(const name_id_t scope_name,
                                     const parse_tree_span_t pts_language)
    {
      SILVA_EXPECT(!current_language_id.has_value(),
                   MINOR,
                   "languages cannot be nested at {}",
                   pts_language);
      SILVA_EXPECT(pts_language.rule_name() == lexicon.ni_language, MINOR, "expected Language");
      const auto pts_rulename        = SILVA_EXPECT_FWD(pts_language.iterate_to_child(0));
      const token_id_t lang_id       = SILVA_EXPECT_FWD(pts_rulename.token());
      current_language_id            = lang_id;
      const auto [lang_it, inserted] = se->languages.emplace(lang_id,
                                                             interpreter_t::language_data_t{
                                                                 .pts = pts_language,
                                                             });
      SILVA_EXPECT(inserted,
                   MINOR,
                   "Language {} already defined at {}; defined again at {}",
                   sfp->token_id_wrap(lang_id),
                   lang_it->second.pts,
                   pts_language);
      auto [it, end] = pts_language.children_range();
      SILVA_EXPECT(it != end, MINOR, "expected child nodes at {}, got none", pts_language);
      SILVA_EXPECT((*it).rule_name() == lexicon.ni_rule_name,
                   MINOR,
                   "expected first child node at {}, to be ruleName",
                   pts_language);
      ++it;
      const name_id_t curr_scope_name = sfp->name_id(scope_name, lang_id);
      SILVA_EXPECT_FWD(handle_scope_impl(curr_scope_name, it, end, false));
      return {};
    }

    expected_t<void> handle_seed(const name_id_t scope_name, const parse_tree_span_t pts_seed)
    {
      SILVA_EXPECT(pts_seed.subtree_size() != 0 && pts_seed.rule_name() == lexicon.ni_seed,
                   MINOR,
                   "Seed parse_tree should start with Seed node");

      for (const auto pts_child: pts_seed.children_range()) {
        if (pts_child.rule_name() == lexicon.ni_language) {
          SILVA_EXPECT_FWD(handle_language(scope_name, pts_child));
        }
        else if (pts_child.rule_name() == lexicon.ni_scope) {
          SILVA_EXPECT_FWD(handle_scope(scope_name, pts_child));
        }
        else {
          SILVA_EXPECT_FWD(handle_rule(scope_name, pts_child, false));
        }
      }
      return {};
    }

    expected_t<void> handle_all(const parse_tree_span_t pts)
    {
      SILVA_EXPECT_FWD(handle_seed(name_id_t{}, pts));
      return {};
    }
  };

  struct seed_exec_trace_data_t {
    name_id_t rule_name;
    fragment_location_t frag_pos;
    bool success = false;
  };

  struct seed_exec_trace_t : public exec_trace_t<seed_exec_trace_data_t> {
    syntax_farm_ptr_t sfp;
    const lexicon_t& lexicon;

    expected_t<string_t> as_tree_to_string() &&
    {
      auto ett = SILVA_EXPECT_FWD(std::move(*this).as_tree());
      tree_span_t ets{ett};
      string_t retval = SILVA_EXPECT_FWD(ets.to_string([&](string_t& curr_line, const auto& path) {
        const auto& data = ets.node_at(path.back().node_index).item.data;
        curr_line += lexicon.name_id_str(data.rule_name);
        string_pad(curr_line, 85);
        curr_line += fmt::format("{}", data.success);
        string_pad(curr_line, 95);
        curr_line += pretty_string(data.frag_pos);
      }));
      return {std::move(retval)};
    }
  };

  struct interpreter_apply_nursery_t : public parse_tree_nursery_t {
    const interpreter_t* se = nullptr;
    syntax_farm_ptr_t sfp   = se->sfp;
    const lexicon_t& lexicon;

    const interpreter_t::language_data_t* lang_data = nullptr;

    int rule_depth = 0;

    int twig_rule_depth = 0;

    const interpreter_t::rule_expr_data_t* curr_rule = nullptr;
    struct rule_expr_data_scope_t {
      interpreter_apply_nursery_t& self;
      const interpreter_t::rule_expr_data_t* prev_value = nullptr;
      rule_expr_data_scope_t(interpreter_apply_nursery_t& self_,
                             const interpreter_t::rule_expr_data_t* new_value)
        : self(self_), prev_value(self_.curr_rule)
      {
        self.curr_rule = new_value;
      }
      ~rule_expr_data_scope_t() { self.curr_rule = prev_value; }
    };

    seed_exec_trace_t exec_trace{.sfp = sfp, .lexicon = lexicon};

    interpreter_apply_nursery_t(fragment_span_t fs,
                                const lexicon_t& lexicon,
                                const interpreter_t* root,
                                const interpreter_t::language_data_t* lang_data)
      : parse_tree_nursery_t(fs), lexicon(lexicon), se(root), lang_data(lang_data)
    {
    }

    expected_t<void> check()
    {
      SILVA_EXPECT(sfp == fp->sfp,
                   MAJOR,
                   "Seed and target parse-trees/fragmentizations must be in same syntax_farm_t");
      return {};
    }

    struct node_and_error_t {
      parse_tree_node_t node;
      error_t last_error;

      node_and_error_t() = default;
      node_and_error_t(parse_tree_node_t node) : node(node) {}
      node_and_error_t(parse_tree_node_t node, error_t last_error)
        : node(node), last_error(std::move(last_error))
      {
      }

      parse_tree_node_t as_node() &&
      {
        last_error.clear();
        return std::move(node);
      }
    };

    expected_t<node_and_error_t> s_terminal(const parse_tree_span_t pts,
                                            const name_id_t t_rule_name)
    {
      auto ss = stake();
      SILVA_EXPECT(pts.rule_name() == lexicon.ni_term, MAJOR);
      const auto [s_token_pts] =
          SILVA_EXPECT_FWD(pts.get_children<1>(), BROKEN_SEED, "{} Terminal without child", pts);
      if (s_token_pts.rule_name() == lexicon.ni_nt) {
        const auto nt_it = se->resolved_names.find(s_token_pts);
        SILVA_EXPECT(nt_it != se->resolved_names.end(),
                     MAJOR,
                     "{} couldn't lookup nonterminal",
                     pts);
        const name_id_t literal_scope = nt_it->resolved_name;
        const auto ls_it              = se->scope_to_literals.find(literal_scope);
        SILVA_EXPECT(ls_it != se->scope_to_literals.end(),
                     MAJOR,
                     "literals_of {}: no such scope",
                     lexicon.name_id_wrap(literal_scope));
        error_nursery_t error_nursery;
        for (const fragmented_token_t& ft: ls_it->second) {
          auto result = parse_literal(ft);
          if (!result) {
            error_nursery.add_child_error(std::move(result).error());
            continue;
          }
          ss.add_proto_node(*result);
          if (curr_rule->is_literal_nodes) {
            ss.create_node(name_id_literal, true);
          }
          auto retval = ss.commit();
          if (twig_rule_depth == 0) {
            SILVA_EXPECT_FWD(skip());
          }
          return retval;
        }
        return std::unexpected(std::move(error_nursery)
                                   .finish_short(MINOR,
                                                 "[{}] not in literals_of {}",
                                                 fragment_location_by(),
                                                 lexicon.name_id_wrap(literal_scope)));
      }
      const token_id_t s_token_id = SILVA_EXPECT_FWD(s_token_pts.token());
      if (s_token_pts.rule_name() == lexicon.ni_keyword) {
        SILVA_EXPECT(s_token_id != lexicon.ti_commit.token_id,
                     BROKEN_SEED,
                     "the \"commit\" keyword is only allowed directly in a concat expression");
        if (s_token_id == lexicon.ti_eps.token_id) {
          return ss.commit();
        }
        else if (s_token_id == token_id_language) {
          SILVA_EXPECT_PARSE(
              t_rule_name,
              twig_rule_depth == 0,
              "the 'language' token-category may not be used inside other token rules");
          ss.create_node(name_id_language, false);
          SILVA_EXPECT_PARSE(t_rule_name,
                             fragment_category_by() == fragment_category_t::LANG_BEGIN,
                             "expected token of category LANG_BEGIN; got {}",
                             fragment_category_by());
          fragment_index =
              SILVA_EXPECT_PARSE_FWD(t_rule_name, fp->advance_language(fragment_index));
          auto retval = ss.commit();
          SILVA_EXPECT_FWD(skip());
          return retval;
        }
        else {
          SILVA_EXPECT(false, BROKEN_SEED, "unknown keyword {}", sfp->token_id_wrap(s_token_id));
        }
      }
      SILVA_EXPECT_PARSE(t_rule_name,
                         num_fragments_left() > 0,
                         "Reached end of fragment-stream when looking for {}",
                         sfp->token_id_wrap(s_token_id));

      if (s_token_pts.rule_name() == lexicon.ni_string) {
        const auto it = se->string_to_ft.find(s_token_id);
        SILVA_EXPECT(it != se->string_to_ft.end(),
                     MAJOR,
                     "couldn't find token for {}",
                     sfp->token_id_wrap(s_token_id));
        const fragmented_token_t& expected_ft = it->second;
        ss.add_proto_node(SILVA_EXPECT_FWD(parse_literal(expected_ft),
                                           "[{}] while matching {}",
                                           fragment_location_by(),
                                           sfp->token_id_wrap(expected_ft.token_id)));
        if (curr_rule->is_literal_nodes) {
          ss.create_node(name_id_literal, true);
        }
        auto retval = ss.commit();
        if (twig_rule_depth == 0) {
          SILVA_EXPECT_FWD(skip());
        }
        return retval;
      }
      else if (s_token_pts.rule_name() == lexicon.ni_frag_name) {
        const token_id_t expected_frag_cat_ti = s_token_id;
        if (expected_frag_cat_ti == lexicon.ti_ID_START.token_id) {
          SILVA_EXPECT(is_fragment_category_id_start(fragment_category_by()),
                       MINOR,
                       "expected token of category ID_START; got {}",
                       sfp->token_id_wrap(expected_frag_cat_ti));
        }
        else if (expected_frag_cat_ti == lexicon.ti_ID_CONTINUE.token_id) {
          SILVA_EXPECT(is_fragment_category_id_continue(fragment_category_by()),
                       MINOR,
                       "expected token of category ID_CONTINUE; got {}",
                       sfp->token_id_wrap(expected_frag_cat_ti));
        }
        else {
          const token_id_t curr_frag_cat_ti =
              fragment_category_to_token_id(*sfp, fragment_category_by());
          SILVA_EXPECT(curr_frag_cat_ti == expected_frag_cat_ti,
                       MINOR,
                       "expected token of category {}; got {}",
                       sfp->token_id_wrap(expected_frag_cat_ti),
                       sfp->token_id_wrap(curr_frag_cat_ti));
        }
        fragment_index += 1;
      }
      else {
        SILVA_EXPECT(false, BROKEN_SEED);
      }
      return ss.commit();
    }

    expected_t<node_and_error_t> s_expr_prefix(const parse_tree_span_t pts,
                                               const name_id_t t_rule_name)
    {
      {
        auto ss              = stake();
        const auto [sub_pts] = SILVA_EXPECT_FWD(pts.get_children<1>());
        const auto result    = SILVA_EXPECT_FWD_IF(MAJOR, s_expr(sub_pts, t_rule_name));
        SILVA_EXPECT(!result, MINOR, "Successfully parsed 'not' expression");
      }
      auto ss = stake();
      return ss.commit();
    }

    expected_t<pair_t<index_t, index_t>> get_min_max_repeat(const token_id_t op_ti)
    {
      index_t min_repeat = 0;
      index_t max_repeat = std::numeric_limits<index_t>::max();
      SILVA_EXPECT(op_ti == lexicon.ti_qmark.token_id || op_ti == lexicon.ti_star.token_id ||
                       op_ti == lexicon.ti_plus.token_id,
                   MAJOR);
      if (op_ti == lexicon.ti_qmark.token_id) {
        max_repeat = 1;
      }
      else if (op_ti == lexicon.ti_star.token_id) {
        ;
      }
      else if (op_ti == lexicon.ti_plus.token_id) {
        min_repeat = 1;
      }
      return {pair_t{min_repeat, max_repeat}};
    }

    expected_t<pair_t<index_t, index_t>> get_min_max_quantifier(const parse_tree_span_t& quant_pts)
    {
      index_t min_repeat         = 0;
      index_t max_repeat         = std::numeric_limits<index_t>::max();
      const auto pts_children    = SILVA_EXPECT_FWD(quant_pts.get_children_up_to<3>());
      const auto child_as_number = [&](const index_t idx) -> expected_t<index_t> {
        const token_id_t ti = SILVA_EXPECT_FWD(pts_children[idx].token());
        const double value  = SILVA_EXPECT_FWD(sfp->get(ti).number_as_double());
        return static_cast<index_t>(value);
      };
      optional_t<index_t> comma_pos;
      for (index_t i = 0; i < pts_children.size; ++i) {
        if (pts_children[i].token() == lexicon.ti_comma.token_id) {
          comma_pos = i;
          break;
        }
      }
      if (!comma_pos.has_value()) {
        SILVA_EXPECT(pts_children.size == 1, MAJOR, "expected single number in quantifier");
        min_repeat = max_repeat = SILVA_EXPECT_FWD(child_as_number(0));
      }
      else {
        const index_t cp = comma_pos.value();
        SILVA_EXPECT(cp <= 1 && pts_children.size <= 2 + cp, MAJOR, "malformed quantifier");
        if (cp == 1) {
          min_repeat = SILVA_EXPECT_FWD(child_as_number(0));
        }
        if (cp + 1 < pts_children.size) {
          max_repeat = SILVA_EXPECT_FWD(child_as_number(cp + 1));
        }
      }
      SILVA_EXPECT(min_repeat <= max_repeat,
                   MINOR,
                   "expected min-repeat (={}) <= max-repeat (={})",
                   min_repeat,
                   max_repeat);
      return {{min_repeat, max_repeat}};
    }

    // can be 'a ?' 'a *' 'a +' or 'a{2,3}'
    expected_t<node_and_error_t> s_expr_postfix(const parse_tree_span_t pts,
                                                const name_id_t t_rule_name)
    {
      auto ss                 = stake();
      index_t min_repeat      = 0;
      index_t max_repeat      = 0;
      const token_id_t op_ti  = sfp->get(pts.rule_name()).base_name;
      const auto pts_children = SILVA_EXPECT_FWD(pts.get_children_up_to<2>());
      SILVA_EXPECT(pts_children.size == 1 || pts_children.size == 2, MAJOR);
      const auto pts_expr = pts_children[0];
      if (pts_children.size == 2) {
        std::tie(min_repeat, max_repeat) =
            SILVA_EXPECT_FWD(get_min_max_quantifier(pts_children[1]));
      }
      else {
        std::tie(min_repeat, max_repeat) = SILVA_EXPECT_FWD(get_min_max_repeat(op_ti));
      }
      index_t repeat_count = 0;
      error_t last_error;
      while (repeat_count < max_repeat) {
        auto result = SILVA_EXPECT_FWD_IF(MAJOR, s_expr(pts_expr, t_rule_name));
        if (result.has_value()) {
          ss.add_proto_node(std::move(*result).as_node());
          repeat_count += 1;
        }
        else {
          last_error = std::move(result).error();
          break;
        }
      }
      if (repeat_count < min_repeat) {
        array_small_t<error_t, 1> maybe_child_error;
        if (!last_error.is_empty()) {
          maybe_child_error.emplace_back(std::move(last_error));
        }
        return std::unexpected(make_error(MINOR,
                                          maybe_child_error,
                                          "min-repeat (={}) not reached, only found {}",
                                          min_repeat,
                                          repeat_count));
      }
      return node_and_error_t{ss.commit(), std::move(last_error)};
    }

    expected_t<node_and_error_t> s_expr_concat(const parse_tree_span_t pts,
                                               const name_id_t t_rule_name)
    {
      const index_t orig_fragment_index = fragment_index;
      auto ss                           = stake();
      error_nursery_t error_nursery;

      const auto it = se->expr_data.find(pts);
      SILVA_EXPECT(it != se->expr_data.end(),
                   MAJOR,
                   "{} no expression-data for concatenated expression",
                   pts);
      const optional_t<index_t> commit_after = it->second.commit_after;

      index_t prev_fragment_end = -1;
      index_t sub_expr_index    = 0;
      for (const auto sub_pts: pts.children_range()) {
        if (is_commit_terminal(lexicon, sub_pts)) {
          continue;
        }
        auto result = s_expr(sub_pts, t_rule_name);
        if (result.has_value()) {
          const parse_tree_node_t& result_node = result->node;
          const bool curr_has_fragments = (result_node.fragment_end > result_node.fragment_begin);
          if (curr_rule->is_no_whitespace && prev_fragment_end >= 0 && curr_has_fragments) {
            SILVA_EXPECT_PARSE(t_rule_name,
                               prev_fragment_end == result_node.fragment_begin,
                               "no_whitespace: gap between {} and {}",
                               fragment_location_at(prev_fragment_end),
                               fragment_location_at(result_node.fragment_begin));
          }
          if (curr_has_fragments) {
            prev_fragment_end = result_node.fragment_end;
          }
          if (!result->last_error.is_empty()) {
            error_nursery.add_child_error(std::move(result->last_error));
          }
          ss.add_proto_node(std::move(result->node));
        }
        else {
          error_level_t error_level = result.error().level;
          if (commit_after.has_value() && sub_expr_index >= commit_after) {
            error_level = std::max(error_level, MAJOR);
          }
          error_nursery.add_child_error(std::move(result).error());
          return std::unexpected(std::move(error_nursery)
                                     .finish(error_level,
                                             "[{}] {}: expected sequence[ {} ]",
                                             fragment_location_at(orig_fragment_index),
                                             lexicon.name_id_wrap(t_rule_name),
                                             pts.fragment_span()));
        }
        sub_expr_index += 1;
      }
      return ss.commit();
    }

    expected_t<node_and_error_t> s_expr_and(const parse_tree_span_t pts,
                                            const name_id_t t_rule_name)
    {
      optional_t<stake_t<>> ss;
      auto [it, end] = pts.children_range();
      while (true) {
        SILVA_EXPECT(it != end, MAJOR);
        ss.emplace(stake());
        auto result = SILVA_EXPECT_FWD(s_expr(*it, t_rule_name));
        ss->add_proto_node(std::move(result).as_node());
        ++it;
        if (it == end) {
          break;
        }
      }
      SILVA_EXPECT(ss.has_value(), MAJOR);
      return ss->commit();
    }

    expected_t<node_and_error_t> s_expr_followup(const parse_tree_span_t pts,
                                                 const name_id_t t_rule_name)
    {
      auto ss        = stake();
      auto [it, end] = pts.children_range();
      bool is_first  = true;
      while (true) {
        SILVA_EXPECT(it != end, MAJOR);
        auto result = s_expr(*it, t_rule_name);
        if (result.has_value()) {
          ss.add_proto_node(std::move(result->node));
        }
        else {
          if (is_first) {
            return std::unexpected(std::move(result).error());
          }
          break;
        }
        is_first = false;
        ++it;
        if (it == end) {
          break;
        }
      }
      return ss.commit();
    }

    expected_t<node_and_error_t> s_expr_ending(const parse_tree_span_t pts,
                                               const name_id_t t_rule_name)
    {
      const auto [pts_expr, pts_endswith] = SILVA_EXPECT_FWD(pts.get_children<2>());
      SILVA_EXPECT(pts_endswith.rule_name() == lexicon.ni_term,
                   MINOR,
                   "rhs of 'ending_with' expect to be plain string/literal");
      const auto [pts_endswith_str] = SILVA_EXPECT_FWD(pts_endswith.get_children<1>());
      SILVA_EXPECT(pts_endswith_str.rule_name() == lexicon.ni_string,
                   MINOR,
                   "rhs of 'ending_with' expect to be plain string/literal");
      auto retval = SILVA_EXPECT_FWD(s_expr(pts_expr, t_rule_name));
      {
        const auto endswith_token = SILVA_EXPECT_FWD(pts_endswith_str.token());
        const auto it             = se->string_to_ft.find(endswith_token);
        SILVA_EXPECT(it != se->string_to_ft.end(),
                     MAJOR,
                     "couldn't find token for {}",
                     sfp->token_id_wrap(endswith_token));
        const fragmented_token_t& ft = it->second;
        const fragment_span_t fs{fp, retval.node.fragment_begin, retval.node.fragment_end};
        const bool endswith = SILVA_EXPECT_FWD_AS(fragment_span_ends_with(fs, ft), MAJOR);
        SILVA_EXPECT(endswith,
                     MINOR,
                     "{} does not end with {}",
                     fs,
                     sfp->token_id_wrap(ft.token_id));
      }
      return retval;
    }

    // can be 'a | b | c' or '[ a b c ]'
    expected_t<node_and_error_t> s_expr_or(const parse_tree_span_t pts, const name_id_t t_rule_name)
    {
      const index_t orig_fragment_index = fragment_index;
      error_nursery_t error_nursery;
      optional_t<parse_tree_node_t> retval;
      error_level_t error_level = MINOR;
      auto [it, end]            = pts.children_range();
      while (true) {
        SILVA_EXPECT_NURSERY_BREAK(error_nursery, it != end, MAJOR, "expected sub-tree");
        auto result = s_expr(*it, t_rule_name);
        if (result.has_value()) {
          retval = std::move(*result).as_node();
          break;
        }
        else {
          error_level = result.error().level;
          error_nursery.add_child_error(std::move(result).error());
          if (error_level >= MAJOR) {
            break;
          }
        }
        ++it;
        if (it == end) {
          break;
        }
      }
      if (retval.has_value()) {
        return std::move(retval).value();
      }
      return std::unexpected(std::move(error_nursery)
                                 .finish(error_level,
                                         "[{}] {}: expected alternation[ {} ]",
                                         fragment_location_at(orig_fragment_index),
                                         lexicon.name_id_wrap(t_rule_name),
                                         pts.fragment_span()));
    }

    expected_t<node_and_error_t> s_nonterminal(const parse_tree_span_t pts,
                                               const name_id_t t_rule_name)
    {
      const auto nt_it = se->resolved_names.find(pts);
      SILVA_EXPECT(nt_it != se->resolved_names.end(), MAJOR, "{} couldn't lookup nonterminal", pts);
      const name_id_t next_t_rule_name = nt_it->resolved_name;
      return SILVA_EXPECT_FWD_IF(MAJOR, handle_rule(next_t_rule_name));
    }

    expected_t<node_and_error_t> s_expr(const parse_tree_span_t pts, const name_id_t t_rule_name)
    {
      const name_id_t s_rule_name = pts.rule_name();
      if (s_rule_name == lexicon.ni_expr) {
        const auto [pts_child] = SILVA_EXPECT_FWD(pts.get_children<1>());
        return s_expr(pts_child, t_rule_name);
      }
      if (lexicon.ni_expr_prefix.is_parent_of(s_rule_name, *sfp)) {
        return s_expr_prefix(pts, t_rule_name);
      }
      else if (lexicon.ni_expr_postfix.is_parent_of(s_rule_name, *sfp)) {
        return s_expr_postfix(pts, t_rule_name);
      }
      else if (lexicon.ni_expr_concat.is_parent_of(s_rule_name, *sfp)) {
        return s_expr_concat(pts, t_rule_name);
      }
      else if (lexicon.ni_expr_and.is_parent_of(s_rule_name, *sfp)) {
        return s_expr_and(pts, t_rule_name);
      }
      else if (lexicon.ni_expr_followup.is_parent_of(s_rule_name, *sfp)) {
        return s_expr_followup(pts, t_rule_name);
      }
      else if (lexicon.ni_expr_ending.is_parent_of(s_rule_name, *sfp)) {
        return s_expr_ending(pts, t_rule_name);
      }
      else if (lexicon.ni_expr_or.is_parent_of(s_rule_name, *sfp)) {
        return s_expr_or(pts, t_rule_name);
      }
      else if (s_rule_name == lexicon.ni_alternation) {
        return s_expr_or(pts, t_rule_name);
      }
      else if (s_rule_name == lexicon.ni_term) {
        return s_terminal(pts, t_rule_name);
      }
      else if (s_rule_name == lexicon.ni_nt) {
        return s_nonterminal(pts, t_rule_name);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "unknown Seed expression {}", pts);
      }
    }

    expected_t<node_and_error_t> handle_rule_axe(const name_id_t axe_rule_name,
                                                 const name_id_t t_rule_name)
    {
      const auto it = se->axes.find(axe_rule_name);
      SILVA_EXPECT(it != se->axes.end(), MAJOR);
      const auto it_re = se->rule_exprs.find(axe_rule_name);
      SILVA_EXPECT(it_re != se->rule_exprs.end(), MAJOR);
      const bool is_no_node = it_re->second.is_no_node;
      auto ss{stake()};
      const axe_t& axe = it->second;
      const axe_t::parse_delegate_t::pack_t pack{
          [&](const name_id_t rule_name) -> expected_t<parse_tree_node_t> {
            node_and_error_t result = SILVA_EXPECT_FWD(handle_rule(rule_name));
            return std::move(result).as_node();
          },
      };
      const auto skip_dg = axe_t::skip_delegate_t::make<&interpreter_apply_nursery_t::skip>(this);
      ss.add_proto_node(SILVA_EXPECT_PARSE_FWD(
          t_rule_name,
          axe.apply(*this, t_rule_name, is_no_node, pack.delegate, skip_dg)));
      return ss.commit();
    }

    expected_t<void> skip()
    {
      if (!lang_data->skip_rule_expr.has_value()) {
        return {};
      }
      const parse_tree_span_t& s_pts = lang_data->skip_rule_expr->expr;
      if (s_pts.ptp.is_nullptr()) {
        return {};
      }
      auto ss = stake();
      SILVA_EXPECT_FWD_IF(MAJOR, s_expr(s_pts, name_id_t{}));
      const index_t new_frag_idx = fragment_index;
      ss.clear();
      fragment_index = new_frag_idx;
      return {};
    }

    expected_t<node_and_error_t> handle_rule(const name_id_t t_rule_name)
    {
      auto ets = SILVA_EXEC_TRACE_SCOPE(exec_trace, t_rule_name, fragment_location_by());
      rule_depth += 1;
      scope_exit_t scope_exit([this] { rule_depth -= 1; });
      SILVA_EXPECT(rule_depth <= 100,
                   FATAL,
                   "Stack is getting too deep. Infinite recursion in grammar?");
      const auto it{se->rule_exprs.find(t_rule_name)};
      SILVA_EXPECT(it != se->rule_exprs.end(),
                   MAJOR,
                   "Unknown rule: {}",
                   lexicon.name_id_str(t_rule_name));
      const interpreter_t::rule_expr_data_t& rule_data = it->second;
      node_and_error_t retval;
      if (rule_data.is_twig_rule) {
        retval = SILVA_EXPECT_FWD_PLAIN(handle_twig_rule(t_rule_name, rule_data));
      }
      else {
        retval = SILVA_EXPECT_FWD_PLAIN(handle_branch_rule(t_rule_name, rule_data));
      }
      ets->success = true;
      return retval;
    }

    expected_t<node_and_error_t>
    handle_branch_rule(const name_id_t t_rule_name,
                       const interpreter_t::rule_expr_data_t& rule_data)
    {
      const parse_tree_span_t& s_pts = rule_data.expr;
      const name_id_t s_expr_name    = s_pts.rule_name();
      rule_expr_data_scope_t rule_scope(*this, &rule_data);
      node_and_error_t retval;
      if (s_expr_name == lexicon.ni_axe) {
        retval = SILVA_EXPECT_PARSE_FWD(t_rule_name, handle_rule_axe(t_rule_name, t_rule_name));
      }
      else if (s_expr_name == lexicon.ni_axe_level) {
        const name_id_t axe_name = sfp->get(t_rule_name).parent_name;
        retval = SILVA_EXPECT_PARSE_FWD(t_rule_name, handle_rule_axe(axe_name, t_rule_name));
      }
      else {
        auto ss = stake();
        if (!rule_data.is_no_node) {
          ss.create_node(t_rule_name, false);
        }
        auto result = SILVA_EXPECT_PARSE_FWD(t_rule_name, s_expr(s_pts, t_rule_name));
        ss.add_proto_node(std::move(result.node));
        retval = node_and_error_t{ss.commit(), std::move(result.last_error)};
      }
      return retval;
    }

    expected_t<node_and_error_t> handle_twig_rule(const name_id_t t_rule_name,
                                                  const interpreter_t::rule_expr_data_t& rule_data)
    {
      const bool entered_token_space = (twig_rule_depth == 0);
      twig_rule_depth += 1;
      scope_exit_t token_scope_exit([this] { twig_rule_depth -= 1; });

      auto ss = stake();
      if (!rule_data.is_no_node) {
        ss.create_node(t_rule_name, true);
      }
      auto result = SILVA_EXPECT_PARSE_FWD(t_rule_name, s_expr(rule_data.expr, t_rule_name));
      ss.add_proto_node(std::move(result.node));
      auto retval = ss.commit();
      if (entered_token_space) {
        SILVA_EXPECT_FWD(skip());
      }
      return retval;
    }
  };
}

namespace silva::seed {
  interpreter_t::interpreter_t(syntax_farm_ptr_t sfp) : sfp(sfp), bootstrap_interpreter(sfp) {}

  expected_t<void> interpreter_t::add_seed(parse_tree_span_t pts)
  {
    impl::interpreter_adder_t adder(this);
    SILVA_EXPECT_FWD(adder.handle_all(pts));
    compile_reset();
    return {};
  }

  expected_t<void> interpreter_t::add_seed_copy(const parse_tree_span_t& stps_ref)
  {
    parse_tree_ptr_t stps = sfp->add(std::make_unique<parse_tree_t>(stps_ref.copy()));
    return add_seed(stps->span());
  }

  expected_t<parse_tree_ptr_t> interpreter_t::add_seed(fragment_span_t fs)
  {
    auto ptp = SILVA_EXPECT_FWD(bootstrap_interpreter.parse(std::move(fs)));
    SILVA_EXPECT_FWD(add_seed(ptp->span()));
    return ptp;
  }

  expected_t<parse_tree_ptr_t> interpreter_t::add_seed_text(filepath_t filepath, string_t text)
  {
    auto ff  = SILVA_EXPECT_FWD(fragmentize(sfp, std::move(filepath), std::move(text)));
    auto ptp = SILVA_EXPECT_FWD(bootstrap_interpreter.parse(std::move(ff)));
    // fmt::print("{}\n", SILVA_EXPECT_FWD(ptp->span().to_string()));
    SILVA_EXPECT_FWD(add_seed(ptp->span()));
    return ptp;

    auto fp = SILVA_EXPECT_FWD(fragmentize(sfp, std::move(filepath), std::move(text)));
    return add_seed(std::move(fp));
  }

  void interpreter_t::compile_reset()
  {
    is_compiled = false;
    resolved_names.clear();
    for (auto& [_, axe]: axes) {
      axe.compile_reset();
    }
  }

  expected_t<void> interpreter_t::compile()
  {
    compile_reset();

    const lexicon_t& lexicon = bootstrap_interpreter.lexicon();
    for (const auto& [rule_name, rule_data]: rule_exprs) {
      const parse_tree_span_t& pts_rule = rule_data.expr;
      if (pts_rule.rule_name() == lexicon.ni_axe_level) {
        // These parse-trees are already handled by the enclosing axe.
        continue;
      }
      auto res = pts_rule.visit_subtree([&](const span_t<const tree_branch_t> path,
                                            const tree_event_t event) -> expected_t<bool> {
        if (!is_on_entry(event)) {
          return true;
        }
        if (pts_rule.node_at(path.back().node_index).rule_name != lexicon.ni_nt) {
          return true;
        }
        const auto pts_nt   = pts_rule.subspan_at(path.back().node_index);
        auto [it, inserted] = resolved_names.emplace(pts_nt);
        SILVA_EXPECT(inserted, ASSERT);
        SILVA_EXPECT_FWD(it->resolve(rule_name, lexicon, rule_exprs));
        return true;
      });
      SILVA_EXPECT_FWD(std::move(res),
                       "during name-resolution for rule {}",
                       lexicon.name_id_wrap(rule_name));
    }

    for (auto& [rule_name, axe]: axes) {
      SILVA_EXPECT_FWD(axe.compile(lexicon, rule_exprs));
    }

    is_compiled = true;
    return {};
  }

  expected_t<parse_tree_ptr_t> interpreter_t::apply(fragment_span_t fs,
                                                    const name_id_t goal_rule_name)
  {
    if (!is_compiled) {
      SILVA_EXPECT_FWD(compile());
    }

    name_id_t curr = goal_rule_name;
    while (sfp->get(curr).parent_name.is_valid()) {
      curr = sfp->get(curr).parent_name;
    }
    const token_id_t lang_name = sfp->get(curr).base_name;
    const auto lang_it         = languages.find(lang_name);
    SILVA_EXPECT(lang_it != languages.end(),
                 MINOR,
                 "unknown language {}",
                 sfp->token_id_wrap(lang_name));

    impl::interpreter_apply_nursery_t nursery(fs,
                                              bootstrap_interpreter.lexicon(),
                                              this,
                                              &lang_it->second);

    const auto do_trace =
        SILVA_EXPECT_FWD_IF(MAJOR, env_context_get_as<bool>("SEED_EXEC_TRACE")).value_or(false);
    scope_exit_t trace_exit([do_trace, &nursery] {
      if (do_trace) {
        fmt::print("{}", SILVA_ASSERT_FWD(std::move(nursery.exec_trace).as_tree_to_string()));
      }
    });

    SILVA_EXPECT_ASSERT(nursery.init(goal_rule_name, nursery.lexicon));
    SILVA_EXPECT_FWD(nursery.skip());
    SILVA_EXPECT_FWD(nursery.check());
    auto ptn = SILVA_EXPECT_FWD(nursery.handle_rule(goal_rule_name),
                                "seed::interpreter_t::apply({}) failed to parse",
                                nursery.lexicon.name_id_wrap(goal_rule_name));
    if (nursery.fragment_index + 1 != fs.end) {
      SILVA_EXPECT(!ptn.last_error.is_empty(),
                   MAJOR,
                   "could not parse entire text of {}",
                   fs.fp->filepath);
      return std::unexpected(std::move(ptn.last_error));
    }
    SILVA_EXPECT(ptn.node.num_children == 1, ASSERT);
    SILVA_EXPECT(ptn.node.subtree_size == nursery.tree.size(), ASSERT);

    return std::move(nursery).finish();
  }

  expected_t<parse_tree_ptr_t>
  interpreter_t::apply_text(filepath_t filepath, string_t text, name_id_t goal_rule_name)
  {
    auto ff = SILVA_EXPECT_FWD(fragmentize(sfp, std::move(filepath), std::move(text)));
    return apply(std::move(ff), goal_rule_name);
  }
}
