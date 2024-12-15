#include "parse_root.hpp"

#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

#include <utility>

namespace silva {
  using enum token_category_t;
  using enum seed_rule_t;

  expected_t<void> parse_root_t::add_rule(const string_view_t rule_name,
                                          const index_t precedence,
                                          const index_t expr_node_index)
  {
    if (rules.empty()) {
      goal_rule_name = rule_name;
    }
    if (precedence > 0) {
      SILVA_EXPECT_FMT(
          !rules.empty() && rules.back().name == rule_name &&
              rules.back().precedence + 1 == precedence,
          "Rule with positive precedence did not follow rule with the same name and preceeding "
          "precedence");
    }
    const index_t offset = rules.size();
    rules.push_back(rule_t{
        .name            = rule_name,
        .precedence      = precedence,
        .expr_node_index = expr_node_index,
    });
    if (precedence == 0) {
      const auto [it, inserted] = rule_name_offsets.emplace(rule_name, offset);
      SILVA_EXPECT_FMT(
          inserted,
          "Repeated rule name that was not consecutive with linearly increasing precedence");
    }
    return {};
  }

  expected_t<parse_root_t> parse_root_t::create(const_ptr_t<parse_tree_t> seed_parse_tree)
  {
    parse_root_t retval;
    retval.seed_parse_tree    = std::move(seed_parse_tree);
    const parse_tree_t* s_pt  = retval.seed_parse_tree.get();
    const parse_root_t* s_ptr = retval.seed_parse_tree->root.get();
    const auto& s_nodes       = s_pt->nodes;
    SILVA_EXPECT_FMT(s_ptr == seed_parse_root_primordial() || s_ptr == seed_parse_root(),
                     "Not a seed parse_tree");
    SILVA_EXPECT_FMT(!s_nodes.empty() && s_nodes.front().rule_index == to_int(SEED),
                     "Seed parse_tree should start with SEED node");
    const auto result = s_pt->visit_children(
        [&](const index_t rule_node_index, index_t) -> expected_t<bool> {
          SILVA_EXPECT_FMT(s_nodes[rule_node_index].rule_index == to_int(RULE), "");
          const small_vector_t<index_t, 3> children =
              SILVA_TRY(s_pt->get_children_up_to<3>(rule_node_index));
          SILVA_EXPECT_FMT(s_nodes[children[0]].rule_index == to_int(NONTERMINAL),
                           "First child of RULE must be RULE_NAME ");
          const string_view_t name =
              s_pt->tokenization->token_data(s_nodes[children[0]].token_index)->str;
          index_t rule_precedence = 0;
          if (children.size == 3) {
            SILVA_EXPECT_FMT(s_nodes[children[1]].rule_index == to_int(RULE_PRECEDENCE),
                             "Middle child of RULE must be RULE_PRECEDENCE");
            rule_precedence =
                s_pt->tokenization->token_data(s_nodes[children[1]].token_index)->as_double();
          }
          const index_t ri = s_nodes[children.back()].rule_index;
          SILVA_EXPECT_FMT(to_int(EXPR_0) <= ri && ri <= to_int(EXPR_1),
                           "Last child of RULE must be EXPR");
          index_t expr_node_index = children.back();
          SILVA_TRY(retval.add_rule(name, rule_precedence, expr_node_index));
          return true;
        },
        0);
    SILVA_EXPECT(result);
    for (index_t node_index = 0; node_index < s_pt->nodes.size(); ++node_index) {
      const auto& node = s_pt->nodes[node_index];
      if (node.rule_index == to_int(REGEX)) {
        string_t regex_str         = s_pt->tokenization->token_data(node.token_index)->as_string();
        retval.regexes[node_index] = std::regex(std::move(regex_str));
      }
    }
    return retval;
  }

  expected_t<parse_root_t> parse_root_t::create(const_ptr_t<source_code_t> source_code)
  {
    auto tokenization = SILVA_TRY(tokenize(std::move(source_code)));
    auto fern_seed_pt = SILVA_TRY(seed_parse(to_unique_ptr(std::move(tokenization))));
    auto retval       = SILVA_TRY(create(to_unique_ptr(std::move(fern_seed_pt))));
    return retval;
  }

  optional_t<index_t> parse_root_t::workspace_t::per_seed_token_id_t::get_target_token_id(
      const tokenization_t::token_data_t* sp_token_data,
      const tokenization_t* target_tokenization)
  {
    SILVA_ASSERT(sp_token_data->category == STRING);
    if (std::holds_alternative<uncached_t>(target_token_id)) {
      const string_t seed_terminal_string = sp_token_data->as_string();
      const optional_t<index_t> result    = target_tokenization->lookup_token(seed_terminal_string);
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
        if (seed_node.rule_index == to_int(TERMINAL_0)) {
          SILVA_EXPECT(token_data()->category == IDENTIFIER);
          SILVA_EXPECT(seed_node.num_children == 1);
          const array_t<index_t, 1> seed_node_index_regex =
              SILVA_TRY(seed_pt->get_children<1>(seed_node_index));
          const auto it = retval.root->regexes.find(seed_node_index_regex[0]);
          SILVA_ASSERT(it != retval.root->regexes.end());
          const std::regex& re          = it->second;
          const string_view_t token_str = token_data()->str;
          const bool is_match           = std::regex_search(token_str.begin(), token_str.end(), re);
          SILVA_EXPECT(is_match);
        }
        else {
          SILVA_EXPECT(seed_node.rule_index == to_int(TERMINAL_1));
          const token_id_t seed_token_id = seed_pt->tokenization->tokens[seed_node.token_index];
          if (seed_token_id == seed_tt_id) {
            SILVA_EXPECT(token_data()->category == IDENTIFIER);
          }
          else if (seed_token_id == seed_tt_op) {
            SILVA_EXPECT(token_data()->category == OPERATOR);
          }
          else if (seed_token_id == seed_tt_str) {
            SILVA_EXPECT(token_data()->category == STRING);
          }
          else if (seed_token_id == seed_tt_num) {
            SILVA_EXPECT(token_data()->category == NUMBER);
          }
          else if (seed_token_id == seed_tt_any) {
            ;
          }
          else {
            const auto* sp_token_data = seed_pt->tokenization->token_data(seed_node.token_index);
            SILVA_EXPECT(sp_token_data->category == STRING);
            auto& seed_token_id_work = workspace->seed_token_id_data[seed_token_id];
            const auto expected_target_token_id =
                seed_token_id_work.get_target_token_id(sp_token_data, retval.tokenization.get());
            SILVA_EXPECT_FMT(token_id() == expected_target_token_id,
                             "Expected '{}'",
                             sp_token_data->str);
          }
        }
        token_index += 1;
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_primary(const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        SILVA_EXPECT(token_index < retval.tokenization->tokens.size());
        const auto* token_data = retval.tokenization->token_data(token_index);
        const auto& seed_node  = seed_pt->nodes[seed_node_index];
        if (seed_node.rule_index == to_int(PRIMARY_0)) {
          gg.sub += SILVA_TRY(apply_expr_1(seed_node_index));
        }
        else if (seed_node.rule_index == to_int(PRIMARY_1)) {
          const array_t<index_t, 1> terminal_child =
              SILVA_TRY(seed_pt->get_children<1>(seed_node_index));
          gg.sub += SILVA_TRY(apply_terminal(terminal_child[0]));
        }
        else if (seed_node.rule_index == to_int(PRIMARY_2)) {
          const array_t<index_t, 1> nonterminal_child =
              SILVA_TRY(seed_pt->get_children<1>(seed_node_index));
          const auto& nonterminal_node = seed_pt->nodes[nonterminal_child[0]];
          const auto* seed_token = seed_pt->tokenization->token_data(nonterminal_node.token_index);
          SILVA_EXPECT(!seed_token->str.empty() && std::isupper(seed_token->str.front()));
          gg.sub += SILVA_TRY(apply_rule(seed_token->str));
        }
        else {
          SILVA_UNEXPECTED();
        }
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_expr_0(const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        bool found_match  = false;
        const auto result = seed_pt->visit_children(
            [&](const index_t node_index, const index_t) -> expected_t<bool> {
              const auto& node = seed_pt->nodes[node_index];
              auto result      = apply_terminal(node_index);
              if (result) {
                found_match = true;
                gg.sub += result.value();
                return false;
              }
              return true;
            },
            seed_node_index);
        SILVA_EXPECT_FMT(found_match, "could not find match");
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

      expected_t<parse_tree_sub_t> apply_expr_1(const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        const auto result = seed_pt->visit_children(
            [&](const index_t seed_node_index_atom, const index_t) -> expected_t<bool> {
              const auto& seed_node_atom = seed_pt->nodes[seed_node_index_atom];
              SILVA_EXPECT_FMT(seed_node_atom.rule_index == to_int(ATOM),
                               "expected atom in seed parse-tree");
              optional_t<char> suffix_char;
              const small_vector_t<index_t, 2> children =
                  SILVA_TRY(seed_pt->get_children_up_to<2>(seed_node_index_atom));
              if (children.size == 2) {
                const auto& seed_node_suffix = seed_pt->nodes[children[1]];
                SILVA_EXPECT(seed_node_suffix.rule_index == to_int(SUFFIX));
                const string_view_t suffix_op =
                    seed_pt->tokenization->token_data(seed_node_suffix.token_index)->str;
                SILVA_EXPECT(suffix_op.size() == 1);
                suffix_char = suffix_op.front();
              }
              else {
                SILVA_EXPECT_FMT(children.size == 1, "Atom had unexpected number of children");
              }
              const index_t seed_node_index_primary = children[0];

              if (!suffix_char) {
                gg.sub += SILVA_TRY(apply_primary(seed_node_index_primary));
              }
              else if (suffix_char.value() == '?' || suffix_char.value() == '*' ||
                       suffix_char.value() == '+') {
                const auto [min_repeat, max_repeat] = get_min_max_repeat(suffix_char.value());
                parse_tree_sub_t sub_sub;
                index_t repeat_count = 0;
                while (repeat_count < max_repeat) {
                  auto result = apply_primary(seed_node_index_primary);
                  if (!result) {
                    break;
                  }
                  else {
                    sub_sub += result.value();
                    repeat_count += 1;
                  }
                }
                SILVA_EXPECT_FMT(min_repeat <= repeat_count,
                                 "min-repeat (={}) not reached, only found {}",
                                 min_repeat,
                                 repeat_count);
                gg.sub += sub_sub;
              }
              else if (suffix_char.value() == '!') {
                parse_tree_guard_t inner_ptg{&retval, &token_index};
                const auto result = apply_primary(seed_node_index_primary);
                SILVA_EXPECT_FMT(!result, "Managed to parse negative primary expression");
              }
              else if (suffix_char.value() == '&') {
                parse_tree_guard_t inner_ptg{&retval, &token_index};
                const auto result = apply_primary(seed_node_index_primary);
                SILVA_EXPECT_FMT(result, "Did not manage to parse positive primary expression");
              }
              else {
                SILVA_UNEXPECTED();
              }
              return true;
            },
            seed_node_index);
        SILVA_EXPECT(result);
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_expr(const index_t seed_node_index)
      {
        const parse_tree_t::node_t& seed_node_expr = seed_pt->nodes[seed_node_index];
        if (seed_node_expr.rule_index == to_int(EXPR_0)) {
          return apply_expr_0(seed_node_index);
        }
        else if (seed_node_expr.rule_index == to_int(EXPR_1)) {
          return apply_expr_1(seed_node_index);
        }
        else {
          SILVA_UNEXPECTED_FMT("Unable to apply expr");
        }
      }

      expected_t<parse_tree_sub_t> apply_rule(const string_view_t rule_name)
      {
        const index_t orig_token_index = token_index;
        const auto it{retval.root->rule_name_offsets.find(rule_name)};
        SILVA_EXPECT_FMT(it != retval.root->rule_name_offsets.end(),
                         "Unknown rule '{}'",
                         rule_name);
        const index_t base_rule_offset = it->second;
        index_t rule_offset            = it->second;
        while (rule_offset < retval.root->rules.size()) {
          const auto& rule = retval.root->rules[rule_offset];
          if (rule.precedence != rule_offset - base_rule_offset) {
            break;
          }
          parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
          gg_rule.set_rule_index(rule_offset);
          const auto result = apply_expr(rule.expr_node_index);
          if (result) {
            gg_rule.sub += result.value();
            return gg_rule.release();
          }
          rule_offset += 1;
        }
        SILVA_UNEXPECTED_FMT("unable to parse rule '{}' at token {}", rule_name, orig_token_index);
      }
    };
  }

  expected_t<parse_tree_t> parse_root_t::apply(const_ptr_t<tokenization_t> tokenization,
                                               parse_root_t::workspace_t* workspace) const
  {
    optional_t<parse_root_t::workspace_t> local_workspace;
    if (workspace == nullptr) {
      local_workspace.emplace();
      workspace = &(*local_workspace);
    }
    impl::parse_root_nursery_t parse_root_nursery(std::move(tokenization),
                                                  const_ptr_unowned(this),
                                                  workspace);
    const parse_tree_sub_t sub = SILVA_TRY(parse_root_nursery.apply_rule(goal_rule_name));
    SILVA_ASSERT(sub.num_children == 1);
    SILVA_ASSERT(sub.num_children_total == parse_root_nursery.retval.nodes.size());
    return {std::move(parse_root_nursery.retval)};
  }
}
