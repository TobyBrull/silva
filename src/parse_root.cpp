#include "parse_root.hpp"

#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

#include <utility>

namespace silva {

  expected_t<void> parse_root_t::add_rule(
      const std::string_view name, const index_t precedence, const index_t expr_node_index)
  {
    if (rules.empty()) {
      goal_rule_name = name;
    }
    if (precedence > 0) {
      SILVA_EXPECT_FMT(
          !rules.empty() && rules.back().name == name && rules.back().precedence + 1 == precedence,
          "Rule with positive precedence did not follow rule with the same name and preceeding "
          "precedence");
    }
    const index_t offset = rules.size();
    rules.push_back(
        rule_t{.name = name, .precedence = precedence, .expr_node_index = expr_node_index});
    if (precedence == 0) {
      const auto [it, inserted] = rule_name_offsets.emplace(name, offset);
      SILVA_EXPECT_FMT(
          inserted,
          "Repeated rule name that was not consecutive with linearly increasing precedence");
    }
    return {};
  }

  expected_t<parse_root_t> parse_root_t::create(parse_tree_t seed_parse_tree)
  {
    parse_root_t retval;
    retval.seed_parse_tree = std::move(seed_parse_tree);
    const auto& spt{retval.seed_parse_tree};
    const auto* spr{retval.seed_parse_tree.root};
    const auto& nodes{spt.nodes};

    SILVA_EXPECT_FMT(
        spr == seed_parse_root_primordial() || spr == seed_parse_root(), "Not a seed parse_tree");

    SILVA_EXPECT_FMT(!nodes.empty(), "Empty Seed parse_tree");
    SILVA_EXPECT_FMT(
        nodes.front().rule_index == std::to_underlying(seed_rule_t::SEED),
        "Seed parse_tree should start with SEED node");

    const auto result = spt.visit_children(
        [&](const index_t node_index_rule, const index_t child_index) -> expected_t<bool> {
          SILVA_EXPECT_FMT(
              nodes[node_index_rule].rule_index == std::to_underlying(seed_rule_t::RULE), "");
          std::string_view name;
          index_t precedence      = 0;
          index_t node_index_expr = 0;
          const auto inner_result = spt.visit_children(
              [&](const index_t node_index_rule_child, const index_t ii) -> expected_t<bool> {
                if (ii == 0) {
                  SILVA_EXPECT_FMT(
                      nodes[node_index_rule_child].rule_index ==
                          std::to_underlying(seed_rule_t::NONTERMINAL),
                      "First child of RULE must be RULE_NAME ");
                  name =
                      spt.tokenization->token_data(nodes[node_index_rule_child].token_index)->str;
                }
                else if (ii == 1 && nodes[node_index_rule].num_children == 3) {
                  SILVA_EXPECT_FMT(
                      nodes[node_index_rule_child].rule_index ==
                          std::to_underlying(seed_rule_t::RULE_PRECEDENCE),
                      "Middle child of RULE must be RULE_PRECEDENCE");
                  precedence =
                      spt.tokenization->token_data(nodes[node_index_rule_child].token_index)
                          ->as_double();
                }
                else {
                  const index_t ri = nodes[node_index_rule_child].rule_index;
                  SILVA_EXPECT_FMT(
                      std::to_underlying(seed_rule_t::EXPR_0) <= ri &&
                          ri <= std::to_underlying(seed_rule_t::EXPR_1),
                      "Last child of RULE must be EXPR");
                  node_index_expr = node_index_rule_child;
                }
                return true;
              },
              node_index_rule);
          SILVA_ASSERT(inner_result);
          SILVA_TRY(retval.add_rule(name, precedence, node_index_expr));
          return true;
        },
        0);
    SILVA_ASSERT(result);

    return retval;
  }

  namespace impl {
    struct parse_root_nursery_t : public parse_tree_nursery_t {
      const parse_root_t* root = nullptr;

      parse_root_nursery_t(const tokenization_t* tokenization_, const parse_root_t* parse_root)
        : parse_tree_nursery_t(tokenization_, parse_root), root(parse_root)
      {
      }

      expected_t<parse_tree_sub_t> apply_terminal(const index_t terminal_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        const auto& terminal_node = root->seed_parse_tree.nodes[terminal_node_index];
        const auto* sp_token_data =
            root->seed_parse_tree.tokenization->token_data(terminal_node.token_index);
        if (sp_token_data->category == token_category_t::STRING) {
          SILVA_EXPECT_FMT(
              token_data()->str == sp_token_data->as_string(), "Expected '{}'", sp_token_data->str);
        }
        else if (sp_token_data->str == "identifier") {
          SILVA_EXPECT(token_data()->category == token_category_t::IDENTIFIER);
        }
        else if (sp_token_data->str == "operator") {
          SILVA_EXPECT(token_data()->category == token_category_t::OPERATOR);
        }
        else if (sp_token_data->str == "string") {
          SILVA_EXPECT(token_data()->category == token_category_t::STRING);
        }
        else if (sp_token_data->str == "number") {
          SILVA_EXPECT(token_data()->category == token_category_t::NUMBER);
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
        const auto& primary_node = root->seed_parse_tree.nodes[primary_node_index];
        if (primary_node.rule_index == std::to_underlying(seed_rule_t::PRIMARY_0)) {
          gg.sub += SILVA_TRY(apply_expr_1(primary_node_index));
        }
        else if (primary_node.rule_index == std::to_underlying(seed_rule_t::PRIMARY_1)) {
          const std::array<index_t, 1> terminal_child =
              SILVA_TRY(root->seed_parse_tree.get_num_children<1>(primary_node_index));
          gg.sub += SILVA_TRY(apply_terminal(terminal_child[0]));
        }
        else if (primary_node.rule_index == std::to_underlying(seed_rule_t::PRIMARY_2)) {
          const std::array<index_t, 1> nonterminal_child =
              SILVA_TRY(root->seed_parse_tree.get_num_children<1>(primary_node_index));
          const parse_tree_t::node_t& nonterminal_node =
              root->seed_parse_tree.nodes[nonterminal_child[0]];
          const auto* seed_token =
              root->seed_parse_tree.tokenization->token_data(nonterminal_node.token_index);
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
        const auto result = root->seed_parse_tree.visit_children(
            [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
              const parse_tree_t::node_t& node = root->seed_parse_tree.nodes[node_index];
              SILVA_ASSERT(node.rule_index == std::to_underlying(seed_rule_t::TERMINAL));
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
        const auto result = root->seed_parse_tree.visit_children(
            [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
              const auto& atom_node = root->seed_parse_tree.nodes[node_index];
              SILVA_EXPECT_FMT(
                  atom_node.rule_index == std::to_underlying(seed_rule_t::ATOM),
                  "expected atom in seed parse-tree");
              std::optional<char> suffix_char;
              index_t primary_node_index = -1;
              if (atom_node.num_children == 2) {
                std::array<index_t, 2> children =
                    SILVA_TRY(root->seed_parse_tree.get_num_children<2>(node_index));
                primary_node_index      = children[0];
                const auto& suffix_node = root->seed_parse_tree.nodes[children[1]];
                SILVA_EXPECT(suffix_node.rule_index == std::to_underlying(seed_rule_t::SUFFIX));
                const std::string_view suffix_op =
                    root->seed_parse_tree.tokenization->token_data(suffix_node.token_index)->str;
                SILVA_EXPECT(suffix_op.size() == 1);
                suffix_char = suffix_op.front();
              }
              else {
                SILVA_EXPECT_FMT(
                    atom_node.num_children == 1, "Atom had unexpected number of children");
                std::array<index_t, 1> child =
                    SILVA_TRY(root->seed_parse_tree.get_num_children<1>(node_index));
                primary_node_index = child[0];
              }

              if (!suffix_char) {
                gg.sub += SILVA_TRY(apply_primary(primary_node_index));
              }
              else if (
                  suffix_char.value() == '?' || suffix_char.value() == '*' ||
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
                SILVA_EXPECT_FMT(
                    min_repeat <= repeat_count,
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
        const parse_tree_t::node_t& expr_node = root->seed_parse_tree.nodes[expr_node_index];
        if (expr_node.rule_index == std::to_underlying(seed_rule_t::EXPR_0)) {
          return apply_expr_0(expr_node_index);
        }
        else if (expr_node.rule_index == std::to_underlying(seed_rule_t::EXPR_1)) {
          return apply_expr_1(expr_node_index);
        }
        else {
          SILVA_UNEXPECTED_FMT("Unable to apply expr");
        }
      }

      expected_t<parse_tree_sub_t> apply_rule(const std::string_view rule_name)
      {
        const index_t orig_token_index = token_index;

        const auto it = root->rule_name_offsets.find(rule_name);
        SILVA_EXPECT_FMT(it != root->rule_name_offsets.end(), "Unknown rule '{}'", rule_name);
        index_t rule_offset = it->second;

        const parse_root_t::rule_t& base_rule = root->rules[rule_offset];
        while (rule_offset < root->rules.size()) {
          const parse_root_t::rule_t& rule = root->rules[rule_offset];
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

  expected_t<parse_tree_t> parse_root_t::apply(const tokenization_t* tokenization) const
  {
    impl::parse_root_nursery_t parse_root_nursery(tokenization, this);
    const parse_tree_sub_t sub = SILVA_TRY(parse_root_nursery.apply_rule(goal_rule_name));
    SILVA_ASSERT(sub.num_children == 1);
    SILVA_ASSERT(sub.num_children_total == parse_root_nursery.retval.nodes.size());
    return {std::move(parse_root_nursery.retval)};
  }

  expected_t<parse_root_t> parse_root_t::create(const tokenization_t* tokenization)
  {
    auto pt = SILVA_TRY(seed_parse_root()->apply(tokenization));
    return parse_root_t::create(std::move(pt));
  }
}
