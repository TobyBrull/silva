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
#include "tokenization.hpp"

#include <utility>

using enum silva::error_level_t;

namespace silva::seed::impl {
  struct interpreter_adder_t {
    interpreter_t* se     = nullptr;
    syntax_farm_ptr_t sfp = se->sfp;
    const lexicon_t& lexicon;

    optional_t<token_id_t> current_language_id;

    const tokenization_ptr_t tp;

    interpreter_adder_t(interpreter_t* se, const parse_tree_span_t& pts)
      : se(se), lexicon(se->bootstrap_interpreter.lexicon()), tp(pts.ptp->tp)
    {
    }

    expected_t<void> register_rule(const name_id_t rule_name,
                                   const parse_tree_span_t& pts,
                                   const bool is_token_rule    = false,
                                   const bool is_alias         = false,
                                   const bool is_no_whitespace = false)
    {
      const auto [emplace_it, inserted] = se->rule_exprs.emplace(
          rule_name,
          interpreter_t::rule_expr_data_t{.expr             = pts,
                                          .is_token_rule    = is_token_rule,
                                          .is_alias         = is_alias,
                                          .is_no_whitespace = is_no_whitespace});
      SILVA_EXPECT(inserted,
                   MINOR,
                   "{} rule {} defined again, previously defined at {}",
                   pts,
                   lexicon.name_id_wrap(rule_name),
                   emplace_it->second.expr);
      return {};
    }

    expected_t<void> handle_rule(const name_id_t scope_name, const parse_tree_span_t pts_rule)
    {
      SILVA_EXPECT(pts_rule[0].rule_name == lexicon.ni_rule, MINOR, "expected Rule");

      auto [it, end] = pts_rule.children_range();
      SILVA_EXPECT(it != end, MINOR, "{} rule must have at least one child", pts_rule);

      name_id_t curr_rule_name;
      bool is_token_rule = false;
      if (SILVA_EXPECT_FWD(pts_rule.front_token_id()) == lexicon.ti_here.token_id) {
        curr_rule_name = scope_name;
      }
      else {
        const auto pts_nt = pts_rule.sub_tree_span_at(it.pos);
        curr_rule_name =
            SILVA_EXPECT_FWD(lexicon.name_id_definition(scope_name, pts_nt.token_span()));
        const name_id_t back_token_cat = SILVA_EXPECT_FWD(pts_nt.back_token_category());
        is_token_rule                  = (back_token_cat == lexicon.ni_token_cat_name);
        ++it;
      }

      bool is_alias         = false;
      bool is_no_whitespace = false;
      while (it != end && pts_rule[it.pos].rule_name == lexicon.ni_qualifier) {
        const auto pts_qual    = pts_rule.sub_tree_span_at(it.pos);
        const token_id_t q_tok = SILVA_EXPECT_FWD(pts_qual.front_token_id());
        if (q_tok == lexicon.ti_alias.token_id) {
          is_alias = true;
        }
        else if (q_tok == lexicon.ti_no_whitespace.token_id) {
          is_no_whitespace = true;
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
      const auto pts_rhs_0 = pts_rule.sub_tree_span_at(it.pos);

      ++it;
      SILVA_EXPECT(it == end, MINOR, "{} rule had too many children", pts_rule);
      SILVA_EXPECT_FWD(
          register_rule(curr_rule_name, pts_rhs_0, is_token_rule, is_alias, is_no_whitespace));

      if (sfp->name_infos[curr_rule_name.val].base_name == lexicon.ti_skip.token_id) {
        SILVA_EXPECT(current_language_id.has_value(),
                     MINOR,
                     "skip token-rule may only be used in language");
        se->languages.at(*current_language_id).skip_rule_expr =
            interpreter_t::rule_expr_data_t{.expr = pts_rhs_0, .is_token_rule = true};
      }

      for (index_t i = 0; i < pts_rhs_0.size(); ++i) {
        if (pts_rhs_0[i].rule_name == lexicon.ni_term) {
          const auto& token = tp->tokens[pts_rhs_0[i].token_begin];
          if (token.category == lexicon.ni_string) {
            const auto ti     = SILVA_EXPECT_FWD(sfp->token_id_in_string(token.token_id));
            const auto& tinfo = sfp->token_infos[ti.val];
            se->string_to_ft[token.token_id] = SILVA_EXPECT_FWD(fragmented_token(sfp, tinfo.str));
          }
        }
      }

      const name_id_t expr_rule_name = pts_rhs_0[0].rule_name;
      if (expr_rule_name == lexicon.ni_axe) {
        se->axes[curr_rule_name] = SILVA_EXPECT_FWD(axe_create(sfp, curr_rule_name, pts_rhs_0));
        auto [axe_it, axe_end]   = pts_rhs_0.children_range();
        SILVA_EXPECT(axe_it != axe_end, MINOR);
        SILVA_EXPECT(pts_rhs_0[axe_it.pos].rule_name == lexicon.ni_nt, MINOR);
        ++axe_it;
        SILVA_EXPECT(axe_it != axe_end, MINOR);
        SILVA_EXPECT(pts_rhs_0[axe_it.pos].rule_name == lexicon.ni_nt, MINOR);
        ++axe_it;
        while (axe_it != axe_end) {
          SILVA_EXPECT(pts_rhs_0[axe_it.pos].rule_name == lexicon.ni_axe_level, MINOR);
          const auto pts_level          = pts_rhs_0.sub_tree_span_at(axe_it.pos);
          const token_id_t level_name   = SILVA_EXPECT_FWD(pts_level.front_token_id());
          const name_id_t axe_rule_name = sfp->name_id(curr_rule_name, level_name);
          SILVA_EXPECT_FWD(register_rule(axe_rule_name, pts_level));
          ++axe_it;
        }
      }

      return {};
    }

    template<typename Iter>
    expected_t<void> handle_scope_impl(const name_id_t scope_name,
                                       const parse_tree_span_t pts_scope,
                                       Iter it,
                                       const Iter end)
    {
      while (it != end) {
        const auto pts_child = pts_scope.sub_tree_span_at(it.pos);
        if (pts_child[0].rule_name == lexicon.ni_scope) {
          SILVA_EXPECT_FWD(handle_scope(scope_name, pts_child));
        }
        else {
          SILVA_EXPECT_FWD(handle_rule(scope_name, pts_child));
        }
        ++it;
      }
      return {};
    }

    expected_t<void> handle_scope(const name_id_t scope_name, const parse_tree_span_t pts_scope)
    {
      SILVA_EXPECT(pts_scope[0].rule_name == lexicon.ni_scope, MINOR, "expected Scope");

      auto [it, end] = pts_scope.children_range();
      SILVA_EXPECT(it != end, MINOR, "{} scope must have at least one child", pts_scope);

      const auto pts_nt = pts_scope.sub_tree_span_at(it.pos);
      const name_id_t curr_scope_name =
          SILVA_EXPECT_FWD(lexicon.name_id_definition(scope_name, pts_nt.token_span()));
      ++it;
      SILVA_EXPECT(it != end,
                   MINOR,
                   "{} scope must have at least one sub-rule or sub-scope",
                   pts_scope);
      SILVA_EXPECT_FWD(handle_scope_impl(curr_scope_name, pts_scope, it, end));
      return {};
    }

    expected_t<void> handle_language(const name_id_t scope_name,
                                     const parse_tree_span_t pts_language)
    {
      SILVA_EXPECT(!current_language_id.has_value(),
                   MINOR,
                   "languages cannot be nested at {}",
                   pts_language);
      SILVA_EXPECT(pts_language[0].rule_name == lexicon.ni_language, MINOR, "expected Language");
      const token_id_t lang_id       = SILVA_EXPECT_FWD(pts_language.at_token_id(1));
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
      auto [it, end]                  = pts_language.children_range();
      const name_id_t curr_scope_name = sfp->name_id(scope_name, lang_id);
      SILVA_EXPECT_FWD(handle_scope_impl(curr_scope_name, pts_language, it, end));
      return {};
    }

    expected_t<void> handle_seed(const name_id_t scope_name, const parse_tree_span_t pts_seed)
    {
      SILVA_EXPECT(pts_seed.size() != 0 && pts_seed[0].rule_name == lexicon.ni_seed,
                   MINOR,
                   "Seed parse_tree should start with Seed node");

      for (const auto [node_index, child_index]: pts_seed.children_range()) {
        const auto pts_child = pts_seed.sub_tree_span_at(node_index);
        if (pts_child[0].rule_name == lexicon.ni_language) {
          SILVA_EXPECT_FWD(handle_language(scope_name, pts_child));
        }
        else if (pts_child[0].rule_name == lexicon.ni_scope) {
          SILVA_EXPECT_FWD(handle_scope(scope_name, pts_child));
        }
        else {
          SILVA_EXPECT_FWD(handle_rule(scope_name, pts_child));
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
        const auto& data = ets[path.back().node_index].item.data;
        curr_line += lexicon.name_id_str(data.rule_name);
        string_pad(curr_line, 55);
        curr_line += fmt::format("{}", data.success);
        string_pad(curr_line, 65);
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

    int token_rule_depth = 0;

    bool no_whitespace = false;
    struct no_whitespace_scope_t {
      interpreter_apply_nursery_t& self;
      bool prev_value;
      no_whitespace_scope_t(interpreter_apply_nursery_t& self_, const bool new_value)
        : self(self_), prev_value(self_.no_whitespace)
      {
        self.no_whitespace = new_value;
      }
      ~no_whitespace_scope_t() { self.no_whitespace = prev_value; }
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
                   "Seed and target parse-trees/tokenizations must be in same syntax_farm_t");
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
      auto ss            = stake();
      const auto& s_node = pts[0];
      SILVA_EXPECT(s_node.rule_name == lexicon.ni_term, MAJOR);
      const auto& s_front_token = pts.ptp->tp->tokens[s_node.token_begin];
      if (s_front_token.token_id == lexicon.ti_eps.token_id) {
        return ss.commit();
      }
      else if (s_front_token.token_id == lexicon.ti_end_of_lang.token_id) {
        SILVA_EXPECT_PARSE(t_rule_name,
                           num_fragments_left() == 0,
                           "expected {}",
                           sfp->token_id_wrap(lexicon.ti_end_of_lang.token_id));
        return ss.commit();
      }
      else if (s_front_token.token_id == lexicon.ti_language.token_id) {
        SILVA_EXPECT_PARSE(
            t_rule_name,
            token_rule_depth == 0,
            "the 'language' token-category may not be used inside other token rules");
        auto ts = token_stake(name_id_t{lexicon.ti_language.token_id.val});
        SILVA_EXPECT_PARSE(t_rule_name,
                           fragment_category_by() == fragment_category_t::LANG_BEGIN,
                           "expected token of category LANG_BEGIN; got {}",
                           fragment_category_by());
        fragment_index = SILVA_EXPECT_PARSE_FWD(t_rule_name, fp->advance_language(fragment_index));
        add_token(ts.commit(true));
        SILVA_EXPECT_FWD(skip());
        return ss.commit();
      }
      SILVA_EXPECT_PARSE(t_rule_name,
                         num_fragments_left() > 0,
                         "Reached end of token-stream when looking for {}",
                         sfp->token_id_wrap(s_front_token.token_id));
      if (s_front_token.category == lexicon.ni_string) {
        const auto it = se->string_to_ft.find(s_front_token.token_id);
        SILVA_EXPECT(it != se->string_to_ft.end(),
                     MAJOR,
                     "Couldn't find token for {}",
                     sfp->token_id_wrap(s_front_token.token_id));
        const fragmented_token_t& expected_ft = it->second;
        const token_t token = SILVA_EXPECT_FWD(literal_fragmented_token(expected_ft),
                                               "[{}] while matching {}",
                                               fragment_location_by(),
                                               sfp->token_id_wrap(expected_ft.token_id));
        if (token_rule_depth == 0) {
          add_token(token);
          SILVA_EXPECT_FWD(skip());
        }
      }
      else if (s_front_token.category == lexicon.ni_frag_name) {
        const token_id_t expected_frag_cat_ti = s_front_token.token_id;
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
        auto ss             = stake();
        const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
        auto result =
            SILVA_EXPECT_FWD_IF(MAJOR, s_expr(pts.sub_tree_span_at(children[0]), t_rule_name));
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
      const index_t num_tokens   = quant_pts.token_size();
      const auto token_as_number = [&](const index_t idx) -> expected_t<index_t> {
        const token_id_t ti = SILVA_EXPECT_FWD(quant_pts.at_token_id(idx));
        const double value  = SILVA_EXPECT_FWD(sfp->token_infos[ti.val].number_as_double());
        return static_cast<index_t>(value);
      };
      optional_t<index_t> comma_pos;
      for (index_t i = 0; i < num_tokens; ++i) {
        if (SILVA_EXPECT_FWD(quant_pts.at_token_id(i)) == lexicon.ti_comma.token_id) {
          comma_pos = i;
          break;
        }
      }
      if (!comma_pos.has_value()) {
        SILVA_EXPECT(num_tokens == 1, MAJOR, "expected single number in quantifier");
        min_repeat = max_repeat = SILVA_EXPECT_FWD(token_as_number(0));
      }
      else {
        const index_t cp = comma_pos.value();
        SILVA_EXPECT(cp <= 1 && num_tokens <= 2 + cp, MAJOR, "malformed quantifier");
        if (cp == 1) {
          min_repeat = SILVA_EXPECT_FWD(token_as_number(0));
        }
        if (cp + 1 < num_tokens) {
          max_repeat = SILVA_EXPECT_FWD(token_as_number(cp + 1));
        }
      }
      SILVA_EXPECT(min_repeat <= max_repeat,
                   MINOR,
                   "expected min-repeat (={}) <= max-repeat (={})",
                   min_repeat,
                   max_repeat);
      return {{min_repeat, max_repeat}};
    }

    expected_t<node_and_error_t> s_expr_postfix(const parse_tree_span_t pts,
                                                const name_id_t t_rule_name)
    {
      auto ss                = stake();
      index_t base_child_idx = 0;
      index_t min_repeat     = 0;
      index_t max_repeat     = 0;
      const token_id_t op_ti = sfp->name_infos[pts[0].rule_name.val].base_name;
      if (op_ti == lexicon.ti_brace_open.token_id) {
        const auto children              = SILVA_EXPECT_FWD(pts.get_children<2>());
        base_child_idx                   = children[0];
        const auto pts_braces            = pts.sub_tree_span_at(children[1]);
        std::tie(min_repeat, max_repeat) = SILVA_EXPECT_FWD(get_min_max_quantifier(pts_braces));
      }
      else {
        const auto children              = SILVA_EXPECT_FWD(pts.get_children<1>());
        base_child_idx                   = children[0];
        std::tie(min_repeat, max_repeat) = SILVA_EXPECT_FWD(get_min_max_repeat(op_ti));
      }
      index_t repeat_count = 0;
      error_t last_error;
      while (repeat_count < max_repeat) {
        auto result =
            SILVA_EXPECT_FWD_IF(MAJOR, s_expr(pts.sub_tree_span_at(base_child_idx), t_rule_name));
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

      // Should do this bit ahead of time and store a map in the interpreter_t.
      index_t lead_terminals = 0;
      for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
        const auto sub_pts = pts.sub_tree_span_at(sub_s_node_index);
        if (sub_pts[0].rule_name == lexicon.ni_term &&
            SILVA_EXPECT_FWD(sub_pts.front_token_category()) == lexicon.ni_string) {
          lead_terminals += 1;
        }
        else {
          break;
        }
      }

      index_t prev_token_end = -1;
      for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
        const auto sub_pts = pts.sub_tree_span_at(sub_s_node_index);
        auto result        = s_expr(sub_pts, t_rule_name);
        if (result.has_value()) {
          const parse_tree_node_t& result_node = result->node;
          const bool curr_has_tokens           = (result_node.token_end > result_node.token_begin);
          if (no_whitespace && prev_token_end >= 0 && curr_has_tokens) {
            const token_t& lhs_data = tokenization.tokens[prev_token_end - 1];
            const token_t& rhs_data = tokenization.tokens[result_node.token_begin];
            SILVA_EXPECT_PARSE(t_rule_name,
                               lhs_data.frag_idx_end == rhs_data.frag_idx_begin,
                               "no_whitespace: gap between {} and {}",
                               sfp->token_id_wrap(lhs_data.token_id),
                               sfp->token_id_wrap(rhs_data.token_id));
          }
          if (curr_has_tokens) {
            prev_token_end = result_node.token_end;
          }
          if (!result->last_error.is_empty()) {
            error_nursery.add_child_error(std::move(result->last_error));
          }
          ss.add_proto_node(std::move(result->node));
        }
        else {
          error_level_t error_level = result.error().level;
          if (lead_terminals >= 1 && child_index >= lead_terminals) {
            error_level = std::max(error_level, MAJOR);
          }
          error_nursery.add_child_error(std::move(result).error());
          return std::unexpected(std::move(error_nursery)
                                     .finish(error_level,
                                             "[{}] {}: expected sequence[ {} ]",
                                             fragment_location_at(orig_fragment_index),
                                             lexicon.name_id_wrap(t_rule_name),
                                             pts.token_span()));
        }
      }
      return ss.commit();
    }

    expected_t<node_and_error_t> s_expr_and(const parse_tree_span_t pts,
                                            const name_id_t t_rule_name)
    {
      optional_t<stake_t> ss;
      for (const auto [child_node_index, child_index]: pts.children_range()) {
        ss.emplace(stake());
        auto result = SILVA_EXPECT_FWD(s_expr(pts.sub_tree_span_at(child_node_index), t_rule_name));
        ss->add_proto_node(std::move(result).as_node());
      }
      SILVA_EXPECT(ss.has_value(), MAJOR);
      return ss->commit();
    }

    expected_t<node_and_error_t> s_expr_or(const parse_tree_span_t pts, const name_id_t t_rule_name)
    {
      const index_t orig_fragment_index = fragment_index;
      error_nursery_t error_nursery;
      optional_t<parse_tree_node_t> retval;
      error_level_t error_level = MINOR;
      for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
        auto result = s_expr(pts.sub_tree_span_at(sub_s_node_index), t_rule_name);
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
      }
      if (retval.has_value()) {
        return std::move(retval).value();
      }
      return std::unexpected(std::move(error_nursery)
                                 .finish(error_level,
                                         "[{}] {}: expected alternation[ {} ]",
                                         fragment_location_at(orig_fragment_index),
                                         lexicon.name_id_wrap(t_rule_name),
                                         pts.token_span()));
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
      const name_id_t s_rule_name = pts[0].rule_name;
      if (sfp->name_id_is_parent(lexicon.ni_expr_prefix, s_rule_name)) {
        return s_expr_prefix(pts, t_rule_name);
      }
      else if (sfp->name_id_is_parent(lexicon.ni_expr_postfix, s_rule_name)) {
        return s_expr_postfix(pts, t_rule_name);
      }
      else if (sfp->name_id_is_parent(lexicon.ni_expr_concat, s_rule_name)) {
        return s_expr_concat(pts, t_rule_name);
      }
      else if (sfp->name_id_is_parent(lexicon.ni_expr_and, s_rule_name)) {
        return s_expr_and(pts, t_rule_name);
      }
      else if (sfp->name_id_is_parent(lexicon.ni_expr_or, s_rule_name)) {
        return s_expr_or(pts, t_rule_name);
      }
      else if (s_rule_name == lexicon.ni_term) {
        return s_terminal(pts, t_rule_name);
      }
      else if (s_rule_name == lexicon.ni_nt) {
        return s_nonterminal(pts, t_rule_name);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "unknown seed expression {}", pts);
      }
    }

    expected_t<node_and_error_t> handle_rule_axe(const name_id_t axe_rule_name,
                                                 const name_id_t t_rule_name)
    {
      const auto it = se->axes.find(axe_rule_name);
      SILVA_EXPECT(it != se->axes.end(), MAJOR);
      auto ss{stake()};
      const axe_t& axe = it->second;
      const axe_t::parse_delegate_t::pack_t pack{
          [&](const name_id_t rule_name) -> expected_t<parse_tree_node_t> {
            node_and_error_t result = SILVA_EXPECT_FWD(handle_rule(rule_name));
            return std::move(result).as_node();
          },
      };
      ss.add_proto_node(
          SILVA_EXPECT_PARSE_FWD(t_rule_name, axe.apply(*this, t_rule_name, pack.delegate)));
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
      if (rule_data.is_token_rule) {
        retval = SILVA_EXPECT_FWD_PLAIN(handle_rule_token(t_rule_name, rule_data));
      }
      else {
        retval = SILVA_EXPECT_FWD_PLAIN(handle_rule_node(t_rule_name, rule_data));
      }
      ets->success = true;
      return retval;
    }

    expected_t<node_and_error_t> handle_rule_node(const name_id_t t_rule_name,
                                                  const interpreter_t::rule_expr_data_t& rule_data)
    {
      const parse_tree_span_t& s_pts = rule_data.expr;
      const name_id_t s_expr_name    = s_pts[0].rule_name;
      no_whitespace_scope_t nws_scope(*this, rule_data.is_no_whitespace);
      node_and_error_t retval;
      if (s_expr_name == lexicon.ni_axe) {
        retval = SILVA_EXPECT_PARSE_FWD(t_rule_name, handle_rule_axe(t_rule_name, t_rule_name));
      }
      else if (s_expr_name == lexicon.ni_axe_level) {
        const name_id_t axe_name = sfp->name_infos[t_rule_name.val].parent_name;
        retval = SILVA_EXPECT_PARSE_FWD(t_rule_name, handle_rule_axe(axe_name, t_rule_name));
      }
      else {
        auto ss = stake();
        if (!rule_data.is_alias) {
          ss.create_node(t_rule_name);
        }
        auto result = SILVA_EXPECT_PARSE_FWD(t_rule_name, s_expr(s_pts, t_rule_name));
        ss.add_proto_node(std::move(result.node));
        retval = node_and_error_t{ss.commit(), std::move(result.last_error)};
      }
      return retval;
    }

    expected_t<node_and_error_t> handle_rule_token(const name_id_t t_rule_name,
                                                   const interpreter_t::rule_expr_data_t& rule_data)
    {
      const bool entered_token_space = (token_rule_depth == 0);
      token_rule_depth += 1;
      scope_exit_t token_scope_exit([this] { token_rule_depth -= 1; });

      auto ss     = stake();
      auto ts     = token_stake(t_rule_name);
      auto result = SILVA_EXPECT_PARSE_FWD(t_rule_name, s_expr(rule_data.expr, t_rule_name));
      const token_t token = ts.commit();
      if (entered_token_space) {
        add_token(token);
        SILVA_EXPECT_FWD(skip());
      }
      ss.add_proto_node(std::move(result.node));
      return ss.commit();
    }
  };
}

namespace silva::seed {
  interpreter_t::interpreter_t(syntax_farm_ptr_t sfp) : sfp(sfp), bootstrap_interpreter(sfp) {}

  expected_t<void> interpreter_t::add_seed(parse_tree_span_t pts)
  {
    impl::interpreter_adder_t adder(this, pts);
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
      if (pts_rule[0].rule_name == lexicon.ni_axe_level) {
        // These parse-trees are already handled by the enclosing axe.
        continue;
      }
      auto res = pts_rule.visit_subtree([&](const span_t<const tree_branch_t> path,
                                            const tree_event_t event) -> expected_t<bool> {
        if (!is_on_entry(event)) {
          return true;
        }
        if (pts_rule[path.back().node_index].rule_name != lexicon.ni_nt) {
          return true;
        }
        const auto pts_nt   = pts_rule.sub_tree_span_at(path.back().node_index);
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
    while (sfp->name_infos[curr.val].parent_name.is_valid()) {
      curr = sfp->name_infos[curr.val].parent_name;
    }
    const token_id_t lang_name = sfp->name_infos[curr.val].base_name;
    const auto lang_it         = languages.find(lang_name);
    SILVA_EXPECT(lang_it != languages.end(),
                 MINOR,
                 "unknown language {}",
                 sfp->token_id_wrap(lang_name));

    impl::interpreter_apply_nursery_t nursery(fs,
                                              bootstrap_interpreter.lexicon(),
                                              this,
                                              &lang_it->second);
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

    if (SILVA_EXPECT_FWD_IF(MAJOR, env_context_get_as<bool>("SEED_EXEC_TRACE")).value_or(false)) {
      fmt::print("{}", SILVA_EXPECT_FWD(std::move(nursery.exec_trace).as_tree_to_string()));
    }

    return std::move(nursery).finish();
  }

  expected_t<parse_tree_ptr_t>
  interpreter_t::apply_text(filepath_t filepath, string_t text, name_id_t goal_rule_name)
  {
    auto ff = SILVA_EXPECT_FWD(fragmentize(sfp, std::move(filepath), std::move(text)));
    return apply(std::move(ff), goal_rule_name);
  }
}
