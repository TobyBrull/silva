#include "seed_interpreter.hpp"

#include "canopy/exec_trace.hpp"
#include "canopy/expected.hpp"
#include "canopy/scope_exit.hpp"
#include "canopy/small_vector.hpp"
#include "canopy/var_context.hpp"
#include "name_id_style.hpp"
#include "parse_tree.hpp"
#include "parse_tree_nursery.hpp"
#include "seed.hpp"
#include "seed_axe.hpp"
#include "tokenization.hpp"

#include <utility>

namespace silva::seed {
  using enum token_category_t;
  using enum error_level_t;

  struct seed_engine_create_nursery_t {
    interpreter_t* se          = nullptr;
    syntax_ward_ptr_t swp      = se->swp;
    const name_id_style_t& nis = swp->default_name_id_style();

    const tokenization_t& s_tokenization;
    const vector_t<token_id_t>& s_tokens = s_tokenization.tokens;

    const name_id_t ni_seed         = swp->name_id_of("Seed");
    const name_id_t ni_rule         = swp->name_id_of(ni_seed, "Rule");
    const name_id_t ni_expr_or_a    = swp->name_id_of(ni_seed, "ExprOrAlias");
    const name_id_t ni_axe          = swp->name_id_of(ni_seed, "Axe");
    const name_id_t ni_expr         = swp->name_id_of(ni_seed, "Expr");
    const name_id_t ni_atom         = swp->name_id_of(ni_seed, "Atom");
    const name_id_t ni_nt_maybe_var = swp->name_id_of(ni_seed, "NonterminalMaybeVar");
    const name_id_t ni_nt           = swp->name_id_of(ni_seed, "Nonterminal");
    const name_id_t ni_term         = swp->name_id_of(ni_seed, "Terminal");

    seed_engine_create_nursery_t(interpreter_t* se, const tokenization_t& s_tokenization)
      : se(se), s_tokenization(s_tokenization)
    {
    }

    expected_t<void> recognize_keyword(name_id_t rule_name, const token_id_t keyword)
    {
      while (true) {
        se->keyword_scopes[rule_name].insert(keyword);
        if (rule_name == name_id_root) {
          break;
        }
        rule_name = swp->name_infos[rule_name].parent_name;
      }
      return {};
    }

    expected_t<void> handle_rule(const name_id_t scope_name, const parse_tree_span_t pts_rule)
    {
      SILVA_EXPECT(pts_rule[0].rule_name == ni_rule, MINOR, "expected Rule");
      const auto children = SILVA_EXPECT_FWD(pts_rule.get_children<2>());
      SILVA_EXPECT(pts_rule[children[0]].rule_name == ni_nt,
                   MINOR,
                   "first child of {} must be {}",
                   swp->name_id_wrap(ni_rule),
                   swp->name_id_wrap(ni_nt));
      const name_id_t curr_rule_name =
          SILVA_EXPECT_FWD(nis.derive_name(scope_name, pts_rule.sub_tree_span_at(children[0])));
      const index_t expr_rule_name = pts_rule[children[1]].rule_name;
      if (expr_rule_name == ni_seed) {
        SILVA_EXPECT_FWD(handle_seed(curr_rule_name, pts_rule.sub_tree_span_at(children[1])));
      }
      else {
        SILVA_EXPECT(expr_rule_name == ni_axe || expr_rule_name == ni_expr_or_a,
                     MINOR,
                     "second child of {} must not be {}",
                     swp->name_id_wrap(ni_rule),
                     swp->name_id_wrap(expr_rule_name));
        const auto [it, inserted] =
            se->rule_exprs.emplace(curr_rule_name, pts_rule.sub_tree_span_at(children[1]));
        SILVA_EXPECT(inserted,
                     MINOR,
                     "{} rule {} defined again, previously defined at {}",
                     pts_rule,
                     swp->name_id_wrap(curr_rule_name),
                     it->second);

        const auto pts_expr = pts_rule.sub_tree_span_at(children[1]);

        for (index_t i = 0; i < pts_expr.size(); ++i) {
          if (pts_expr[i].rule_name == ni_term) {
            const index_t token_idx        = pts_expr[i].token_begin;
            const token_id_t token_id      = pts_expr.tp->tokens[token_idx];
            const token_info_t& token_info = swp->token_infos[token_id];
            if (token_info.category == token_category_t::STRING) {
              const auto keyword              = SILVA_EXPECT_FWD(swp->token_id_in_string(token_id));
              se->string_to_keyword[token_id] = keyword;
              SILVA_EXPECT_FWD(recognize_keyword(scope_name, keyword));
            }
          }
        }

        if (expr_rule_name == ni_axe) {
          se->seed_axes[curr_rule_name] =
              SILVA_EXPECT_FWD(seed_axe_create(swp, curr_rule_name, pts_expr));
        }
        else {
          for (index_t i = 0; i < pts_expr.size(); ++i) {
            if (pts_expr[i].rule_name == ni_nt) {
              const name_id_t nt_name =
                  SILVA_EXPECT_FWD(nis.derive_name(scope_name, pts_expr.sub_tree_span_at(i)));
              const auto [it, inserted] =
                  se->nonterminal_rules.emplace(pts_expr.sub_tree_span_at(i), nt_name);
              SILVA_EXPECT(inserted, MAJOR);
            }
          }
        }
      }
      return {};
    }

    expected_t<void> handle_seed(const name_id_t scope_name, const parse_tree_span_t pts_seed)
    {
      SILVA_EXPECT(pts_seed.size() != 0 && pts_seed[0].rule_name == ni_seed,
                   MINOR,
                   "Seed parse_tree should start with Seed node");

      for (const auto [rule_node_index, child_index]: pts_seed.children_range()) {
        SILVA_EXPECT_FWD(handle_rule(scope_name, pts_seed.sub_tree_span_at(rule_node_index)));
      }
      return {};
    }

    expected_t<void> handle_all(const parse_tree_span_t pts)
    {
      SILVA_EXPECT_FWD(handle_seed(name_id_root, pts));

      // Pre-compile hashmap_t of "regexes".
      for (index_t node_index = 0; node_index < pts.size(); ++node_index) {
        const auto& s_node = pts[node_index];
        if (s_node.rule_name == ni_term && s_node.num_tokens() == 3) {
          const token_id_t regex_token_id = s_tokens[s_node.token_begin + 2];
          if (auto& regex = se->regexes[regex_token_id]; !regex.has_value()) {
            const auto& regex_td = s_tokenization.token_info_get(s_node.token_begin + 2);
            const string_t regex_str{
                SILVA_EXPECT_FWD(regex_td->string_as_plain_contained(), MAJOR)};
            regex = std::regex(regex_str);
          }
        }
      }

      return {};
    }
  };

  interpreter_t::interpreter_t(syntax_ward_ptr_t swp) : swp(swp) {}

  expected_t<void> interpreter_t::callback_if(const parse_tree_span_t& pts) const
  {
    const auto it = parse_callbacks.find(pts[0].rule_name);
    if (it != parse_callbacks.end()) {
      const auto& cb = it->second;
      SILVA_EXPECT_FWD(cb(pts));
    }
    return {};
  }

  expected_t<void> interpreter_t::add(parse_tree_span_t stps)
  {
    seed_engine_create_nursery_t nursery(this, *stps.tp);
    SILVA_EXPECT_FWD(nursery.handle_all(stps));
    return {};
  }

  expected_t<void> interpreter_t::add_copy(const parse_tree_span_t& stps_ref)
  {
    parse_tree_ptr_t stps = swp->add(std::make_unique<parse_tree_t>(stps_ref.copy()));
    return add(stps->span());
  }

  expected_t<parse_tree_ptr_t> interpreter_t::add_complete_file(filesystem_path_t filepath,
                                                                string_view_t text)
  {
    auto tt  = SILVA_EXPECT_FWD(tokenize(swp, std::move(filepath), std::move(text)));
    auto ptp = SILVA_EXPECT_FWD(seed::seed_parse(std::move(tt)));
    // fmt::print("{}\n", SILVA_EXPECT_FWD(ptp->span().to_string()));
    SILVA_EXPECT_FWD(add(ptp->span()));
    return ptp;
  }

  // Maps variable name to the index in the parsed tree that it refers to.
  using var_map_t = hashmap_t<token_id_t, index_t>;

  namespace impl {
    struct seed_exec_trace_data_t {
      name_id_t rule_name = name_id_root;
      token_position_t token_pos;
      bool success = false;
    };
    struct seed_exec_trace_t : public exec_trace_t<seed_exec_trace_data_t> {
      syntax_ward_ptr_t swp;

      expected_t<string_t> as_tree_to_string() &&
      {
        const auto& nis = swp->default_name_id_style();
        auto ett        = SILVA_EXPECT_FWD(std::move(*this).as_tree());
        tree_span_t ets{ett};
        string_t retval =
            SILVA_EXPECT_FWD(ets.to_string([&](string_t& curr_line, const auto& path) {
              const auto& data = ets[path.back().node_index].item.data;
              curr_line += nis.absolute(data.rule_name);
              string_pad(curr_line, 55);
              curr_line += fmt::format("{}", data.success);
              string_pad(curr_line, 65);
              curr_line += fmt::format("{}", to_string(data.token_pos));
            }));
        return {std::move(retval)};
      }
    };

    struct seed_engine_nursery_t : public parse_tree_nursery_t {
      const interpreter_t* se    = nullptr;
      syntax_ward_ptr_t swp      = se->swp;
      const name_id_style_t& nis = swp->default_name_id_style();

      const tokenization_t& t_tokenization = *tp;
      const vector_t<token_id_t>& t_tokens = t_tokenization.tokens;

      int rule_depth = 0;

      seed_exec_trace_t exec_trace{.swp = swp};

      const token_id_t ti_id          = *swp->token_id("identifier");
      const token_id_t ti_op          = *swp->token_id("operator");
      const token_id_t ti_string      = *swp->token_id("string");
      const token_id_t ti_number      = *swp->token_id("number");
      const token_id_t ti_any         = *swp->token_id("any");
      const token_id_t ti_eps         = *swp->token_id("epsilon");
      const token_id_t ti_eof         = *swp->token_id("end_of_file");
      const token_id_t ti_keywords_of = *swp->token_id("keywords_of");
      const token_id_t ti_ques        = *swp->token_id("?");
      const token_id_t ti_star        = *swp->token_id("*");
      const token_id_t ti_plus        = *swp->token_id("+");
      const token_id_t ti_not         = *swp->token_id("not");
      const token_id_t ti_but_then    = *swp->token_id("but_then");
      const token_id_t ti_regex       = *swp->token_id("/");
      const token_id_t ti_equal       = *swp->token_id("=");
      const token_id_t ti_alias       = *swp->token_id("=>");
      const token_id_t ti_axe         = *swp->token_id("=/");

      const name_id_t ni_seed         = swp->name_id_of("Seed");
      const name_id_t ni_rule         = swp->name_id_of(ni_seed, "Rule");
      const name_id_t ni_expr         = swp->name_id_of(ni_seed, "Expr");
      const name_id_t ni_expr_parens  = swp->name_id_of(ni_expr, "Parens");
      const name_id_t ni_expr_prefix  = swp->name_id_of(ni_expr, "Prefix");
      const name_id_t ni_expr_postfix = swp->name_id_of(ni_expr, "Postfix");
      const name_id_t ni_expr_concat  = swp->name_id_of(ni_expr, "Concat");
      const name_id_t ni_expr_or      = swp->name_id_of(ni_expr, "Or");
      const name_id_t ni_expr_and     = swp->name_id_of(ni_expr, "And");
      const name_id_t ni_atom         = swp->name_id_of(ni_seed, "Atom");
      const name_id_t ni_axe          = swp->name_id_of(ni_seed, "Axe");
      const name_id_t ni_nt_maybe_var = swp->name_id_of(ni_seed, "NonterminalMaybeVar");
      const name_id_t ni_func         = swp->name_id_of(ni_seed, "Function");
      const name_id_t ni_func_arg     = swp->name_id_of(ni_func, "Arg");
      const name_id_t ni_nt           = swp->name_id_of(ni_seed, "Nonterminal");
      const name_id_t ni_term         = swp->name_id_of(ni_seed, "Terminal");
      const name_id_t ni_var          = swp->name_id_of(ni_seed, "Variable");

      seed_engine_nursery_t(tokenization_ptr_t tp, const interpreter_t* root)
        : parse_tree_nursery_t(tp), se(root)
      {
      }

      expected_t<void> check()
      {
        SILVA_EXPECT(swp == t_tokenization.swp,
                     MAJOR,
                     "Seed and target parse-trees/tokenizations must be in same syntax_ward_t");
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

      expected_t<node_and_error_t> parse_f(const span_t<const parse_tree_span_t> params)
      {
        SILVA_EXPECT(params.size() == 2, MAJOR, "expected two arguments, got {}", params.size());
        auto pts_scope = params[0];
        if (pts_scope[0].rule_name == ni_nt_maybe_var) {
          pts_scope = pts_scope.sub_tree_span_at(1);
        }
        const auto pts_rel = params[1];
        SILVA_EXPECT(pts_scope[0].rule_name == ni_nt && pts_rel[0].rule_name == ni_nt,
                     MAJOR,
                     "expected nonterminals, got {} and {}",
                     swp->name_id_wrap(pts_scope[0].rule_name),
                     swp->name_id_wrap(pts_rel[0].rule_name));

        const name_id_t scope_name =
            SILVA_EXPECT_FWD_AS(nis.derive_name(name_id_root, pts_scope), MAJOR);
        const name_id_t t_rule_name = SILVA_EXPECT_FWD(nis.derive_name(scope_name, pts_rel));
        return handle_rule(t_rule_name);
      }
      expected_t<node_and_error_t>
      parse_and_callback_f(const span_t<const parse_tree_span_t> params)
      {
        const index_t orig_token_index = token_index;
        const index_t curr_num_nodes   = tree.size();
        node_and_error_t nae           = SILVA_EXPECT_FWD(parse_f(params));
        if (tree.size() > curr_num_nodes) {
          const parse_tree_span_t parsed_pts{&tree[curr_num_nodes], 1, tp};
          auto cb_exp = se->callback_if(parsed_pts);
          if (!cb_exp.has_value()) {
            error_nursery_t error_nursery;
            if (!nae.last_error.is_empty()) {
              error_nursery.add_child_error(std::move(nae.last_error));
            }
            error_nursery.add_child_error(std::move(cb_exp).error());
            return std::unexpected(std::move(error_nursery)
                                       .finish(MINOR,
                                               "[{}] error running callback for {}",
                                               token_position_at(orig_token_index),
                                               swp->name_id_wrap(nae.node.rule_name)));
          }
        }
        return nae;
      }
      expected_t<node_and_error_t> print_f(const span_t<const parse_tree_span_t> params)
      {
        for (const auto& pts: params) {
          fmt::print("{}\n", to_string(pts));
        }
        auto ss = stake();
        return ss.commit();
      }
      using func_t = delegate_t<expected_t<node_and_error_t>(span_t<const parse_tree_span_t>)>;
      using func_table_t      = hashmap_t<token_id_t, func_t>;
      func_table_t func_table = {
          {
              *swp->token_id("parse_f"),
              func_t::make<&seed_engine_nursery_t::parse_f>(this),
          },
          {
              *swp->token_id("parse_and_callback_f"),
              func_t::make<&seed_engine_nursery_t::parse_and_callback_f>(this),
          },
          {
              *swp->token_id("print_f"),
              func_t::make<&seed_engine_nursery_t::print_f>(this),
          },
      };

      expected_t<node_and_error_t> s_terminal(const parse_tree_span_t pts,
                                              const name_id_t t_rule_name)
      {
        auto ss            = stake();
        const auto& s_node = pts[0];
        SILVA_EXPECT(s_node.rule_name == ni_term, MAJOR);
        const token_id_t s_front_ti = pts.tp->tokens[s_node.token_begin];
        if (s_front_ti == ti_eps) {
          return ss.commit();
        }
        if (s_front_ti == ti_eof) {
          SILVA_EXPECT_PARSE(t_rule_name,
                             num_tokens_left() == 0,
                             "expected {}",
                             swp->token_id_wrap(ti_eof));
          return ss.commit();
        }
        SILVA_EXPECT_PARSE(t_rule_name,
                           num_tokens_left() > 0,
                           "Reached end of token-stream when looking for {}",
                           pts.tp->token_info_get(s_node.token_begin)->str);
        if ((s_front_ti == ti_id || s_front_ti == ti_op) && s_node.num_tokens() == 3) {
          SILVA_EXPECT(s_node.num_children == 0, MAJOR, "expected Terminal node have no children");
          if (s_front_ti == ti_id) {
            SILVA_EXPECT_PARSE(t_rule_name,
                               token_data_by()->category == IDENTIFIER,
                               "expected {}",
                               pts.token_range());
          }
          else if (s_front_ti == ti_op) {
            SILVA_EXPECT_PARSE(t_rule_name,
                               token_data_by()->category == OPERATOR,
                               "expected {}",
                               pts.token_range());
          }
          else {
            SILVA_EXPECT(false, MAJOR, "Only 'identifier' and 'operator' may have regexes");
          }
          const token_id_t regex_token_id = pts.tp->tokens[s_node.token_begin + 2];
          const auto it                   = se->regexes.find(regex_token_id);
          SILVA_EXPECT(it != se->regexes.end() && it->second.has_value(), MAJOR);
          const std::regex& re          = it->second.value();
          const string_view_t token_str = token_data_by()->str;
          const bool is_match           = std::regex_search(token_str.begin(), token_str.end(), re);
          SILVA_EXPECT_PARSE(t_rule_name,
                             is_match,
                             "{} does not match regex {}",
                             swp->token_id_wrap(token_id_by()),
                             swp->token_infos[regex_token_id].str);
        }
        else if (s_front_ti == ti_keywords_of) {
          const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
          const auto pts_nt   = pts.sub_tree_span_at(children[0]);
          const auto it       = se->nonterminal_rules.find(pts_nt);
          SILVA_EXPECT(it != se->nonterminal_rules.end(), MAJOR, "Couldn't lookup nonterminal");
          const name_id_t keyword_scope = it->second;
          const auto it2                = se->keyword_scopes.find(keyword_scope);
          SILVA_EXPECT(it2 != se->keyword_scopes.end(),
                       MAJOR,
                       "keywords_of {}: no such nonterminal",
                       swp->name_id_wrap(keyword_scope));
          const hashset_t<token_id_t>& keywords = it2->second;
          SILVA_EXPECT(keywords.contains(token_id_by()),
                       MINOR,
                       "{} '{}' not in keywords_of {}",
                       token_position_by(),
                       swp->token_id_wrap(token_id_by()),
                       swp->name_id_wrap(keyword_scope));
        }
        else {
          SILVA_EXPECT(s_node.num_children == 0, MAJOR, "Expected Terminal node have no children");
          SILVA_EXPECT(s_node.num_tokens() == 1,
                       MAJOR,
                       "Terminal nodes must have one or three tokens");
          if (s_front_ti == ti_id) {
            SILVA_EXPECT_PARSE(t_rule_name,
                               token_data_by()->category == IDENTIFIER,
                               "expected identifier");
          }
          else if (s_front_ti == ti_op) {
            SILVA_EXPECT_PARSE(t_rule_name,
                               token_data_by()->category == OPERATOR,
                               "expected operator");
          }
          else if (s_front_ti == ti_string) {
            SILVA_EXPECT_PARSE(t_rule_name, token_data_by()->category == STRING, "expected string");
          }
          else if (s_front_ti == ti_number) {
            SILVA_EXPECT_PARSE(t_rule_name, token_data_by()->category == NUMBER, "expected number");
          }
          else if (s_front_ti == ti_any) {
            ;
          }
          else {
            const index_t s_token_id = pts.tp->tokens[s_node.token_begin];
            const auto it            = se->string_to_keyword.find(s_token_id);
            SILVA_EXPECT(it != se->string_to_keyword.end(), MAJOR, "Couldn't find keyword");
            const token_id_t t_expected_ti = it->second;
            SILVA_EXPECT_PARSE(t_rule_name,
                               token_id_by() == t_expected_ti,
                               "expected {}",
                               swp->token_id_wrap(t_expected_ti));
          }
        }
        token_index += 1;
        return ss.commit();
      }

      expected_t<node_and_error_t>
      s_expr_prefix(const parse_tree_span_t pts, const name_id_t t_rule_name, var_map_t& var_map)
      {
        {
          auto ss             = stake();
          const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
          auto result =
              SILVA_EXPECT_FWD_IF(s_expr(pts.sub_tree_span_at(children[0]), t_rule_name, var_map),
                                  MAJOR);
          SILVA_EXPECT(!result, MINOR, "Successfully parsed 'not' expression");
        }
        auto ss = stake();
        return ss.commit();
      }

      expected_t<pair_t<index_t, index_t>> get_min_max_repeat(const token_id_t op_ti)
      {
        index_t min_repeat = 0;
        index_t max_repeat = std::numeric_limits<index_t>::max();
        SILVA_EXPECT(op_ti == ti_ques || op_ti == ti_star || op_ti == ti_plus, MAJOR);
        if (op_ti == ti_ques) {
          max_repeat = 1;
        }
        else if (op_ti == ti_star) {
          ;
        }
        else if (op_ti == ti_plus) {
          min_repeat = 1;
        }
        return {pair_t{min_repeat, max_repeat}};
      }

      expected_t<node_and_error_t>
      s_expr_postfix(const parse_tree_span_t pts, const name_id_t t_rule_name, var_map_t& var_map)
      {
        auto ss                             = stake();
        const auto children                 = SILVA_EXPECT_FWD(pts.get_children<1>());
        const token_id_t op_ti              = swp->name_infos[pts[0].rule_name].base_name;
        const auto [min_repeat, max_repeat] = SILVA_EXPECT_FWD(get_min_max_repeat(op_ti));
        index_t repeat_count                = 0;
        error_t last_error;
        while (repeat_count < max_repeat) {
          auto result =
              SILVA_EXPECT_FWD_IF(s_expr(pts.sub_tree_span_at(children[0]), t_rule_name, var_map),
                                  MAJOR);
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
          small_vector_t<error_t, 1> maybe_child_error;
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

      expected_t<node_and_error_t>
      s_expr_concat(const parse_tree_span_t pts, const name_id_t t_rule_name, var_map_t& var_map)
      {
        const index_t orig_token_index = token_index;
        auto ss                        = stake();
        error_nursery_t error_nursery;

        // Could do this bit ahead of time and store a map in the interpreter_t.
        index_t lead_terminals = 0;
        for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
          const auto sub_pts = pts.sub_tree_span_at(sub_s_node_index);
          if (sub_pts[0].rule_name == ni_term) {
            lead_terminals += 1;
          }
          else {
            break;
          }
        }

        for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
          const auto sub_pts = pts.sub_tree_span_at(sub_s_node_index);
          auto result        = s_expr(sub_pts, t_rule_name, var_map);
          if (result.has_value()) {
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
                                               token_position_at(orig_token_index),
                                               swp->name_id_wrap(t_rule_name),
                                               pts.token_range()));
          }
        }
        return ss.commit();
      }

      expected_t<node_and_error_t>
      s_expr_and(const parse_tree_span_t pts, const name_id_t t_rule_name, var_map_t& var_map)
      {
        optional_t<stake_t> ss;
        for (const auto [child_node_index, child_index]: pts.children_range()) {
          ss.emplace(stake());
          auto result = SILVA_EXPECT_FWD(
              s_expr(pts.sub_tree_span_at(child_node_index), t_rule_name, var_map));
          ss->add_proto_node(std::move(result).as_node());
        }
        SILVA_EXPECT(ss.has_value(), MAJOR);
        return ss->commit();
      }

      expected_t<node_and_error_t>
      s_expr_or(const parse_tree_span_t pts, const name_id_t t_rule_name, var_map_t& var_map)
      {
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;
        optional_t<parse_tree_node_t> retval;
        error_level_t error_level = MINOR;
        for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
          auto result = s_expr(pts.sub_tree_span_at(sub_s_node_index), t_rule_name, var_map);
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
                                           token_position_at(orig_token_index),
                                           swp->name_id_wrap(t_rule_name),
                                           pts.token_range()));
      }

      expected_t<node_and_error_t> s_nonterminal_maybe_var(const parse_tree_span_t pts,
                                                           const name_id_t t_rule_name,
                                                           var_map_t& var_map)
      {
        SILVA_EXPECT(pts[0].num_children == 1 || pts[0].num_children == 2,
                     MAJOR,
                     "{} expected one or two children",
                     pts);
        auto [it, end]   = pts.children_range();
        const auto nt_it = se->nonterminal_rules.find(pts.sub_tree_span_at(it.pos));
        ++it;
        SILVA_EXPECT(nt_it != se->nonterminal_rules.end(),
                     MAJOR,
                     "{} couldn't lookup nonterminal",
                     pts);
        const name_id_t next_t_rule_name = nt_it->second;
        const index_t parsed_tree_index  = tree.size();
        expected_t<node_and_error_t> retval =
            SILVA_EXPECT_FWD_IF(handle_rule(next_t_rule_name), MAJOR);
        if (it != end) {
          const auto pts_var = pts.sub_tree_span_at(it.pos);
          const auto var_ti  = pts_var.first_token_id();
          var_map[var_ti]    = parsed_tree_index;
        }
        return retval;
      }

      expected_t<node_and_error_t>
      s_func(const parse_tree_span_t pts, const name_id_t t_rule_name, var_map_t& var_map)
      {
        const auto children      = SILVA_EXPECT_FWD(pts.get_children<2>());
        const token_id_t func_ti = pts.sub_tree_span_at(children[0]).first_token_id();
        const auto f_it          = func_table.find(func_ti);
        SILVA_EXPECT(f_it != func_table.end(), MAJOR);
        const auto& func = f_it->second;

        const auto pts_args = pts.sub_tree_span_at(children[1]);
        vector_t<parse_tree_span_t> args;
        for (const auto [child_node_index, child_index]: pts_args.children_range()) {
          const auto pts_arg = pts_args.sub_tree_span_at(child_node_index);
          if (pts_arg[0].rule_name == ni_var) {
            const token_id_t given_var = pts_arg.first_token_id();
            const auto v_it            = var_map.find(given_var);
            SILVA_EXPECT(v_it != var_map.end(), MAJOR);
            args.push_back(parse_tree_span_t{&tree[v_it->second], 1, tp});
          }
          else {
            args.push_back(pts_arg);
          }
        }
        return func(args);
      }

      expected_t<node_and_error_t>
      s_expr(const parse_tree_span_t pts, const name_id_t t_rule_name, var_map_t& var_map)
      {
        const name_id_t s_rule_name = pts[0].rule_name;
        if (swp->name_id_is_parent(ni_expr_parens, s_rule_name)) {
          const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
          return s_expr(pts.sub_tree_span_at(children[0]), t_rule_name, var_map);
        }
        else if (swp->name_id_is_parent(ni_expr_prefix, s_rule_name)) {
          return s_expr_prefix(pts, t_rule_name, var_map);
        }
        else if (swp->name_id_is_parent(ni_expr_postfix, s_rule_name)) {
          return s_expr_postfix(pts, t_rule_name, var_map);
        }
        else if (swp->name_id_is_parent(ni_expr_concat, s_rule_name)) {
          return s_expr_concat(pts, t_rule_name, var_map);
        }
        else if (swp->name_id_is_parent(ni_expr_and, s_rule_name)) {
          return s_expr_and(pts, t_rule_name, var_map);
        }
        else if (swp->name_id_is_parent(ni_expr_or, s_rule_name)) {
          return s_expr_or(pts, t_rule_name, var_map);
        }
        else if (s_rule_name == ni_term) {
          return s_terminal(pts, t_rule_name);
        }
        else if (s_rule_name == ni_nt_maybe_var) {
          return s_nonterminal_maybe_var(pts, t_rule_name, var_map);
        }
        else if (s_rule_name == ni_func) {
          return s_func(pts, t_rule_name, var_map);
        }
        else {
          SILVA_EXPECT(false, MAJOR, "unknown seed expression {}", pts);
        }
      }

      expected_t<node_and_error_t> handle_rule_axe(const name_id_t t_rule_name)
      {
        const auto it = se->seed_axes.find(t_rule_name);
        SILVA_EXPECT(it != se->seed_axes.end(), MAJOR);
        auto ss{stake()};
        const seed_axe_t& seed_axe = it->second;
        const seed_axe_t::parse_delegate_t::pack_t pack{
            [&](const name_id_t rule_name) -> expected_t<parse_tree_node_t> {
              node_and_error_t result = SILVA_EXPECT_FWD(handle_rule(rule_name));
              return std::move(result).as_node();
            },
        };
        ss.add_proto_node(
            SILVA_EXPECT_PARSE_FWD(t_rule_name, seed_axe.apply(*this, pack.delegate)));
        return ss.commit();
      }

      expected_t<node_and_error_t> handle_rule(const name_id_t t_rule_name)
      {
        auto ets = SILVA_EXEC_TRACE_SCOPE(exec_trace, t_rule_name, token_position_by());
        rule_depth += 1;
        scope_exit_t scope_exit([this] { rule_depth -= 1; });
        SILVA_EXPECT(rule_depth <= 100,
                     FATAL,
                     "Stack is getting too deep. Infinite recursion in grammar?");
        const index_t orig_token_index = token_index;
        const auto it{se->rule_exprs.find(t_rule_name)};
        SILVA_EXPECT(it != se->rule_exprs.end(),
                     MAJOR,
                     "Unknown rule: {}",
                     nis.absolute(t_rule_name));
        const parse_tree_span_t pts = it->second;
        const auto& s_node          = pts[0];
        const name_id_t s_expr_name = s_node.rule_name;
        node_and_error_t retval;
        if (s_expr_name == ni_axe) {
          retval = SILVA_EXPECT_PARSE_FWD(t_rule_name, handle_rule_axe(t_rule_name));
        }
        else {
          const token_id_t rule_token = pts.tp->tokens[s_node.token_begin];
          SILVA_EXPECT(rule_token == ti_equal || rule_token == ti_alias,
                       MAJOR,
                       "expected one of [ '=' '=>' ], got {} at {}",
                       swp->token_id_wrap(rule_token),
                       pts);
          const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
          var_map_t var_map;
          if (rule_token == ti_equal) {
            auto ss_rule = stake();
            ss_rule.create_node(t_rule_name);
            auto result = SILVA_EXPECT_PARSE_FWD(
                t_rule_name,
                s_expr(pts.sub_tree_span_at(children[0]), t_rule_name, var_map));
            ss_rule.add_proto_node(std::move(result.node));
            retval = node_and_error_t{ss_rule.commit(), std::move(result.last_error)};
          }
          else {
            auto ss     = stake();
            auto result = SILVA_EXPECT_PARSE_FWD(
                t_rule_name,
                s_expr(pts.sub_tree_span_at(children[0]), t_rule_name, var_map));
            ss.add_proto_node(std::move(result.node));
            retval = node_and_error_t{ss.commit(), std::move(result.last_error)};
          }
        }
        ets->success = true;
        return retval;
      }
    };
  }

  expected_t<parse_tree_ptr_t> interpreter_t::apply(tokenization_ptr_t tp,
                                                    const name_id_t goal_rule_name) const
  {
    impl::seed_engine_nursery_t nursery(tp, this);
    SILVA_EXPECT_FWD(nursery.check());
    auto ptn = SILVA_EXPECT_FWD(nursery.handle_rule(goal_rule_name),
                                "seed::interpreter_t::apply({}) failed to parse",
                                tp->swp->name_id_wrap(goal_rule_name));
    if (ptn.node.token_begin != 0 || ptn.node.token_end != tp->tokens.size()) {
      SILVA_EXPECT(!ptn.last_error.is_empty(),
                   MAJOR,
                   "could not parse entire tokenization of {}",
                   tp->filepath);
      return std::unexpected(std::move(ptn.last_error));
    }
    SILVA_EXPECT(ptn.node.num_children == 1, ASSERT);
    SILVA_EXPECT(ptn.node.subtree_size == nursery.tree.size(), ASSERT);

    if (SILVA_EXPECT_FWD_IF(var_context_get_as<bool>("SEED_EXEC_TRACE"), MAJOR).value_or(false)) {
      fmt::print("{}", SILVA_EXPECT_FWD(std::move(nursery.exec_trace).as_tree_to_string()));
    }

    return tp->swp->add(std::move(nursery).finish());
  }
}
