#include "parse_root.hpp"

#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

#include <utility>

namespace silva {
  using enum token_category_t;
  using enum seed_rule_t;

  expected_t<void> parse_root_t::add_rule(const std::string_view rule_name,
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
    SILVA_ASSERT(result);
    return retval;
  }

  namespace impl {
    struct parse_root_nursery_t : public parse_tree_nursery_t {
      parse_root_nursery_t(const_ptr_t<tokenization_t> tokenization,
                           const_ptr_t<parse_root_t> parse_root)
        : parse_tree_nursery_t(std::move(tokenization), std::move(parse_root))
      {
      }

      expected_t<parse_tree_sub_t> apply_terminal(const index_t terminal_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        const auto& terminal_node = retval.root->seed_parse_tree->nodes[terminal_node_index];
        const auto* sp_token_data =
            retval.root->seed_parse_tree->tokenization->token_data(terminal_node.token_index);
        if (sp_token_data->category == STRING) {
          SILVA_EXPECT_FMT(token_data()->str == sp_token_data->as_string(),
                           "Expected '{}'",
                           sp_token_data->str);
        }
        else if (sp_token_data->str == "identifier") {
          SILVA_EXPECT(token_data()->category == IDENTIFIER);
        }
        else if (sp_token_data->str == "operator") {
          SILVA_EXPECT(token_data()->category == OPERATOR);
        }
        else if (sp_token_data->str == "string") {
          SILVA_EXPECT(token_data()->category == STRING);
        }
        else if (sp_token_data->str == "number") {
          SILVA_EXPECT(token_data()->category == NUMBER);
        }
        else if (sp_token_data->str == "any") {
          ;
        }
        else {
          SILVA_ASSERT(false);
        }
        token_index += 1;
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_primary(const index_t primary_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};

        SILVA_EXPECT(token_index < retval.tokenization->tokens.size());
        const auto* token_data   = retval.tokenization->token_data(token_index);
        const auto& primary_node = retval.root->seed_parse_tree->nodes[primary_node_index];
        if (primary_node.rule_index == to_int(PRIMARY_0)) {
          gg.sub += SILVA_TRY(apply_expr_1(primary_node_index));
        }
        else if (primary_node.rule_index == to_int(PRIMARY_1)) {
          const array_t<index_t, 1> terminal_child =
              SILVA_TRY(retval.root->seed_parse_tree->get_children<1>(primary_node_index));
          gg.sub += SILVA_TRY(apply_terminal(terminal_child[0]));
        }
        else if (primary_node.rule_index == to_int(PRIMARY_2)) {
          const array_t<index_t, 1> nonterminal_child =
              SILVA_TRY(retval.root->seed_parse_tree->get_children<1>(primary_node_index));
          const parse_tree_t::node_t& nonterminal_node =
              retval.root->seed_parse_tree->nodes[nonterminal_child[0]];
          const auto* seed_token =
              retval.root->seed_parse_tree->tokenization->token_data(nonterminal_node.token_index);
          SILVA_ASSERT(!seed_token->str.empty() && std::isupper(seed_token->str.front()));
          gg.sub += SILVA_TRY(apply_rule(seed_token->str));
        }
        else {
          SILVA_UNEXPECTED();
        }
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_expr_0(const index_t expr_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        bool found_match  = false;
        const auto result = retval.root->seed_parse_tree->visit_children(
            [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
              const parse_tree_t::node_t& node = retval.root->seed_parse_tree->nodes[node_index];
              SILVA_ASSERT(node.rule_index == to_int(TERMINAL));
              auto result = apply_terminal(node_index);
              if (result) {
                found_match = true;
                return false;
              }
              return true;
            },
            expr_node_index);
        SILVA_EXPECT_FMT(found_match, "could not find match");
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_expr_1(const index_t expr_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        const auto result = retval.root->seed_parse_tree->visit_children(
            [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
              const auto& atom_node = retval.root->seed_parse_tree->nodes[node_index];
              SILVA_EXPECT_FMT(atom_node.rule_index == to_int(ATOM),
                               "expected atom in seed parse-tree");
              std::optional<char> suffix_char;
              index_t primary_node_index = -1;
              const small_vector_t<index_t, 2> children =
                  SILVA_TRY(retval.root->seed_parse_tree->get_children_up_to<2>(node_index));
              if (children.size == 2) {
                primary_node_index      = children[0];
                const auto& suffix_node = retval.root->seed_parse_tree->nodes[children[1]];
                SILVA_EXPECT(suffix_node.rule_index == to_int(SUFFIX));
                const std::string_view suffix_op =
                    retval.root->seed_parse_tree->tokenization->token_data(suffix_node.token_index)
                        ->str;
                SILVA_EXPECT(suffix_op.size() == 1);
                suffix_char = suffix_op.front();
              }
              else {
                SILVA_EXPECT_FMT(children.size == 1, "Atom had unexpected number of children");
                primary_node_index = children[0];
              }

              if (!suffix_char) {
                gg.sub += SILVA_TRY(apply_primary(primary_node_index));
              }
              else if (suffix_char.value() == '?' || suffix_char.value() == '*' ||
                       suffix_char.value() == '+') {
                index_t min_repeat = 0;
                index_t max_repeat = std::numeric_limits<index_t>::max();
                if (suffix_char.value() == '?') {
                  max_repeat = 1;
                }
                else if (suffix_char.value() == '*') {
                  ;
                }
                else if (suffix_char.value() == '+') {
                  min_repeat = 1;
                }

                parse_tree_sub_t sub_sub;

                index_t repeat_count = 0;
                while (repeat_count < max_repeat) {
                  auto result = apply_primary(primary_node_index);
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
                const auto result = apply_primary(primary_node_index);
                SILVA_EXPECT_FMT(!result, "Managed to parse negative primary expression");
              }
              else if (suffix_char.value() == '&') {
                parse_tree_guard_t inner_ptg{&retval, &token_index};
                const auto result = apply_primary(primary_node_index);
                SILVA_EXPECT_FMT(result, "Did not manage to parse positive primary expression");
              }
              else {
                SILVA_UNEXPECTED();
              }
              return true;
            },
            expr_node_index);
        SILVA_EXPECT(result);
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_expr(const index_t expr_node_index)
      {
        const parse_tree_t::node_t& expr_node =
            retval.root->seed_parse_tree->nodes[expr_node_index];
        if (expr_node.rule_index == to_int(EXPR_0)) {
          return apply_expr_0(expr_node_index);
        }
        else if (expr_node.rule_index == to_int(EXPR_1)) {
          return apply_expr_1(expr_node_index);
        }
        else {
          SILVA_UNEXPECTED_FMT("Unable to apply expr");
        }
      }

      expected_t<parse_tree_sub_t> apply_rule(const std::string_view rule_name)
      {
        const index_t orig_token_index = token_index;

        const auto it = retval.root->rule_name_offsets.find(rule_name);
        SILVA_EXPECT_FMT(it != retval.root->rule_name_offsets.end(),
                         "Unknown rule '{}'",
                         rule_name);
        index_t rule_offset = it->second;

        const parse_root_t::rule_t& base_rule = retval.root->rules[rule_offset];
        while (rule_offset < retval.root->rules.size()) {
          const parse_root_t::rule_t& rule = retval.root->rules[rule_offset];
          if (rule.name != base_rule.name) {
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

  expected_t<parse_tree_t> parse_root_t::apply(const_ptr_t<tokenization_t> tokenization) const
  {
    impl::parse_root_nursery_t parse_root_nursery(std::move(tokenization), const_ptr_unowned(this));
    const parse_tree_sub_t sub = SILVA_TRY(parse_root_nursery.apply_rule(goal_rule_name));
    SILVA_ASSERT(sub.num_children == 1);
    SILVA_ASSERT(sub.num_children_total == parse_root_nursery.retval.nodes.size());
    return {std::move(parse_root_nursery.retval)};
  }

  expected_t<parse_root_t> parse_root_t::create(const_ptr_t<source_code_t> source_code)
  {
    auto tokenization = SILVA_TRY(tokenize(std::move(source_code)));
    auto fern_seed_pt = SILVA_TRY(seed_parse(to_unique_ptr(std::move(tokenization))));
    auto retval       = SILVA_TRY(create(to_unique_ptr(std::move(fern_seed_pt))));
    return retval;
  }
}
