#include "parse_root.hpp"

#include "canopy/scope_exit.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

#include <utility>

namespace silva {
  using enum token_category_t;
  using enum seed_rule_t;
  using enum error_level_t;

  namespace impl {
    expected_t<void> create_add_rule(parse_root_t& retval,
                                     const token_id_t rule_token_id,
                                     const string_view_t rule_name,
                                     const index_t precedence,
                                     const index_t expr_node_index)
    {
      if (retval.rules.empty()) {
        retval.goal_rule_token_id = rule_token_id;
      }
      if (precedence > 0) {
        SILVA_EXPECT(
            !retval.rules.empty() && retval.rules.back().token_id == rule_token_id &&
                retval.rules.back().precedence + 1 == precedence,
            MAJOR,
            "Rule with positive precedence did not follow rule with the same name and preceeding "
            "precedence");
      }
      const index_t offset = retval.rules.size();
      retval.rules.push_back(parse_root_t::rule_t{
          .token_id        = rule_token_id,
          .name            = rule_name,
          .precedence      = precedence,
          .expr_node_index = expr_node_index,
      });
      if (precedence == 0) {
        const auto [it, inserted] = retval.rule_indexes.emplace(rule_token_id, offset);
        SILVA_EXPECT(
            inserted,
            MAJOR,
            "Repeated rule name that was not consecutive with linearly increasing precedence");
      }
      return {};
    }
  }

  expected_t<parse_root_t> parse_root_t::create(const_ptr_t<parse_tree_t> seed_parse_tree)
  {
    parse_root_t retval;
    retval.seed_parse_tree    = std::move(seed_parse_tree);
    const parse_tree_t* s_pt  = retval.seed_parse_tree.get();
    const parse_root_t* s_ptr = retval.seed_parse_tree->root.get();
    const auto& s_nodes       = s_pt->nodes;
    SILVA_EXPECT(s_ptr == seed_parse_root_primordial() || s_ptr == seed_parse_root(),
                 ASSERT,
                 "Not a Seed parse-tree");
    SILVA_EXPECT(!s_nodes.empty() && s_nodes.front().rule_index == to_int(SEED),
                 MINOR,
                 "Seed parse_tree should start with SEED node");

    auto result = s_pt->visit_children(
        [&](const index_t rule_node_index, index_t) -> expected_t<bool> {
          SILVA_EXPECT(s_nodes[rule_node_index].rule_index == to_int(RULE), MINOR, "");
          const small_vector_t<index_t, 3> children =
              SILVA_EXPECT_FWD(s_pt->get_children_up_to<3>(rule_node_index));
          SILVA_EXPECT(s_nodes[children[0]].rule_index == to_int(NONTERMINAL),
                       MINOR,
                       "First child of RULE must be NONTERMINAL ");
          const token_id_t rule_token_id =
              s_pt->tokenization->tokens[s_nodes[children[0]].token_index];
          const string_view_t rule_name = s_pt->tokenization->token_datas[rule_token_id].str;
          index_t rule_precedence       = 0;
          if (children.size == 3) {
            SILVA_EXPECT(s_nodes[children[1]].rule_index == to_int(RULE_PRECEDENCE),
                         MINOR,
                         "Middle child of RULE must be RULE_PRECEDENCE");
            const auto* token_data =
                s_pt->tokenization->token_data(s_nodes[children[1]].token_index);
            rule_precedence = SILVA_EXPECT_FWD(token_data->as_double());
          }
          const index_t ri = s_nodes[children.back()].rule_index;
          SILVA_EXPECT(to_int(DERIVATION_0) <= ri && ri <= to_int(DERIVATION_2),
                       MINOR,
                       "Last child of RULE must be DERIVATION_{0,1,2}");
          index_t expr_node_index = children.back();
          SILVA_EXPECT_FWD(impl::create_add_rule(retval,
                                                 rule_token_id,
                                                 rule_name,
                                                 rule_precedence,
                                                 expr_node_index));
          return true;
        },
        0);
    SILVA_EXPECT_FWD(std::move(result));

    auto result_2 = s_pt->visit_children(
        [&](const index_t rule_node_index, index_t) -> expected_t<bool> {
          SILVA_EXPECT(s_nodes[rule_node_index].rule_index == to_int(RULE), MINOR, "");
          const small_vector_t<index_t, 3> rule_children =
              SILVA_EXPECT_FWD(s_pt->get_children_up_to<3>(rule_node_index));
          if (s_nodes[rule_children.back()].rule_index == to_int(DERIVATION_2)) {
            const small_vector_t<index_t, 2> alias_children =
                SILVA_EXPECT_FWD(s_pt->get_children_up_to<2>(rule_children.back()));
            SILVA_EXPECT(s_nodes[alias_children[0]].rule_index == to_int(NONTERMINAL),
                         MINOR,
                         "First child of DERIVATION_2 must be NONTERMINAL");
            const token_id_t tgt_rule_token_id =
                s_pt->tokenization->tokens[s_nodes[alias_children[0]].token_index];
            index_t tgt_rule_precedence = 0;
            if (alias_children.size == 2) {
              SILVA_EXPECT(s_nodes[alias_children[1]].rule_index == to_int(RULE_PRECEDENCE),
                           MINOR,
                           "Second child of DERIVATION_2 must be RULE_PRECEDENCE");
              const auto* token_data =
                  s_pt->tokenization->token_data(s_nodes[alias_children[1]].token_index);
              tgt_rule_precedence = SILVA_EXPECT_FWD(token_data->as_double());
            }

            const token_id_t base_rule_token_id =
                s_pt->tokenization->tokens[s_nodes[rule_children[0]].token_index];
            index_t base_rule_precedence = 0;
            if (rule_children.size == 3) {
              SILVA_EXPECT(s_nodes[rule_children[1]].rule_index == to_int(RULE_PRECEDENCE),
                           MINOR,
                           "Middle child of RULE must be RULE_PRECEDENCE");
              const auto* token_data =
                  s_pt->tokenization->token_data(s_nodes[rule_children[1]].token_index);
              base_rule_precedence = SILVA_EXPECT_FWD(token_data->as_double());
            }

            const index_t base_offset =
                retval.rule_indexes.at(base_rule_token_id) + base_rule_precedence;
            const index_t alias_offset =
                retval.rule_indexes.at(tgt_rule_token_id) + tgt_rule_precedence;
            retval.rules[base_offset].aliased_rule_offset = alias_offset;
          }
          return true;
        },
        0);
    SILVA_EXPECT_FWD(std::move(result_2));

    for (index_t i = 0; i < retval.rules.size(); ++i) {
      if (retval.rules[i].aliased_rule_offset.has_value()) {
        const auto& rule  = retval.rules[i];
        const index_t aro = rule.aliased_rule_offset.value();
        SILVA_EXPECT(aro < retval.rules.size(), MINOR, "Invalid rule-offset {}", aro);
        const auto& alias_rule = retval.rules[aro];
        SILVA_EXPECT(!alias_rule.aliased_rule_offset.has_value(),
                     MINOR,
                     "Rule (={},{}) cannot be alias to another alias-rule (={},{})",
                     retval.seed_parse_tree->tokenization->token_datas[rule.token_id].str,
                     rule.precedence,
                     retval.seed_parse_tree->tokenization->token_datas[alias_rule.token_id].str,
                     alias_rule.precedence);
      }
    }

    for (index_t node_index = 0; node_index < s_pt->nodes.size(); ++node_index) {
      const auto& node = s_pt->nodes[node_index];
      if (node.rule_index == to_int(REGEX)) {
        const token_id_t regex_token_id = s_pt->tokenization->tokens[node.token_index];
        if (auto& regex = retval.regexes[regex_token_id]; !regex.has_value()) {
          const auto& regex_td = s_pt->tokenization->token_data(node.token_index);
          regex                = std::regex(regex_td->as_string());
        }
      }
    }
    return retval;
  }

  expected_t<parse_root_t> parse_root_t::create(const_ptr_t<source_code_t> source_code)
  {
    auto tokenization = SILVA_EXPECT_FWD(tokenize(std::move(source_code)));
    auto fern_seed_pt = SILVA_EXPECT_FWD(seed_parse(to_unique_ptr(std::move(tokenization))));
    auto retval       = SILVA_EXPECT_FWD(create(to_unique_ptr(std::move(fern_seed_pt))));
    return retval;
  }

  optional_t<index_t> parse_root_t::workspace_t::per_seed_token_id_t::get_target_token_id(
      const tokenization_t::token_data_t* sp_token_data,
      const tokenization_t* target_tokenization)
  {
    SILVA_ASSERT(sp_token_data->category == STRING);
    if (std::holds_alternative<uncached_t>(target_token_id)) {
      const string_or_view_t seed_terminal = sp_token_data->as_string_or_view();
      const optional_t<index_t> result =
          target_tokenization->lookup_token(seed_terminal.get_view());
      if (result) {
        target_token_id = result.value();
      }
      else {
        target_token_id = none;
      }
    }
    if (std::holds_alternative<none_t>(target_token_id)) {
      return none;
    }
    else {
      SILVA_ASSERT(std::holds_alternative<index_t>(target_token_id));
      return std::get<index_t>(target_token_id);
    }
  }

  namespace impl {
    struct parse_root_nursery_t : public parse_tree_nursery_t {
      const parse_tree_t* seed_pt          = nullptr;
      parse_root_t::workspace_t* workspace = nullptr;

      int rule_depth = 0;

      optional_t<token_id_t> seed_tt_id  = seed_pt->tokenization->lookup_token("identifier");
      optional_t<token_id_t> seed_tt_op  = seed_pt->tokenization->lookup_token("operator");
      optional_t<token_id_t> seed_tt_str = seed_pt->tokenization->lookup_token("string");
      optional_t<token_id_t> seed_tt_num = seed_pt->tokenization->lookup_token("number");
      optional_t<token_id_t> seed_tt_any = seed_pt->tokenization->lookup_token("any");

      parse_root_nursery_t(const_ptr_t<tokenization_t> tokenization,
                           const_ptr_t<parse_root_t> parse_root,
                           parse_root_t::workspace_t* workspace)
        : parse_tree_nursery_t(std::move(tokenization), std::move(parse_root))
        , seed_pt(retval.root->seed_parse_tree.get())
        , workspace(workspace)
      {
        const index_t n = seed_pt->tokenization->token_datas.size();
        workspace->seed_token_id_data.assign(n, {});
      }

      expected_t<parse_tree_sub_t> apply_terminal(const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        const auto& seed_node = seed_pt->nodes[seed_node_index];
        SILVA_EXPECT_PARSE(token_index < retval.tokenization->tokens.size(),
                           "Reached end of token-stream when looking for {}",
                           seed_pt->tokenization->token_data(seed_node.token_index)->str);
        if (seed_node.rule_index == to_int(TERMINAL_0)) {
          SILVA_EXPECT(seed_node.num_children == 1,
                       MAJOR,
                       "Expected Seed node TERIMNAL_0 to have exactly one child");
          const array_t<index_t, 1> seed_node_index_regex =
              SILVA_EXPECT_FWD(seed_pt->get_children<1>(seed_node_index));
          const auto& seed_regex_node = seed_pt->nodes[seed_node_index_regex[0]];
          const token_id_t regex_token_id =
              seed_pt->tokenization->tokens[seed_regex_node.token_index];
          const auto it = retval.root->regexes.find(regex_token_id);
          SILVA_EXPECT(it != retval.root->regexes.end() || !it->second.has_value(), FATAL);
          SILVA_EXPECT_PARSE(token_data_by()->category == IDENTIFIER, "Expected identifier");
          const std::regex& re          = it->second.value();
          const string_view_t token_str = token_data_by()->str;
          const bool is_match           = std::regex_search(token_str.begin(), token_str.end(), re);
          SILVA_EXPECT_PARSE(is_match,
                             "Token \"{}\" does not match regex {}",
                             token_str,
                             seed_pt->tokenization->token_datas[regex_token_id].str);
        }
        else {
          SILVA_EXPECT(seed_node.rule_index == to_int(TERMINAL_1),
                       MAJOR,
                       "Expected Seed node TERMINAL_1");
          const token_id_t seed_token_id = seed_pt->tokenization->tokens[seed_node.token_index];
          if (seed_token_id == seed_tt_id) {
            SILVA_EXPECT_PARSE(token_data_by()->category == IDENTIFIER, "Expected identifier");
          }
          else if (seed_token_id == seed_tt_op) {
            SILVA_EXPECT_PARSE(token_data_by()->category == OPERATOR, "Expected operator");
          }
          else if (seed_token_id == seed_tt_str) {
            SILVA_EXPECT_PARSE(token_data_by()->category == STRING, "Expected string");
          }
          else if (seed_token_id == seed_tt_num) {
            SILVA_EXPECT_PARSE(token_data_by()->category == NUMBER, "Expected number");
          }
          else if (seed_token_id == seed_tt_any) {
            ;
          }
          else {
            const auto* sp_token_data = seed_pt->tokenization->token_data(seed_node.token_index);
            SILVA_EXPECT(sp_token_data->category == STRING, MAJOR);
            auto& seed_token_id_work = workspace->seed_token_id_data[seed_token_id];
            const auto expected_target_token_id =
                seed_token_id_work.get_target_token_id(sp_token_data, retval.tokenization.get());
            SILVA_EXPECT_PARSE(token_id_by() == expected_target_token_id,
                               "Expected {}",
                               sp_token_data->str);
          }
        }
        token_index += 1;
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_primary(const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        const auto& seed_node = seed_pt->nodes[seed_node_index];
        if (seed_node.rule_index == to_int(PRIMARY_0)) {
          gg.sub += SILVA_EXPECT_FWD(apply_derivation_0("subexpression", seed_node_index));
        }
        else if (seed_node.rule_index == to_int(PRIMARY_1)) {
          const array_t<index_t, 1> terminal_child =
              SILVA_EXPECT_FWD(seed_pt->get_children<1>(seed_node_index));
          gg.sub += SILVA_EXPECT_FWD(apply_terminal(terminal_child[0]));
        }
        else if (seed_node.rule_index == to_int(PRIMARY_2)) {
          const array_t<index_t, 1> nonterminal_child =
              SILVA_EXPECT_FWD(seed_pt->get_children<1>(seed_node_index));
          const auto& nonterminal_node = seed_pt->nodes[nonterminal_child[0]];
          gg.sub += SILVA_EXPECT_FWD(
              apply_rule(seed_pt->tokenization->tokens[nonterminal_node.token_index]));
        }
        else {
          SILVA_EXPECT(false, MAJOR);
        }
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_derivation_1(const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        bool found_match = false;
        error_nursery_t error_nursery;
        auto result = seed_pt->visit_children(
            [&](const index_t node_index, const index_t) -> expected_t<bool> {
              const auto& node = seed_pt->nodes[node_index];
              if (auto result = apply_terminal(node_index); result) {
                found_match = true;
                gg.sub += *std::move(result);
                return false;
              }
              else {
                error_nursery.add_child_error(
                    SILVA_EXPECT_FWD_IF(std::move(result), MAJOR).error());
              }
              return true;
            },
            seed_node_index);
        SILVA_EXPECT_FWD(std::move(result));
        if (!found_match) {
          return std::unexpected(std::move(error_nursery)
                                     .finish(MINOR,
                                             "{} Expected to parse '{{...}}'",
                                             token_position_at(gg.orig_token_index)));
        }
        return gg.release();
      }

      std::pair<index_t, index_t> get_min_max_repeat(const char op)
      {
        index_t min_repeat = 0;
        index_t max_repeat = std::numeric_limits<index_t>::max();
        if (op == '?') {
          max_repeat = 1;
        }
        else if (op == '*') {
          ;
        }
        else if (op == '+') {
          min_repeat = 1;
        }
        return {min_repeat, max_repeat};
      }

      expected_t<parse_tree_sub_t> apply_derivation_0(string_view_t expr_name,
                                                      const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        error_nursery_t error_nursery;
        error_level_t min_error_level = NONE;
        auto result                   = seed_pt->visit_children(
            [&](const index_t seed_node_index_atom, const index_t) -> expected_t<bool> {
              const auto& seed_node_atom = seed_pt->nodes[seed_node_index_atom];
              if (seed_node_atom.rule_index == to_int(ATOM_0)) {
                min_error_level = MAJOR;
                return true;
              }
              SILVA_EXPECT(seed_node_atom.rule_index == to_int(ATOM_1),
                           MAJOR,
                           "Expected Atom in Seed parse-tree");
              optional_t<char> suffix_char;
              const small_vector_t<index_t, 2> children =
                  SILVA_EXPECT_FWD(seed_pt->get_children_up_to<2>(seed_node_index_atom));
              if (children.size == 2) {
                const auto& seed_node_suffix = seed_pt->nodes[children[1]];
                SILVA_EXPECT(seed_node_suffix.rule_index == to_int(SUFFIX), MAJOR);
                const string_view_t suffix_op =
                    seed_pt->tokenization->token_data(seed_node_suffix.token_index)->str;
                SILVA_EXPECT(suffix_op.size() == 1, MAJOR);
                suffix_char = suffix_op.front();
              }
              else {
                SILVA_EXPECT(children.size == 1, MAJOR, "Atom had unexpected number of children");
              }
              const index_t seed_node_index_primary = children[0];

              if (!suffix_char) {
                gg.sub += SILVA_EXPECT_FWD(apply_primary(seed_node_index_primary));
              }
              else if (suffix_char.value() == '?' || suffix_char.value() == '*' ||
                       suffix_char.value() == '+') {
                const auto [min_repeat, max_repeat] = get_min_max_repeat(suffix_char.value());
                parse_tree_sub_t sub_sub;
                index_t repeat_count = 0;
                while (repeat_count < max_repeat) {
                  if (auto result = apply_primary(seed_node_index_primary); result) {
                    sub_sub += *std::move(result);
                    repeat_count += 1;
                  }
                  else {
                    error_nursery.add_child_error(
                        SILVA_EXPECT_FWD_IF(std::move(result), MAJOR).error());
                    break;
                  }
                }
                SILVA_EXPECT_PARSE(min_repeat <= repeat_count,
                                   "min-repeat (={}) not reached, only found {}",
                                   min_repeat,
                                   repeat_count);
                gg.sub += std::move(sub_sub);
              }
              else if (suffix_char.value() == '!') {
                parse_tree_guard_t inner_ptg{&retval, &token_index};
                const auto result =
                    SILVA_EXPECT_FWD_IF(apply_primary(seed_node_index_primary), MAJOR);
                SILVA_EXPECT(!result, MINOR, "Managed to parse negative primary expression");
              }
              else if (suffix_char.value() == '&') {
                parse_tree_guard_t inner_ptg{&retval, &token_index};
                const auto result =
                    SILVA_EXPECT_FWD_IF(apply_primary(seed_node_index_primary), MAJOR);
                SILVA_EXPECT(result, MINOR, "Did not manage to parse positive primary expression");
              }
              else {
                SILVA_EXPECT(false, MAJOR);
              }
              return true;
            },
            seed_node_index);
        if (!result) {
          const error_level_t el = std::max(result.error().level, min_error_level);
          error_nursery.add_child_error(std::move(result).error());
          return std::unexpected(std::move(error_nursery)
                                     .finish(el,
                                             "{} Expected {} expr",
                                             token_position_at(gg.orig_token_index),
                                             expr_name));
        }
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_expr(const parse_root_t::rule_t& rule)
      {
        const parse_tree_t::node_t& seed_node_expr = seed_pt->nodes[rule.expr_node_index];
        if (seed_node_expr.rule_index == to_int(DERIVATION_0)) {
          return apply_derivation_0(rule.name, rule.expr_node_index);
        }
        else if (seed_node_expr.rule_index == to_int(DERIVATION_1)) {
          return apply_derivation_1(rule.expr_node_index);
        }
        else {
          SILVA_EXPECT(false, MAJOR, "Expected Seed node DERIVATION_0 or DERIVATION_1");
        }
      }

      expected_t<parse_tree_sub_t> apply_rule(const token_id_t rule_token_id)
      {
        rule_depth += 1;
        scope_exit_t scope_exit([this] { rule_depth -= 1; });
        SILVA_EXPECT(rule_depth <= 50,
                     FATAL,
                     "Stack is getting to deep. Do you have an infinite loop in your grammar?");
        const index_t orig_token_index = token_index;
        const auto it{retval.root->rule_indexes.find(rule_token_id)};
        SILVA_EXPECT(it != retval.root->rule_indexes.end(),
                     MAJOR,
                     "Unknown rule-token-id '{}'",
                     rule_token_id);
        const index_t base_rule_offset = it->second;
        index_t rule_offset            = it->second;
        error_nursery_t error_nursery;
        error_level_t retval_error_level = MINOR;
        while (rule_offset < retval.root->rules.size()) {
          const parse_root_t::rule_t& rule = retval.root->rules[rule_offset];
          if (rule.precedence != rule_offset - base_rule_offset) {
            break;
          }
          parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
          const index_t aro =
              retval.root->rules[rule_offset].aliased_rule_offset.value_or(rule_offset);
          const auto& aliased_rule = retval.root->rules[aro];
          gg_rule.set_rule_index(aro);
          if (auto result = apply_expr(aliased_rule); result) {
            gg_rule.sub += *std::move(result);
            return gg_rule.release();
          }
          else {
            const error_level_t el = result.error().level;
            error_nursery.add_child_error(std::move(result).error());
            if (el >= MAJOR) {
              retval_error_level = MAJOR;
              break;
            }
          }
          rule_offset += 1;
        }
        return std::unexpected(std::move(error_nursery)
                                   .finish_short(retval_error_level,
                                                 "{} Expected {} rule",
                                                 token_position_at(orig_token_index),
                                                 retval.root->rules[rule_offset].name));
      }
    };
  }

  expected_t<parse_tree_t> parse_root_t::apply(const_ptr_t<tokenization_t> tokenization,
                                               parse_root_t::workspace_t* workspace) const
  {
    expected_traits_t expected_traits{.materialize_fwd = true};
    optional_t<parse_root_t::workspace_t> local_workspace;
    if (workspace == nullptr) {
      local_workspace.emplace();
      workspace = &(*local_workspace);
    }
    impl::parse_root_nursery_t parse_root_nursery(std::move(tokenization),
                                                  const_ptr_unowned(this),
                                                  workspace);
    const parse_tree_sub_t sub =
        SILVA_EXPECT_FWD(parse_root_nursery.apply_rule(goal_rule_token_id));
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.num_children_total == parse_root_nursery.retval.nodes.size(), ASSERT);
    return {std::move(parse_root_nursery.retval)};
  }
}
