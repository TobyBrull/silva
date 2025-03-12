#include "parse_root.hpp"

#include "canopy/expected.hpp"
#include "canopy/scope_exit.hpp"
#include "canopy/small_vector.hpp"
#include "parse_axe.hpp"
#include "parse_tree.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

#include <utility>

namespace silva {
  using enum token_category_t;
  using enum error_level_t;

  struct parse_root_create_nursery_t {
    shared_ptr_t<const parse_tree_t> seed_parse_tree;
    token_context_ptr_t tcp = seed_parse_tree->tokenization->context;
    full_name_id_style_t fnis{.tcp = tcp};

    std::unique_ptr<parse_root_t> retval = std::make_unique<parse_root_t>();

    const parse_tree_t& s_pt                      = *seed_parse_tree;
    const tokenization_t& s_tokenization          = *s_pt.tokenization;
    const vector_t<token_id_t>& s_tokens          = s_tokenization.tokens;
    const vector_t<parse_tree_t::node_t>& s_nodes = s_pt.nodes;

    const token_id_t ti_atom_nest    = tcp->token_id("atom_nest");
    const token_id_t ti_prefix       = tcp->token_id("prefix");
    const token_id_t ti_prefix_nest  = tcp->token_id("prefix_nest");
    const token_id_t ti_infix        = tcp->token_id("infix");
    const token_id_t ti_infix_flat   = tcp->token_id("infix_flat");
    const token_id_t ti_ternary      = tcp->token_id("ternary");
    const token_id_t ti_postfix      = tcp->token_id("postfix");
    const token_id_t ti_postfix_nest = tcp->token_id("postfix_nest");
    const token_id_t ti_concat       = tcp->token_id("concat");
    const token_id_t ti_nest         = tcp->token_id("nest");
    const token_id_t ti_ltr          = tcp->token_id("ltr");
    const token_id_t ti_rtl          = tcp->token_id("rtl");

    const full_name_id_t fni_seed        = tcp->full_name_id_of("Seed");
    const full_name_id_t fni_rule        = tcp->full_name_id_of("Rule");
    const full_name_id_t fni_expr        = tcp->full_name_id_of("Expr");
    const full_name_id_t fni_atom        = tcp->full_name_id_of("Atom");
    const full_name_id_t fni_alias       = tcp->full_name_id_of("Alias");
    const full_name_id_t fni_axe         = tcp->full_name_id_of("Axe");
    const full_name_id_t fni_axe_level   = tcp->full_name_id_of("AxeLevel");
    const full_name_id_t fni_axe_assoc   = tcp->full_name_id_of("AxeAssoc");
    const full_name_id_t fni_axe_ops     = tcp->full_name_id_of("AxeOps");
    const full_name_id_t fni_axe_op_type = tcp->full_name_id_of("AxeOpType");
    const full_name_id_t fni_axe_op      = tcp->full_name_id_of("AxeOp");
    const full_name_id_t fni_nonterm     = tcp->full_name_id_of("Nonterminal");
    const full_name_id_t fni_term        = tcp->full_name_id_of("Terminal");

    parse_root_create_nursery_t(shared_ptr_t<const parse_tree_t> seed_parse_tree)
      : seed_parse_tree(seed_parse_tree)
    {
      retval->seed_parse_tree = std::move(seed_parse_tree);
    }

    expected_t<void> add_rule(const index_t rule_node_index)
    {
      SILVA_EXPECT(s_nodes[rule_node_index].rule_name == fni_rule, MINOR, "Expected Rule");
      const array_t<index_t, 2> children = SILVA_EXPECT_FWD(s_pt.get_children<2>(rule_node_index));
      SILVA_EXPECT(s_nodes[children[0]].rule_name == fni_nonterm,
                   MINOR,
                   "First child of Rule must be Nonterminal");
      const token_id_t rule_token_id = s_tokens[s_nodes[children[0]].token_begin];
      const full_name_id_t rule_name = tcp->full_name_id(full_name_id_none, rule_token_id);
      const index_t expr_rule_name   = s_nodes[children[1]].rule_name;
      SILVA_EXPECT(
          expr_rule_name == fni_axe || expr_rule_name == fni_alias ||
              tcp->full_name_id_is_parent(fni_expr, expr_rule_name) ||
              expr_rule_name == fni_nonterm || expr_rule_name == fni_term,
          MINOR,
          "Second child of Rule must be one of [ Expr... Axe Alias Nonterminal Terminal ]");
      if (retval->rules.empty()) {
        retval->goal_rule = rule_name;
      }
      const index_t offset = retval->rules.size();
      retval->rules.push_back(parse_root_t::rule_t{
          .name            = rule_name,
          .expr_node_index = children[1],
      });
      const auto [it, inserted] = retval->rule_indexes.emplace(rule_name, offset);
      SILVA_EXPECT(inserted, MINOR, "Repeated rule name '{}'", fnis.absolute(rule_name));
      return {};
    }

    expected_t<void> axe_ops(parse_axe::parse_axe_level_desc_t& level,
                             const index_t axe_ops_node_index)
    {
      token_id_t axe_op_type = token_id_none;
      vector_t<token_id_t> axe_op_vec;
      auto result = s_pt.visit_children(
          [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
            if (child_index == 0) {
              SILVA_EXPECT(s_nodes[child_node_index].rule_name == fni_axe_op_type, MINOR);
              axe_op_type = s_tokens[s_nodes[child_node_index].token_begin];
              SILVA_EXPECT(axe_op_type == ti_atom_nest || axe_op_type == ti_prefix ||
                               axe_op_type == ti_prefix_nest || axe_op_type == ti_infix ||
                               axe_op_type == ti_infix_flat || axe_op_type == ti_ternary ||
                               axe_op_type == ti_postfix || axe_op_type == ti_postfix_nest,
                           MINOR,
                           "Expected one of [ atom_nest prefix infix postfix ... ]");
            }
            else {
              SILVA_EXPECT(s_nodes[child_node_index].rule_name == fni_axe_op, MINOR);
              const token_id_t axe_op  = s_tokens[s_nodes[child_node_index].token_begin];
              const token_info_t* info = &tcp->token_infos[axe_op];
              SILVA_EXPECT(info->category == token_category_t::STRING || axe_op == ti_concat,
                           MINOR,
                           "Expected 'concat' or string");
              if (axe_op_type != ti_infix && axe_op_type != ti_infix_flat) {
                SILVA_EXPECT(
                    axe_op != ti_concat,
                    MINOR,
                    "The 'concat' token may only be used with 'infix' or 'infix_flat' operations.");
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
                     "For operations [ atom_nest prefix_nest ternary postfix_nest ] "
                     "an even number of operators is expected");
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
        else if (axe_op_type == ti_infix || axe_op_type == ti_infix_flat) {
          const bool flatten = (axe_op_type == ti_infix_flat);
          if (axe_op_vec[i] == ti_concat) {
            level.opers.push_back(parse_axe::infix_t{
                .token_id = ti_concat,
                .concat   = true,
                .flatten  = flatten,
            });
          }
          else {
            level.opers.push_back(parse_axe::infix_t{
                .token_id = SILVA_EXPECT_FWD(tcp->token_id_unquoted(axe_op_vec[i])),
                .flatten  = flatten,
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

    expected_t<void> axe_level(parse_axe::parse_axe_level_desc_t& level,
                               const full_name_id_t base_name,
                               const index_t axe_level_node_index)
    {
      auto result = s_pt.visit_children(
          [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
            if (child_index == 0) {
              SILVA_EXPECT(s_nodes[child_node_index].rule_name == fni_nonterm, MINOR);
              const token_id_t nonterminal = s_tokens[s_nodes[child_node_index].token_begin];
              level.name                   = tcp->full_name_id(base_name, nonterminal);
            }
            else if (child_index == 1) {
              SILVA_EXPECT(s_nodes[child_node_index].rule_name == fni_axe_assoc, MINOR);
              const token_id_t assoc = s_tokens[s_nodes[child_node_index].token_begin];
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
                SILVA_EXPECT(false, MINOR);
              }
            }
            else {
              SILVA_EXPECT(s_nodes[child_node_index].rule_name == fni_axe_ops, MINOR);
              SILVA_EXPECT_FWD(axe_ops(level, child_node_index));
            }
            return true;
          },
          axe_level_node_index);
      SILVA_EXPECT_FWD(std::move(result));
      return {};
    }

    expected_t<parse_root_t::parse_axe_data_t> create_parse_axe(const full_name_id_t base_name,
                                                                const index_t axe_node_index)
    {
      SILVA_EXPECT(s_nodes[axe_node_index].rule_name == fni_axe, MINOR);
      vector_t<parse_axe::parse_axe_level_desc_t> level_descs;
      SILVA_EXPECT(s_nodes[axe_node_index].num_children >= 1, MINOR);
      level_descs.reserve(s_nodes[axe_node_index].num_children - 1);
      full_name_id_t atom_rule_name = full_name_id_none;
      auto result                   = s_pt.visit_children(
          [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
            if (child_index == 0) {
              SILVA_EXPECT(s_nodes[child_node_index].rule_name == fni_nonterm, MINOR);
              const token_id_t nonterminal = s_tokens[s_nodes[child_node_index].token_begin];
              atom_rule_name               = tcp->full_name_id(full_name_id_none, nonterminal);
            }
            else {
              SILVA_EXPECT(s_nodes[child_node_index].rule_name == fni_axe_level, MINOR);
              auto& curr_level = level_descs.emplace_back();
              SILVA_EXPECT_FWD(axe_level(curr_level, base_name, child_node_index));
            }
            return true;
          },
          axe_node_index);
      SILVA_EXPECT_FWD(std::move(result));
      auto pa = SILVA_EXPECT_FWD(parse_axe::parse_axe_create(tcp, std::move(level_descs)));
      return {{
          .atom_rule_name = atom_rule_name,
          .parse_axe      = std::move(pa),
      }};
    }

    expected_t<void> go()
    {
      SILVA_EXPECT(!s_nodes.empty() && s_nodes.front().rule_name == fni_seed,
                   MINOR,
                   "Seed parse_tree should start with Seed node");

      auto result = s_pt.visit_children(
          [&](const index_t rule_node_index, index_t) -> expected_t<bool> {
            SILVA_EXPECT_FWD(add_rule(rule_node_index));
            return true;
          },
          0);
      SILVA_EXPECT_FWD(std::move(result));

      // Pre-compile hashmap_t of "parse_axes".
      for (const auto& rule: retval->rules) {
        const full_name_id_t rule_name = s_nodes[rule.expr_node_index].rule_name;
        if (rule_name == fni_axe) {
          retval->parse_axes[rule.name] =
              SILVA_EXPECT_FWD(create_parse_axe(rule.name, rule.expr_node_index));
        }
      }

      // Pre-compile hashmap_t of "regexes".
      for (index_t node_index = 0; node_index < s_nodes.size(); ++node_index) {
        const auto& s_node = s_nodes[node_index];
        if (s_node.rule_name == fni_term && s_node.num_tokens() == 3) {
          const token_id_t regex_token_id = s_tokens[s_node.token_begin + 2];
          if (auto& regex = retval->regexes[regex_token_id]; !regex.has_value()) {
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

  expected_t<unique_ptr_t<parse_root_t>>
  parse_root_t::create(shared_ptr_t<const parse_tree_t> seed_parse_tree)
  {
    parse_root_create_nursery_t nursery(std::move(seed_parse_tree));
    SILVA_EXPECT_FWD(nursery.go());
    return std::move(nursery).retval;
  }

  expected_t<unique_ptr_t<parse_root_t>>
  parse_root_t::create(token_context_ptr_t tcp, filesystem_path_t filepath, string_t text)
  {
    auto tt     = SILVA_EXPECT_FWD(tokenize(tcp, std::move(filepath), std::move(text)));
    auto pt     = SILVA_EXPECT_FWD(seed_parse(std::move(tt)));
    auto retval = SILVA_EXPECT_FWD(parse_root_t::create(std::move(pt)));
    return retval;
  }

  namespace impl {
    struct parse_root_nursery_t : public parse_tree_nursery_t {
      const parse_root_t* root = nullptr;
      const parse_tree_t& s_pt = *root->seed_parse_tree;
      token_context_ptr_t tcp  = s_pt.tokenization->context;
      full_name_id_style_t fnis{.tcp = tcp};

      const tokenization_t& s_tokenization          = *s_pt.tokenization;
      const vector_t<token_id_t>& s_tokens          = s_tokenization.tokens;
      const vector_t<parse_tree_t::node_t>& s_nodes = s_pt.nodes;

      const tokenization_t& t_tokenization = *retval.tokenization;
      const vector_t<token_id_t>& t_tokens = t_tokenization.tokens;

      int rule_depth = 0;

      const token_id_t ti_id     = tcp->token_id("identifier");
      const token_id_t ti_op     = tcp->token_id("operator");
      const token_id_t ti_string = tcp->token_id("string");
      const token_id_t ti_number = tcp->token_id("number");
      const token_id_t ti_any    = tcp->token_id("any");
      const token_id_t ti_eof    = tcp->token_id("end_of_file");
      const token_id_t ti_ques   = tcp->token_id("?");
      const token_id_t ti_star   = tcp->token_id("*");
      const token_id_t ti_plus   = tcp->token_id("+");
      const token_id_t ti_excl   = tcp->token_id("!");
      const token_id_t ti_ampr   = tcp->token_id("&");
      const token_id_t ti_regex  = tcp->token_id("/");

      const full_name_id_t fni_seed         = tcp->full_name_id_of("Seed");
      const full_name_id_t fni_rule         = tcp->full_name_id_of("Rule");
      const full_name_id_t fni_expr_parens  = tcp->full_name_id_of("Expr", "Parens");
      const full_name_id_t fni_expr_postfix = tcp->full_name_id_of("Expr", "Postfix");
      const full_name_id_t fni_expr_concat  = tcp->full_name_id_of("Expr", "Concat");
      const full_name_id_t fni_expr_alt     = tcp->full_name_id_of("Expr", "Alt");
      const full_name_id_t fni_atom         = tcp->full_name_id_of("Atom");
      const full_name_id_t fni_alias        = tcp->full_name_id_of("Alias");
      const full_name_id_t fni_axe          = tcp->full_name_id_of("Axe");
      const full_name_id_t fni_axe_level    = tcp->full_name_id_of("AxeLevel");
      const full_name_id_t fni_axe_assoc    = tcp->full_name_id_of("AxeAssoc");
      const full_name_id_t fni_axe_ops      = tcp->full_name_id_of("AxeOps");
      const full_name_id_t fni_axe_op_type  = tcp->full_name_id_of("AxeOpType");
      const full_name_id_t fni_axe_op       = tcp->full_name_id_of("AxeOp");
      const full_name_id_t fni_nonterm      = tcp->full_name_id_of("Nonterminal");
      const full_name_id_t fni_term         = tcp->full_name_id_of("Terminal");

      parse_root_nursery_t(shared_ptr_t<const tokenization_t> tokenization,
                           const parse_root_t* root)
        : parse_tree_nursery_t(tokenization), root(root)
      {
      }

      expected_t<void> check()
      {
        SILVA_EXPECT(s_tokenization.context.get() == t_tokenization.context.get(),
                     MAJOR,
                     "Seed and target parse-trees/tokenizations must be in same token_context_t");
        return {};
      }

      expected_t<parse_tree_sub_t> s_terminal(const index_t s_node_index)
      {
        auto gg            = guard();
        const auto& s_node = s_nodes[s_node_index];
        SILVA_EXPECT(s_node.num_children == 0, MAJOR, "Expected Terminal node have no children");
        SILVA_EXPECT(s_node.rule_name == fni_term, MAJOR);
        const token_id_t s_front_ti = s_tokens[s_node.token_begin];
        if (s_front_ti == ti_eof) {
          SILVA_EXPECT_PARSE(num_tokens_left() == 0, "Expected end of file");
          return gg.release();
        }
        SILVA_EXPECT_PARSE(num_tokens_left() > 0,
                           "Reached end of token-stream when looking for {}",
                           s_tokenization.token_info_get(s_node.token_begin)->str);
        if (s_node.num_tokens() == 3) {
          if (s_front_ti == ti_id) {
            SILVA_EXPECT_PARSE(token_data_by()->category == IDENTIFIER, "Expected identifier");
          }
          else if (s_front_ti == ti_op) {
            SILVA_EXPECT_PARSE(token_data_by()->category == OPERATOR, "Expected operator");
          }
          else {
            SILVA_EXPECT(false, MAJOR, "Only 'identifier' and 'operator' may have regexes");
          }
          const token_id_t regex_token_id = s_tokens[s_node.token_begin + 2];
          const auto it                   = root->regexes.find(regex_token_id);
          SILVA_EXPECT(it != root->regexes.end() && it->second.has_value(), MAJOR);
          const std::regex& re          = it->second.value();
          const string_view_t token_str = token_data_by()->str;
          const bool is_match           = std::regex_search(token_str.begin(), token_str.end(), re);
          SILVA_EXPECT_PARSE(is_match,
                             "Token \"{}\" does not match regex {}",
                             token_str,
                             tcp->token_infos[regex_token_id].str);
        }
        else {
          SILVA_EXPECT(s_node.num_tokens() == 1,
                       MAJOR,
                       "Terminal nodes must have one or three tokens");
          if (s_front_ti == ti_id) {
            SILVA_EXPECT_PARSE(token_data_by()->category == IDENTIFIER, "Expected identifier");
          }
          else if (s_front_ti == ti_op) {
            SILVA_EXPECT_PARSE(token_data_by()->category == OPERATOR, "Expected operator");
          }
          else if (s_front_ti == ti_string) {
            SILVA_EXPECT_PARSE(token_data_by()->category == STRING, "Expected string");
          }
          else if (s_front_ti == ti_number) {
            SILVA_EXPECT_PARSE(token_data_by()->category == NUMBER, "Expected number");
          }
          else if (s_front_ti == ti_any) {
            ;
          }
          else {
            const auto* s_token_data = s_tokenization.token_info_get(s_node.token_begin);
            SILVA_EXPECT(s_token_data->category == STRING, MAJOR);
            const string_t t_expected{
                SILVA_EXPECT_FWD(s_token_data->string_as_plain_contained(), MINOR)};
            const token_id_t t_expected_ti = tcp->token_id(t_expected);
            SILVA_EXPECT_PARSE(token_id_by() == t_expected_ti, "Expected {}", t_expected);
          }
        }
        token_index += 1;
        return gg.release();
      }

      std::pair<index_t, index_t> get_min_max_repeat(const token_id_t op_ti)
      {
        index_t min_repeat = 0;
        index_t max_repeat = std::numeric_limits<index_t>::max();
        if (op_ti == ti_ques) {
          max_repeat = 1;
        }
        else if (op_ti == ti_star) {
          ;
        }
        else if (op_ti == ti_plus) {
          min_repeat = 1;
        }
        return {min_repeat, max_repeat};
      }

      expected_t<parse_tree_sub_t> s_expr_postfix(const index_t s_expr_node_index)
      {
        auto gg             = guard();
        const auto children = SILVA_EXPECT_FWD(s_pt.get_children<1>(s_expr_node_index));
        const token_id_t op_ti =
            tcp->full_name_infos[s_nodes[s_expr_node_index].rule_name].base_name;
        if (op_ti == ti_ques || op_ti == ti_star || op_ti == ti_plus) {
          const auto [min_repeat, max_repeat] = get_min_max_repeat(op_ti);
          parse_tree_sub_t sub_sub;
          index_t repeat_count = 0;
          optional_t<error_t> last_error;
          while (repeat_count < max_repeat) {
            if (auto result = s_expr(children[0]); result.has_value()) {
              sub_sub += std::move(result).value();
              repeat_count += 1;
            }
            else {
              last_error = std::move(result).error();
              break;
            }
          }
          if (repeat_count < min_repeat) {
            small_vector_t<error_t, 1> maybe_child_error;
            if (last_error.has_value()) {
              maybe_child_error.emplace_back(std::move(last_error).value());
            }
            return std::unexpected(make_error(MINOR,
                                              maybe_child_error,
                                              "min-repeat (={}) not reached, only found {}",
                                              min_repeat,
                                              repeat_count));
          }
          gg.sub += std::move(sub_sub);
        }
        else if (op_ti == ti_excl) {
          auto inner_ptg    = guard();
          const auto result = SILVA_EXPECT_FWD_IF(s_expr(children[0]), MAJOR);
          SILVA_EXPECT(!result, MINOR, "Managed to parse '!' expression");
        }
        else if (op_ti == ti_ampr) {
          auto inner_ptg = guard();
          auto result    = SILVA_EXPECT_FWD_IF(s_expr(children[0]), MAJOR);
          SILVA_EXPECT_FWD(std::move(result), "Did not manage to parse '&' expression");
        }
        else {
          SILVA_EXPECT(false, MAJOR);
        }
        return gg.release();
      }

      expected_t<parse_tree_sub_t> s_expr_concat(const index_t s_expr_node_index)
      {
        auto gg     = guard();
        auto result = s_pt.visit_children(
            [&](const index_t sub_s_node_index, const index_t) -> expected_t<bool> {
              gg.sub += SILVA_EXPECT_FWD(s_expr(sub_s_node_index));
              return true;
            },
            s_expr_node_index);
        SILVA_EXPECT_FWD(std::move(result));
        return gg.release();
      }

      expected_t<parse_tree_sub_t> s_expr_alt(const index_t s_expr_node_index)
      {
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;
        optional_t<parse_tree_sub_t> retval;
        auto result = s_pt.visit_children(
            [&](const index_t sub_s_node_index, const index_t) -> expected_t<bool> {
              auto result = s_expr(sub_s_node_index);
              if (result.has_value()) {
                retval = std::move(result).value();
                return false;
              }
              else {
                error_nursery.add_child_error(std::move(result).error());
              }
              return true;
            },
            s_expr_node_index);
        SILVA_EXPECT_FWD(std::move(result));
        if (retval.has_value()) {
          return std::move(retval).value();
        }
        return std::unexpected(std::move(error_nursery)
                                   .finish(MINOR,
                                           "{} Expected to parse alternation '{{...}}'",
                                           token_position_at(orig_token_index)));
      }

      expected_t<parse_tree_sub_t> s_expr(const index_t s_expr_node_index)
      {
        const full_name_id_t s_rule_name = s_nodes[s_expr_node_index].rule_name;
        if (tcp->full_name_id_is_parent(fni_expr_parens, s_rule_name)) {
          const auto children = SILVA_EXPECT_FWD(s_pt.get_children<1>(s_expr_node_index));
          return s_expr(children[0]);
        }
        else if (tcp->full_name_id_is_parent(fni_expr_postfix, s_rule_name)) {
          return s_expr_postfix(s_expr_node_index);
        }
        else if (tcp->full_name_id_is_parent(fni_expr_concat, s_rule_name)) {
          return s_expr_concat(s_expr_node_index);
        }
        else if (tcp->full_name_id_is_parent(fni_expr_alt, s_rule_name)) {
          return s_expr_alt(s_expr_node_index);
        }
        else if (s_rule_name == fni_term) {
          return s_terminal(s_expr_node_index);
        }
        else if (s_rule_name == fni_nonterm) {
          const full_name_id_t t_rule_name =
              tcp->full_name_id(full_name_id_none,
                                s_tokens[s_nodes[s_expr_node_index].token_begin]);
          return handle_rule(t_rule_name);
        }
        else {
          SILVA_EXPECT(false, MAJOR);
        }
      }

      expected_t<parse_tree_sub_t> handle_rule_axe(const parse_root_t::rule_t& rule)
      {
        const auto it = root->parse_axes.find(rule.name);
        SILVA_EXPECT(it != root->parse_axes.end(), MAJOR);
        auto gg{guard()};
        const parse_root_t::parse_axe_data_t& parse_axe_data = it->second;
        const delegate_t<expected_t<parse_tree_sub_t>()>::pack_t pack{
            [&]() { return handle_rule(parse_axe_data.atom_rule_name); },
        };
        gg.sub += SILVA_EXPECT_FWD(
            parse_axe_data.parse_axe.apply(*this, parse_axe_data.atom_rule_name, pack.delegate));
        return gg.release();
      }

      expected_t<parse_tree_sub_t> handle_rule_alias(const parse_root_t::rule_t& rule)
      {
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;
        optional_t<parse_tree_sub_t> retval;
        auto result = s_pt.visit_children(
            [&](const index_t alias_node_index, const index_t) -> expected_t<bool> {
              SILVA_EXPECT(s_nodes[alias_node_index].rule_name == fni_nonterm,
                           MAJOR,
                           "Expected Nonterminal");
              const full_name_id_t t_rule_name =
                  tcp->full_name_id(full_name_id_none,
                                    s_tokens[s_nodes[alias_node_index].token_begin]);
              auto result = handle_rule(t_rule_name);
              if (result.has_value()) {
                retval = std::move(result).value();
                return false;
              }
              else {
                error_nursery.add_child_error(std::move(result).error());
              }
              return true;
            },
            rule.expr_node_index);
        SILVA_EXPECT_FWD(std::move(result));
        if (retval.has_value()) {
          return std::move(retval).value();
        }
        return std::unexpected(std::move(error_nursery)
                                   .finish(MINOR,
                                           "{} Expected to parse alias '{{...}}'",
                                           token_position_at(orig_token_index)));
      }

      expected_t<parse_tree_sub_t> handle_rule(const full_name_id_t t_rule_name)
      {
        rule_depth += 1;
        scope_exit_t scope_exit([this] { rule_depth -= 1; });
        SILVA_EXPECT(rule_depth <= 50,
                     FATAL,
                     "Stack is getting too deep. Infinite recursion in grammar?");
        const index_t orig_token_index = token_index;
        const auto it{root->rule_indexes.find(t_rule_name)};
        SILVA_EXPECT(it != root->rule_indexes.end(),
                     MAJOR,
                     "Unknown rule: {}",
                     fnis.absolute(t_rule_name));
        const auto& rule                 = root->rules[it->second];
        const full_name_id_t s_expr_name = s_nodes[rule.expr_node_index].rule_name;
        if (s_expr_name == fni_alias) {
          return SILVA_EXPECT_FWD(handle_rule_alias(rule));
        }
        else if (s_expr_name == fni_axe) {
          return SILVA_EXPECT_FWD(handle_rule_axe(rule), "Expected Axe");
        }
        else {
          auto gg_rule = guard_for_rule();
          gg_rule.set_rule_name(rule.name);
          gg_rule.sub += SILVA_EXPECT_FWD(s_expr(rule.expr_node_index),
                                          "{} Expected {}",
                                          token_position_at(orig_token_index),
                                          fnis.absolute(rule.name));
          return gg_rule.release();
        }
      }
    };
  }

  expected_t<unique_ptr_t<parse_tree_t>>
  parse_root_t::apply(shared_ptr_t<const tokenization_t> tokenization) const
  {
    impl::parse_root_nursery_t nursery(std::move(tokenization), this);
    SILVA_EXPECT_FWD(nursery.check());
    expected_traits_t expected_traits{.materialize_fwd = true};
    const parse_tree_sub_t sub = SILVA_EXPECT_FWD(nursery.handle_rule(goal_rule));
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.num_children_total == nursery.retval.nodes.size(), ASSERT);
    return {std::make_unique<parse_tree_t>(std::move(nursery.retval))};
  }
}
