#include "parse_root.hpp"

#include "canopy/expected.hpp"
#include "canopy/scope_exit.hpp"
#include "parse_axe.hpp"
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

    expected_t<void> create_parse_axe__axe_ops(token_context_ptr_t tcp,
                                               parse_axe::parse_axe_level_desc_t& level,
                                               const parse_tree_t* seed_pt,
                                               const index_t axe_ops_node_index)
    {
      const token_id_t ti_atom_nest    = tcp->token_id("atom_nest");
      const token_id_t ti_prefix       = tcp->token_id("prefix");
      const token_id_t ti_prefix_nest  = tcp->token_id("prefix_nest");
      const token_id_t ti_infix        = tcp->token_id("infix");
      const token_id_t ti_ternary      = tcp->token_id("ternary");
      const token_id_t ti_postfix      = tcp->token_id("postfix");
      const token_id_t ti_postfix_nest = tcp->token_id("postfix_nest");
      const token_id_t ti_none         = tcp->token_id("none");

      const full_name_id_t fni_axe_op_type = tcp->full_name_id_of("AxeOpType", "0");
      const full_name_id_t fni_axe_op      = tcp->full_name_id_of("AxeOp", "0");
      token_id_t axe_op_type               = token_id_none;
      vector_t<token_id_t> axe_op_vec;
      auto result = seed_pt->visit_children(
          [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
            if (child_index == 0) {
              SILVA_EXPECT(seed_pt->nodes[child_node_index].rule_name == fni_axe_op_type, MAJOR);
              const index_t axe_op_type_index = seed_pt->nodes[child_node_index].token_begin;
              axe_op_type                     = seed_pt->tokenization->tokens[axe_op_type_index];
              SILVA_EXPECT(axe_op_type == ti_atom_nest || axe_op_type == ti_prefix ||
                               axe_op_type == ti_prefix_nest || axe_op_type == ti_infix ||
                               axe_op_type == ti_ternary || axe_op_type == ti_postfix ||
                               axe_op_type == ti_postfix_nest,
                           MAJOR);
            }
            else {
              SILVA_EXPECT(seed_pt->nodes[child_node_index].rule_name == fni_axe_op, MAJOR);
              const index_t axe_op_index = seed_pt->nodes[child_node_index].token_begin;
              const token_id_t axe_op    = seed_pt->tokenization->tokens[axe_op_index];
              const token_info_t* info   = &tcp->token_infos[axe_op];
              SILVA_EXPECT(info->category == token_category_t::STRING || axe_op == ti_none, MAJOR);
              if (axe_op_type != ti_infix) {
                SILVA_EXPECT(axe_op != ti_none,
                             MINOR,
                             "The \"none\" token may only be used with \"infix\" operations.");
              }
              axe_op_vec.push_back(axe_op);
            }
            return true;
          },
          axe_ops_node_index);
      SILVA_EXPECT_FWD(std::move(result));

      if (axe_op_type == ti_atom_nest || axe_op_type == ti_prefix_nest ||
          axe_op_type == ti_ternary || axe_op_type == ti_postfix_nest) {
        SILVA_EXPECT(axe_op_vec.size() % 2 == 0,
                     MINOR,
                     "For [ \"atom_nest\" \"prefix_nest\" \"ternary\" \"postfix_nest\" ] "
                     "operations an even number of operators is expected");
      }

      index_t i = 0;
      while (i < axe_op_vec.size()) {
        if (axe_op_type == ti_atom_nest) {
          SILVA_EXPECT(i + 1 < axe_op_vec.size(), ASSERT);
          level.opers.push_back(parse_axe::atom_nest_t{
              .left_bracket  = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
              .right_bracket = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i + 1])),
          });
          i += 2;
        }
        else if (axe_op_type == ti_prefix) {
          level.opers.push_back(parse_axe::prefix_t{
              .token_id = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
          });
          i += 1;
        }
        else if (axe_op_type == ti_prefix_nest) {
          SILVA_EXPECT(i + 1 < axe_op_vec.size(), ASSERT);
          level.opers.push_back(parse_axe::prefix_nest_t{
              .left_bracket  = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
              .right_bracket = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i + 1])),
          });
          i += 2;
        }
        else if (axe_op_type == ti_infix) {
          if (axe_op_vec[i] == ti_none) {
            level.opers.push_back(parse_axe::infix_t{
                .token_id = token_id_none,
            });
          }
          else {
            level.opers.push_back(parse_axe::infix_t{
                .token_id = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
            });
          }
          i += 1;
        }
        else if (axe_op_type == ti_ternary) {
          SILVA_EXPECT(i + 1 < axe_op_vec.size(), ASSERT);
          level.opers.push_back(parse_axe::ternary_t{
              .first  = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
              .second = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i + 1])),
          });
          i += 2;
        }
        else if (axe_op_type == ti_postfix) {
          level.opers.push_back(parse_axe::postfix_t{
              .token_id = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
          });
          i += 1;
        }
        else if (axe_op_type == ti_postfix_nest) {
          SILVA_EXPECT(i + 1 < axe_op_vec.size(), ASSERT);
          level.opers.push_back(parse_axe::postfix_nest_t{
              .left_bracket  = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
              .right_bracket = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i + 1])),
          });
          i += 2;
        }
        else {
          SILVA_EXPECT(false, ASSERT);
        }
      }
      return {};
    }

    expected_t<void> create_parse_axe__axe_level(token_context_ptr_t tcp,
                                                 parse_axe::parse_axe_level_desc_t& level,
                                                 const full_name_id_t base_name,
                                                 const parse_tree_t* seed_pt,
                                                 const index_t axe_level_node_index)
    {
      const token_id_t ti_nest = tcp->token_id("nest");
      const token_id_t ti_ltr  = tcp->token_id("ltr");
      const token_id_t ti_rtl  = tcp->token_id("rtl");

      const full_name_id_t fni_nonterm   = tcp->full_name_id_of("Nonterminal", "0");
      const full_name_id_t fni_axe_assoc = tcp->full_name_id_of("AxeAssoc", "0");
      const full_name_id_t fni_axe_ops   = tcp->full_name_id_of("AxeOps", "0");
      auto result                        = seed_pt->visit_children(
          [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
            if (child_index == 0) {
              SILVA_EXPECT(seed_pt->nodes[child_node_index].rule_name == fni_nonterm, MAJOR);
              const index_t nonterminal_index = seed_pt->nodes[child_node_index].token_begin;
              const token_id_t nonterminal    = seed_pt->tokenization->tokens[nonterminal_index];
              level.name                      = tcp->full_name_id(base_name, nonterminal);
            }
            else if (child_index == 1) {
              SILVA_EXPECT(seed_pt->nodes[child_node_index].rule_name == fni_axe_assoc, MAJOR);
              const index_t assoc_index = seed_pt->nodes[child_node_index].token_begin;
              const token_id_t assoc    = seed_pt->tokenization->tokens[assoc_index];
              using enum parse_axe::assoc_t;
              if (assoc == ti_nest) {
                level.assoc = NEST;
              }
              else if (assoc == ti_ltr) {
                level.assoc = LEFT_TO_RIGHT;
              }
              else if (assoc == ti_rtl) {
                level.assoc = RIGHT_TO_LEFT;
              }
              else {
                SILVA_EXPECT(false, MAJOR);
              }
            }
            else {
              SILVA_EXPECT(seed_pt->nodes[child_node_index].rule_name == fni_axe_ops, MAJOR);
              SILVA_EXPECT_FWD(create_parse_axe__axe_ops(tcp, level, seed_pt, child_node_index));
            }
            return true;
          },
          axe_level_node_index);
      SILVA_EXPECT_FWD(std::move(result));
      return {};
    }

    expected_t<parse_root_t::parse_axe_data_t> create_parse_axe(token_context_ptr_t tcp,
                                                                const full_name_id_t base_name,
                                                                const parse_tree_t* seed_pt,
                                                                const index_t axe_spec_node_index)
    {
      full_name_id_t fni_axe_spec  = tcp->full_name_id_of("AxeSpec", "0");
      full_name_id_t fni_nonterm   = tcp->full_name_id_of("Nonterminal", "0");
      full_name_id_t fni_axe_level = tcp->full_name_id_of("AxeLevel", "0");
      SILVA_EXPECT(seed_pt->nodes[axe_spec_node_index].rule_name == fni_axe_spec, MAJOR);
      vector_t<parse_axe::parse_axe_level_desc_t> level_descs;
      SILVA_EXPECT(seed_pt->nodes[axe_spec_node_index].num_children >= 1, MAJOR);
      level_descs.reserve(seed_pt->nodes[axe_spec_node_index].num_children - 1);
      token_id_t atom_rule = token_id_none;
      auto result          = seed_pt->visit_children(
          [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
            if (child_index == 0) {
              SILVA_EXPECT(seed_pt->nodes[child_node_index].rule_name == fni_nonterm, MAJOR);
              const index_t nt_token_idx = seed_pt->nodes[child_node_index].token_begin;
              atom_rule                  = seed_pt->tokenization->tokens[nt_token_idx];
            }
            else {
              SILVA_EXPECT(seed_pt->nodes[child_node_index].rule_name == fni_axe_level, MAJOR);
              auto& curr_level = level_descs.emplace_back();
              SILVA_EXPECT_FWD(create_parse_axe__axe_level(tcp,
                                                           curr_level,
                                                           base_name,
                                                           seed_pt,
                                                           child_node_index));
            }
            return true;
          },
          axe_spec_node_index);
      SILVA_EXPECT_FWD(std::move(result));
      auto pa = SILVA_EXPECT_FWD(parse_axe::parse_axe_create(tcp, std::move(level_descs)));
      return {{.atom_rule = atom_rule, .parse_axe = std::move(pa)}};
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
    full_name_id_t fni_axe_spec  = tcp->full_name_id_of("AxeSpec", "0");

    auto retval              = std::make_unique<parse_root_t>();
    retval->seed_parse_tree  = std::move(seed_parse_tree);
    const parse_tree_t* s_pt = retval->seed_parse_tree.get();
    const auto& s_nodes      = s_pt->nodes;
    SILVA_EXPECT(!s_nodes.empty() && s_nodes.front().rule_name == fni_seed,
                 MINOR,
                 "Seed parse_tree should start with SEED node");

    // Create entry in "rules" vector_t (and "rule_indexes" hashmap_t) for each rule.
    auto result = s_pt->visit_children(
        [&](const index_t rule_node_index, index_t) -> expected_t<bool> {
          SILVA_EXPECT(s_nodes[rule_node_index].rule_name == fni_rule, MINOR, "");
          const small_vector_t<index_t, 3> children =
              SILVA_EXPECT_FWD(s_pt->get_children_up_to<3>(rule_node_index));
          SILVA_EXPECT(s_nodes[children[0]].rule_name == fni_nonterm,
                       MINOR,
                       "First child of RULE must be NONTERMINAL ");
          const token_id_t rule_token_id =
              s_pt->tokenization->tokens[s_nodes[children[0]].token_begin];
          index_t rule_precedence = 0;
          if (children.size == 3) {
            SILVA_EXPECT(s_nodes[children[1]].rule_name == fni_rule_prec,
                         MINOR,
                         "Middle child of RULE must be RULE_PRECEDENCE");
            const auto* token_data =
                s_pt->tokenization->token_info_get(s_nodes[children[1]].token_begin);
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

    // Handle alias rules.
    for (const auto& rule: retval->rules) {
      if (s_nodes[rule.expr_node_index].rule_name == fni_deriv_2) {
        const array_t<index_t, 2> alias_children =
            SILVA_EXPECT_FWD(s_pt->get_children<2>(rule.expr_node_index));
        SILVA_EXPECT(s_nodes[alias_children[0]].rule_name == fni_nonterm,
                     MINOR,
                     "First child of Derivation,2 must be Nonterminal");
        SILVA_EXPECT(s_nodes[alias_children[1]].rule_name == fni_rule_prec,
                     MINOR,
                     "Second child of Derivation,2 must be RulePrecedence");
        const token_id_t tgt_rule_token_id =
            s_pt->tokenization->tokens[s_nodes[alias_children[0]].token_begin];
        const auto* tgt_rule_precedence_token_data =
            s_pt->tokenization->token_info_get(s_nodes[alias_children[1]].token_begin);
        const index_t tgt_rule_precedence =
            SILVA_EXPECT_FWD(tgt_rule_precedence_token_data->number_as_double(), MAJOR);
        const index_t base_offset = retval->rule_indexes.at(rule.token_id) + rule.precedence;
        const index_t alias_offset =
            retval->rule_indexes.at(tgt_rule_token_id) + tgt_rule_precedence;
        retval->rules[base_offset].aliased_rule_offset = alias_offset;
      }
    }

    // Pre-compile hashmap_t of "parse_axes".
    for (const auto& rule: retval->rules) {
      if (s_nodes[rule.expr_node_index].rule_name == fni_deriv_3) {
        const array_t<index_t, 1> deriv_3_child =
            SILVA_EXPECT_FWD(s_pt->get_children<1>(rule.expr_node_index));
        SILVA_EXPECT(s_nodes[deriv_3_child[0]].rule_name == fni_axe_spec,
                     MINOR,
                     "Child of Derivation,3 must be AxeSpec");
        retval->parse_axes[rule.rule_name] =
            SILVA_EXPECT_FWD(impl::create_parse_axe(tcp, rule.rule_name, s_pt, deriv_3_child[0]));
      }
    }

    // Sanity check on alias rules.
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

    // Pre-compile hashmap_t of "regexes".
    for (index_t node_index = 0; node_index < s_pt->nodes.size(); ++node_index) {
      const auto& node = s_pt->nodes[node_index];
      if (node.rule_name == fni_regex) {
        const token_id_t regex_token_id = s_pt->tokenization->tokens[node.token_begin];
        if (auto& regex = retval->regexes[regex_token_id]; !regex.has_value()) {
          const auto& regex_td = s_pt->tokenization->token_info_get(node.token_begin);
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
        auto gg               = guard();
        const auto& seed_node = seed_pt->nodes[seed_node_index];
        SILVA_EXPECT_PARSE(token_index < retval.tokenization->tokens.size(),
                           "Reached end of token-stream when looking for {}",
                           seed_pt->tokenization->token_info_get(seed_node.token_begin)->str);
        if (seed_node.rule_name == fni_term_0) {
          SILVA_EXPECT(seed_node.num_children == 1,
                       MAJOR,
                       "Expected Seed node TERIMNAL_0 to have exactly one child");
          const array_t<index_t, 1> seed_node_index_regex =
              SILVA_EXPECT_FWD(seed_pt->get_children<1>(seed_node_index));
          const auto& seed_regex_node = seed_pt->nodes[seed_node_index_regex[0]];
          const token_id_t regex_token_id =
              seed_pt->tokenization->tokens[seed_regex_node.token_begin];
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
          const token_id_t seed_token_id = seed_pt->tokenization->tokens[seed_node.token_begin];
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
                seed_pt->tokenization->token_info_get(seed_node.token_begin);
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
        auto gg               = guard();
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
              apply_rule(seed_pt->tokenization->tokens[nonterminal_node.token_begin]));
        }
        else {
          SILVA_EXPECT(false, MAJOR);
        }
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_derivation_1(const index_t seed_node_index)
      {
        auto gg          = guard();
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
        auto gg = guard();
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
                    seed_pt->tokenization->token_info_get(seed_node_suffix.token_begin)->str;
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
                auto inner_ptg = guard();
                const auto result =
                    SILVA_EXPECT_FWD_IF(apply_primary(seed_node_index_primary), MAJOR);
                SILVA_EXPECT(!result, MINOR, "Managed to parse negative primary expression");
              }
              else if (suffix_char.value() == '&') {
                auto inner_ptg = guard();
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

      expected_t<parse_tree_sub_t> apply_derivation_3(const parse_root_t::rule_t& rule)
      {
        const auto it = root->parse_axes.find(rule.rule_name);
        SILVA_EXPECT(it != root->parse_axes.end(), MAJOR);
        auto gg{guard()};
        const parse_root_t::parse_axe_data_t& parse_axe_data = it->second;
        using dg_t = delegate_t<expected_t<parse_tree_sub_t>()>;
        struct callback_t {
          token_id_t atom_rule       = token_id_none;
          parse_root_nursery_t* self = nullptr;
          expected_t<parse_tree_sub_t> operator()() const { return self->apply_rule(atom_rule); }
        };
        callback_t callback{
            .atom_rule = parse_axe_data.atom_rule,
            .self      = this,
        };
        gg.sub += SILVA_EXPECT_FWD(parse_axe_data.parse_axe.apply(
            *this,
            tcp->full_name_id(full_name_id_none, parse_axe_data.atom_rule),
            dg_t::make<&callback_t::operator()>(&callback)));
        return gg.release();
      }

      expected_t<parse_tree_sub_t> apply_expr(const parse_root_t::rule_t& rule)
      {
        const parse_tree_t::node_t& seed_node_expr = seed_pt->nodes[rule.expr_node_index];
        if (seed_node_expr.rule_name == fni_deriv_0) {
          return apply_derivation_0(tcp->full_name_to_string(rule.rule_name), rule.expr_node_index);
        }
        else if (seed_node_expr.rule_name == fni_deriv_1) {
          return apply_derivation_1(rule.expr_node_index);
        }
        else if (seed_node_expr.rule_name == fni_deriv_3) {
          return apply_derivation_3(rule);
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
          auto gg_rule      = guard_for_rule();
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
                .finish_short(retval_error_level,
                              "{} Expected {} rule",
                              token_position_at(orig_token_index),
                              tcp->full_name_to_string(root->rules[base_rule_offset].rule_name)));
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
