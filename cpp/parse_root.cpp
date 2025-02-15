#include "parse_root.hpp"

#include "canopy/scope_exit.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

#include <utility>

namespace silva {
  using enum token_category_t;
  using enum error_level_t;

  namespace impl {
    expected_t<void> create_add_rule(parse_root_t& retval,
                                     token_context_t& tc,
                                     const token_id_t rule_token_id,
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
      const string_t prec_str        = std::to_string(precedence);
      const token_id_t prec_token_id = tc.token_id(prec_str);
      const full_name_id_t rule_name_id =
          tc.full_name_id(std::array<token_id_t, 2>{rule_token_id, prec_token_id});
      const index_t offset = retval.rules.size();
      retval.rules.push_back(parse_root_t::rule_t{
          .token_id        = rule_token_id,
          .precedence      = precedence,
          .rule_name       = rule_name_id,
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

  expected_t<unique_ptr_t<parse_root_t>>
  parse_root_t::create(shared_ptr_t<const parse_tree_t> seed_parse_tree)
  {
    token_context_ptr_t tcp      = seed_parse_tree->tokenization->context;
    full_name_id_t fni_seed      = tcp->full_name_id_of("Seed", "0");
    full_name_id_t fni_rule      = tcp->full_name_id_of("Rule", "0");
    full_name_id_t fni_rule_prec = tcp->full_name_id_of("RulePrecedence", "0");
    full_name_id_t fni_deriv_0   = tcp->full_name_id_of("Derivation", "0");
    full_name_id_t fni_deriv_1   = tcp->full_name_id_of("Derivation", "1");
    full_name_id_t fni_deriv_2   = tcp->full_name_id_of("Derivation", "2");
    full_name_id_t fni_deriv_3   = tcp->full_name_id_of("Derivation", "3");
    full_name_id_t fni_regex     = tcp->full_name_id_of("Regex", "0");
    full_name_id_t fni_nonterm   = tcp->full_name_id_of("Nonterminal", "0");

    auto retval              = std::make_unique<parse_root_t>();
    retval->seed_parse_tree  = std::move(seed_parse_tree);
    const parse_tree_t* s_pt = retval->seed_parse_tree.get();
    const auto& s_nodes      = s_pt->nodes;
    SILVA_EXPECT(!s_nodes.empty() && s_nodes.front().rule_name == fni_seed,
                 MINOR,
                 "Seed parse_tree should start with SEED node");

    auto result = s_pt->visit_children(
        [&](const index_t rule_node_index, index_t) -> expected_t<bool> {
          SILVA_EXPECT(s_nodes[rule_node_index].rule_name == fni_rule, MINOR, "");
          const small_vector_t<index_t, 3> children =
              SILVA_EXPECT_FWD(s_pt->get_children_up_to<3>(rule_node_index));
          SILVA_EXPECT(s_nodes[children[0]].rule_name == fni_nonterm,
                       MINOR,
                       "First child of RULE must be NONTERMINAL ");
          const token_id_t rule_token_id =
              s_pt->tokenization->tokens[s_nodes[children[0]].token_index];
          index_t rule_precedence = 0;
          if (children.size == 3) {
            SILVA_EXPECT(s_nodes[children[1]].rule_name == fni_rule_prec,
                         MINOR,
                         "Middle child of RULE must be RULE_PRECEDENCE");
            const auto* token_data =
                s_pt->tokenization->token_info_get(s_nodes[children[1]].token_index);
            rule_precedence = SILVA_EXPECT_FWD(token_data->number_as_double(), MAJOR);
          }
          const index_t ri = s_nodes[children.back()].rule_name;
          SILVA_EXPECT(ri == fni_deriv_0 || ri == fni_deriv_1 || ri == fni_deriv_2 ||
                           ri == fni_deriv_3,
                       MINOR,
                       "Last child of RULE must be DERIVATION_{0,1,2,3}");
          index_t expr_node_index = children.back();
          SILVA_EXPECT_FWD(impl::create_add_rule(*retval,
                                                 *tcp,
                                                 rule_token_id,
                                                 rule_precedence,
                                                 expr_node_index));
          return true;
        },
        0);
    SILVA_EXPECT_FWD(std::move(result));

    auto result_2 = s_pt->visit_children(
        [&](const index_t rule_node_index, index_t) -> expected_t<bool> {
          SILVA_EXPECT(s_nodes[rule_node_index].rule_name == fni_rule, MINOR, "");
          const small_vector_t<index_t, 3> rule_children =
              SILVA_EXPECT_FWD(s_pt->get_children_up_to<3>(rule_node_index));
          if (s_nodes[rule_children.back()].rule_name == fni_deriv_2) {
            const array_t<index_t, 2> alias_children =
                SILVA_EXPECT_FWD(s_pt->get_children<2>(rule_children.back()));
            SILVA_EXPECT(s_nodes[alias_children[0]].rule_name == fni_nonterm,
                         MINOR,
                         "First child of DERIVATION_2 must be NONTERMINAL");
            SILVA_EXPECT(s_nodes[alias_children[1]].rule_name == fni_rule_prec,
                         MINOR,
                         "Second child of DERIVATION_2 must be RULE_PRECEDENCE");
            const token_id_t tgt_rule_token_id =
                s_pt->tokenization->tokens[s_nodes[alias_children[0]].token_index];
            const auto* tgt_rule_precedence_token_data =
                s_pt->tokenization->token_info_get(s_nodes[alias_children[1]].token_index);
            const index_t tgt_rule_precedence =
                SILVA_EXPECT_FWD(tgt_rule_precedence_token_data->number_as_double(), MAJOR);

            const token_id_t base_rule_token_id =
                s_pt->tokenization->tokens[s_nodes[rule_children[0]].token_index];
            index_t base_rule_precedence = 0;
            if (rule_children.size == 3) {
              SILVA_EXPECT(s_nodes[rule_children[1]].rule_name == fni_rule_prec,
                           MINOR,
                           "Middle child of RULE must be RULE_PRECEDENCE");
              const auto* token_data =
                  s_pt->tokenization->token_info_get(s_nodes[rule_children[1]].token_index);
              base_rule_precedence = SILVA_EXPECT_FWD(token_data->number_as_double(), MAJOR);
            }

            const index_t base_offset =
                retval->rule_indexes.at(base_rule_token_id) + base_rule_precedence;
            const index_t alias_offset =
                retval->rule_indexes.at(tgt_rule_token_id) + tgt_rule_precedence;
            retval->rules[base_offset].aliased_rule_offset = alias_offset;
          }
          return true;
        },
        0);
    SILVA_EXPECT_FWD(std::move(result_2));

    for (const auto& rule: retval->rules) {
      if (rule.aliased_rule_offset.has_value()) {
        const index_t aro = rule.aliased_rule_offset.value();
        SILVA_EXPECT(aro < retval->rules.size(), MINOR, "Invalid rule-offset {}", aro);
        const auto& alias_rule = retval->rules[aro];
        SILVA_EXPECT(!alias_rule.aliased_rule_offset.has_value(),
                     MINOR,
                     "Rule (={},{}) cannot be alias to another alias-rule (={},{})",
                     tcp->token_infos[rule.token_id].str,
                     rule.precedence,
                     tcp->token_infos[alias_rule.token_id].str,
                     alias_rule.precedence);
      }
    }

    for (index_t node_index = 0; node_index < s_pt->nodes.size(); ++node_index) {
      const auto& node = s_pt->nodes[node_index];
      if (node.rule_name == fni_regex) {
        const token_id_t regex_token_id = s_pt->tokenization->tokens[node.token_index];
        if (auto& regex = retval->regexes[regex_token_id]; !regex.has_value()) {
          const auto& regex_td = s_pt->tokenization->token_info_get(node.token_index);
          const string_t regex_str{SILVA_EXPECT_FWD(regex_td->string_as_plain_contained(), MAJOR)};
          regex = std::regex(regex_str);
        }
      }
    }
    return retval;
  }

  namespace impl {
    struct parse_root_nursery_t : public parse_tree_nursery_t {
      const parse_root_t* root    = nullptr;
      const parse_tree_t* seed_pt = nullptr;

      int rule_depth = 0;

      token_id_t seed_tt_id  = tcp->token_id("identifier");
      token_id_t seed_tt_op  = tcp->token_id("operator");
      token_id_t seed_tt_str = tcp->token_id("string");
      token_id_t seed_tt_num = tcp->token_id("number");
      token_id_t seed_tt_any = tcp->token_id("any");

      full_name_id_t fni_seed      = tcp->full_name_id_of("Seed", "0");
      full_name_id_t fni_rule      = tcp->full_name_id_of("Rule", "0");
      full_name_id_t fni_rule_prec = tcp->full_name_id_of("RulePrecedence", "0");
      full_name_id_t fni_deriv_0   = tcp->full_name_id_of("Derivation", "0");
      full_name_id_t fni_deriv_1   = tcp->full_name_id_of("Derivation", "1");
      full_name_id_t fni_deriv_2   = tcp->full_name_id_of("Derivation", "2");
      full_name_id_t fni_deriv_3   = tcp->full_name_id_of("Derivation", "3");
      full_name_id_t fni_regex     = tcp->full_name_id_of("Regex", "0");
      full_name_id_t fni_nonterm   = tcp->full_name_id_of("Nonterminal", "0");
      full_name_id_t fni_term_0    = tcp->full_name_id_of("Terminal", "0");
      full_name_id_t fni_term_1    = tcp->full_name_id_of("Terminal", "1");
      full_name_id_t fni_prim_0    = tcp->full_name_id_of("Primary", "0");
      full_name_id_t fni_prim_1    = tcp->full_name_id_of("Primary", "1");
      full_name_id_t fni_prim_2    = tcp->full_name_id_of("Primary", "2");
      full_name_id_t fni_atom_0    = tcp->full_name_id_of("Atom", "0");
      full_name_id_t fni_atom_1    = tcp->full_name_id_of("Atom", "1");
      full_name_id_t fni_suffix    = tcp->full_name_id_of("Suffix", "0");

      parse_root_nursery_t(shared_ptr_t<const tokenization_t> tokenization,
                           const parse_root_t* root)
        : parse_tree_nursery_t(std::move(tokenization))
        , root(root)
        , seed_pt(root->seed_parse_tree.get())
      {
      }

      expected_t<parse_tree_sub_t> apply_terminal(const index_t seed_node_index)
      {
        parse_tree_guard_t gg{&retval, &token_index};
        const auto& seed_node = seed_pt->nodes[seed_node_index];
        SILVA_EXPECT_PARSE(token_index < retval.tokenization->tokens.size(),
                           "Reached end of token-stream when looking for {}",
                           seed_pt->tokenization->token_info_get(seed_node.token_index)->str);
        if (seed_node.rule_name == fni_term_0) {
          SILVA_EXPECT(seed_node.num_children == 1,
                       MAJOR,
                       "Expected Seed node TERIMNAL_0 to have exactly one child");
          const array_t<index_t, 1> seed_node_index_regex =
              SILVA_EXPECT_FWD(seed_pt->get_children<1>(seed_node_index));
          const auto& seed_regex_node = seed_pt->nodes[seed_node_index_regex[0]];
          const token_id_t regex_token_id =
              seed_pt->tokenization->tokens[seed_regex_node.token_index];
          const auto it = root->regexes.find(regex_token_id);
          SILVA_EXPECT(it != root->regexes.end() || !it->second.has_value(), FATAL);
          SILVA_EXPECT_PARSE(token_data_by()->category == IDENTIFIER, "Expected identifier");
          const std::regex& re          = it->second.value();
          const string_view_t token_str = token_data_by()->str;
          const bool is_match           = std::regex_search(token_str.begin(), token_str.end(), re);
          SILVA_EXPECT_PARSE(is_match,
                             "Token \"{}\" does not match regex {}",
                             token_str,
                             tcp->token_infos[regex_token_id].str);
        }
        else {
          SILVA_EXPECT(seed_node.rule_name == fni_term_1, MAJOR, "Expected Seed node TERMINAL_1");
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
            const auto* sp_token_data =
                seed_pt->tokenization->token_info_get(seed_node.token_index);
            SILVA_EXPECT(sp_token_data->category == STRING, MAJOR);
            const token_id_t expected_target_token_id =
                tcp->token_id(SILVA_EXPECT_FWD(sp_token_data->string_as_plain_contained(), MAJOR));
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
        if (seed_node.rule_name == fni_prim_0) {
          gg.sub += SILVA_EXPECT_FWD(apply_derivation_0("subexpression", seed_node_index));
        }
        else if (seed_node.rule_name == fni_prim_1) {
          const array_t<index_t, 1> terminal_child =
              SILVA_EXPECT_FWD(seed_pt->get_children<1>(seed_node_index));
          gg.sub += SILVA_EXPECT_FWD(apply_terminal(terminal_child[0]));
        }
        else if (seed_node.rule_name == fni_prim_2) {
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
        error_level_t min_error_level = NO_ERROR;
        auto result                   = seed_pt->visit_children(
            [&](const index_t seed_node_index_atom, const index_t) -> expected_t<bool> {
              const auto& seed_node_atom = seed_pt->nodes[seed_node_index_atom];
              if (seed_node_atom.rule_name == fni_atom_0) {
                min_error_level = MAJOR;
                return true;
              }
              SILVA_EXPECT(seed_node_atom.rule_name == fni_atom_1,
                           MAJOR,
                           "Expected Atom in Seed parse-tree");
              optional_t<char> suffix_char;
              const small_vector_t<index_t, 2> children =
                  SILVA_EXPECT_FWD(seed_pt->get_children_up_to<2>(seed_node_index_atom));
              if (children.size == 2) {
                const auto& seed_node_suffix = seed_pt->nodes[children[1]];
                SILVA_EXPECT(seed_node_suffix.rule_name == fni_suffix, MAJOR);
                const string_view_t suffix_op =
                    seed_pt->tokenization->token_info_get(seed_node_suffix.token_index)->str;
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
                                             string_t{expr_name}));
        }
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_derivation_3(const index_t seed_node_index)
      {
        SILVA_EXPECT(false, FATAL, "Operator '=%' not yet implemented");
      }

      expected_t<parse_tree_sub_t> apply_expr(const parse_root_t::rule_t& rule)
      {
        const parse_tree_t::node_t& seed_node_expr = seed_pt->nodes[rule.expr_node_index];
        if (seed_node_expr.rule_name == fni_deriv_0) {
          return apply_derivation_0(tcp->full_name_to_string(rule.rule_name, "."),
                                    rule.expr_node_index);
        }
        else if (seed_node_expr.rule_name == fni_deriv_1) {
          return apply_derivation_1(rule.expr_node_index);
        }
        else if (seed_node_expr.rule_name == fni_deriv_3) {
          return apply_derivation_3(rule.expr_node_index);
        }
        else {
          SILVA_EXPECT(false, MAJOR, "Expected Seed node with DERIVATION_{0,1,3}");
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
        const auto it{root->rule_indexes.find(rule_token_id)};
        SILVA_EXPECT(it != root->rule_indexes.end(),
                     MAJOR,
                     "Unknown rule-token-id '{}'",
                     rule_token_id);
        const index_t base_rule_offset = it->second;
        index_t rule_offset            = it->second;
        error_nursery_t error_nursery;
        error_level_t retval_error_level = MINOR;
        while (rule_offset < root->rules.size()) {
          const parse_root_t::rule_t& rule = root->rules[rule_offset];
          if (rule.precedence != rule_offset - base_rule_offset) {
            break;
          }
          parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
          const index_t aro = root->rules[rule_offset].aliased_rule_offset.value_or(rule_offset);
          const auto& aliased_rule = root->rules[aro];
          gg_rule.set_rule_name(aliased_rule.rule_name);
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
        return std::unexpected(
            std::move(error_nursery)
                .finish_short(
                    retval_error_level,
                    "{} Expected {} rule",
                    token_position_at(orig_token_index),
                    tcp->full_name_to_string(root->rules[base_rule_offset].rule_name, ".")));
      }
    };
  }

  expected_t<unique_ptr_t<parse_root_t>>
  parse_root_t::create(token_context_ptr_t tcp, filesystem_path_t filepath, string_t text)
  {
    auto tt     = SILVA_EXPECT_FWD(tokenize(tcp, std::move(filepath), std::move(text)));
    auto pt     = SILVA_EXPECT_FWD(seed_parse(std::move(tt)));
    auto retval = SILVA_EXPECT_FWD(parse_root_t::create(std::move(pt)));
    return retval;
  }

  expected_t<unique_ptr_t<parse_tree_t>>
  parse_root_t::apply(shared_ptr_t<const tokenization_t> tokenization) const
  {
    impl::parse_root_nursery_t parse_root_nursery(std::move(tokenization), this);
    expected_traits_t expected_traits{.materialize_fwd = true};
    const parse_tree_sub_t sub =
        SILVA_EXPECT_FWD(parse_root_nursery.apply_rule(goal_rule_token_id));
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.num_children_total == parse_root_nursery.retval.nodes.size(), ASSERT);
    return {std::make_unique<parse_tree_t>(std::move(parse_root_nursery.retval))};
  }
}
