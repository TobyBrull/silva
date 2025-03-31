#include "seed_engine.hpp"

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

  struct seed_engine_create_nursery_t {
    shared_ptr_t<const parse_tree_t> seed_parse_tree;
    token_context_ptr_t tcp = seed_parse_tree->tokenization->context;
    name_id_style_t fnis    = seed_name_style(tcp);

    std::unique_ptr<seed_engine_t> retval = std::make_unique<seed_engine_t>();

    const tokenization_t& s_tokenization = *seed_parse_tree->tokenization;
    const vector_t<token_id_t>& s_tokens = s_tokenization.tokens;

    const vector_t<shared_ptr_t<const parse_tree_t>>& spts = retval->seed_parse_trees;

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

    const name_id_t fni_seed        = tcp->name_id_of("Seed");
    const name_id_t fni_rule        = tcp->name_id_of(fni_seed, "Rule");
    const name_id_t fni_expr_or_a   = tcp->name_id_of(fni_seed, "ExprOrAlias");
    const name_id_t fni_expr        = tcp->name_id_of(fni_seed, "Expr");
    const name_id_t fni_atom        = tcp->name_id_of(fni_seed, "Atom");
    const name_id_t fni_axe         = tcp->name_id_of(fni_seed, "Axe");
    const name_id_t fni_axe_level   = tcp->name_id_of(fni_axe, "Level");
    const name_id_t fni_axe_assoc   = tcp->name_id_of(fni_axe, "Assoc");
    const name_id_t fni_axe_ops     = tcp->name_id_of(fni_axe, "Ops");
    const name_id_t fni_axe_op_type = tcp->name_id_of(fni_axe, "OpType");
    const name_id_t fni_axe_op      = tcp->name_id_of(fni_axe, "Op");
    const name_id_t fni_nt          = tcp->name_id_of(fni_seed, "Nonterminal");
    const name_id_t fni_nt_base     = tcp->name_id_of(fni_nt, "Base");
    const name_id_t fni_term        = tcp->name_id_of(fni_seed, "Terminal");

    seed_engine_create_nursery_t(shared_ptr_t<const parse_tree_t> seed_parse_tree)
      : seed_parse_tree(seed_parse_tree)
    {
      retval->seed_parse_trees.push_back(std::move(seed_parse_tree));
    }

    expected_t<void> axe_ops(parse_axe::parse_axe_level_desc_t& level,
                             const parse_tree_span_t pts_axe_ops)
    {
      token_id_t axe_op_type = token_id_none;
      vector_t<token_id_t> axe_op_vec;
      for (const auto [child_node_index, child_index]: pts_axe_ops.children_range()) {
        if (child_index == 0) {
          SILVA_EXPECT(pts_axe_ops[child_node_index].rule_name == fni_axe_op_type, MINOR);
          axe_op_type = s_tokens[pts_axe_ops[child_node_index].token_begin];
          SILVA_EXPECT(axe_op_type == ti_atom_nest || axe_op_type == ti_prefix ||
                           axe_op_type == ti_prefix_nest || axe_op_type == ti_infix ||
                           axe_op_type == ti_infix_flat || axe_op_type == ti_ternary ||
                           axe_op_type == ti_postfix || axe_op_type == ti_postfix_nest,
                       MINOR,
                       "Expected one of [ atom_nest prefix infix postfix ... ]");
        }
        else {
          SILVA_EXPECT(pts_axe_ops[child_node_index].rule_name == fni_axe_op, MINOR);
          const token_id_t axe_op  = s_tokens[pts_axe_ops[child_node_index].token_begin];
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
      }

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
                               const name_id_t scope_name,
                               const parse_tree_span_t pts_axe_level)
    {
      for (const auto [child_node_index, child_index]: pts_axe_level.children_range()) {
        if (child_index == 0) {
          SILVA_EXPECT(pts_axe_level[child_node_index].rule_name == fni_nt_base, MINOR);
          level.name = SILVA_EXPECT_FWD(
              derive_name_base(scope_name, pts_axe_level.sub_tree_span_at(child_node_index)));
        }
        else if (child_index == 1) {
          SILVA_EXPECT(pts_axe_level[child_node_index].rule_name == fni_axe_assoc, MINOR);
          const token_id_t assoc = s_tokens[pts_axe_level[child_node_index].token_begin];
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
          SILVA_EXPECT(pts_axe_level[child_node_index].rule_name == fni_axe_ops, MINOR);
          SILVA_EXPECT_FWD(axe_ops(level, pts_axe_level.sub_tree_span_at(child_node_index)));
        }
      }
      return {};
    }

    expected_t<seed_engine_t::parse_axe_data_t> create_parse_axe(const name_id_t scope_name,
                                                                 const name_id_t rule_name,
                                                                 const parse_tree_span_t pts_axe)
    {
      const auto& s_node = pts_axe[0];
      SILVA_EXPECT(s_node.rule_name == fni_axe, MINOR);
      vector_t<parse_axe::parse_axe_level_desc_t> level_descs;
      SILVA_EXPECT(s_node.num_children >= 1, MINOR);
      level_descs.reserve(s_node.num_children - 1);
      name_id_t atom_rule_name = name_id_root;
      for (const auto [child_node_index, child_index]: pts_axe.children_range()) {
        if (child_index == 0) {
          SILVA_EXPECT(pts_axe[child_node_index].rule_name == fni_nt, MINOR);
          atom_rule_name =
              SILVA_EXPECT_FWD(derive_name(scope_name, pts_axe.sub_tree_span_at(child_node_index)));
        }
        else {
          SILVA_EXPECT(pts_axe[child_node_index].rule_name == fni_axe_level, MINOR);
          auto& curr_level = level_descs.emplace_back();
          SILVA_EXPECT_FWD(
              axe_level(curr_level, rule_name, pts_axe.sub_tree_span_at(child_node_index)));
        }
      }
      SILVA_EXPECT(atom_rule_name != name_id_root, MAJOR);
      auto pa = SILVA_EXPECT_FWD(parse_axe::parse_axe_create(tcp, std::move(level_descs)));
      return {{
          .atom_rule_name = atom_rule_name,
          .parse_axe      = std::move(pa),
      }};
    }

    expected_t<name_id_t> derive_name_base(const name_id_t scope_name,
                                           const parse_tree_span_t pts_nonterminal_base)
    {
      name_id_t retval   = scope_name;
      const auto& s_node = pts_nonterminal_base[0];
      SILVA_EXPECT(s_node.rule_name == fni_nt_base && s_node.num_children == 0,
                   MINOR,
                   "Expected Nonterminal.Base");
      const token_id_t base = s_tokenization.tokens[s_node.token_begin];
      if (base == fnis.current) {
        return scope_name;
      }
      else {
        return tcp->name_id(scope_name, base);
      }
      return retval;
    }

    expected_t<name_id_t> derive_name(const name_id_t scope_name,
                                      const parse_tree_span_t pts_nonterminal)
    {
      name_id_t retval = scope_name;
      SILVA_EXPECT(pts_nonterminal[0].rule_name == fni_nt, MINOR, "Expected Nonterminal");
      for (const auto [child_node_index, child_index]: pts_nonterminal.children_range()) {
        const auto& s_node = pts_nonterminal[child_node_index];
        SILVA_EXPECT(s_node.rule_name == fni_nt_base, MINOR, "Expected Nonterminal.Base");
        const token_id_t base = s_tokenization.tokens[s_node.token_begin];
        if (base == fnis.root) {
          SILVA_EXPECT(child_index == 0, MINOR, "Root node may only appear as first element");
          retval = name_id_root;
        }
        else if (base == fnis.current) {
          ;
        }
        else if (base == fnis.parent) {
          retval = tcp->name_infos[retval].parent_name;
        }
        else {
          retval = tcp->name_id(retval, base);
        }
      }
      return retval;
    }

    expected_t<void> recognize_keyword(name_id_t rule_name, const token_id_t keyword)
    {
      while (true) {
        retval->keywords[rule_name].insert(keyword);
        if (rule_name == name_id_root) {
          break;
        }
        rule_name = tcp->name_infos[rule_name].parent_name;
      }
      return {};
    }

    expected_t<void> handle_rule(const name_id_t scope_name, const parse_tree_span_t pts_rule)
    {
      SILVA_EXPECT(pts_rule[0].rule_name == fni_rule, MINOR, "Expected Rule");
      const auto children = SILVA_EXPECT_FWD(pts_rule.get_children<2>());
      SILVA_EXPECT(pts_rule[children[0]].rule_name == fni_nt_base,
                   MINOR,
                   "First child of Rule must be Nonterminal.Base");
      const name_id_t curr_rule_name =
          SILVA_EXPECT_FWD(derive_name_base(scope_name, pts_rule.sub_tree_span_at(children[0])));
      const index_t expr_rule_name = pts_rule[children[1]].rule_name;
      if (expr_rule_name == fni_seed) {
        SILVA_EXPECT_FWD(handle_seed(curr_rule_name, pts_rule.sub_tree_span_at(children[1])));
      }
      else {
        SILVA_EXPECT(expr_rule_name == fni_axe || expr_rule_name == fni_expr_or_a,
                     MINOR,
                     "Second child of Rule must be one of [ Axe Seed ExprOrAlias ]");
        const auto [it, inserted] =
            retval->rule_exprs.emplace(curr_rule_name, pts_rule.sub_tree_span_at(children[1]));
        SILVA_EXPECT(inserted, MINOR, "Repeated rule name '{}'", fnis.absolute(curr_rule_name));

        const auto pts_expr = pts_rule.sub_tree_span_at(children[1]);

        for (index_t i = 0; i < pts_expr.size(); ++i) {
          if (pts_expr[i].rule_name == fni_term) {
            const index_t token_idx        = pts_expr[i].token_begin;
            const token_id_t token_id      = pts_expr.tokenization->tokens[token_idx];
            const token_info_t& token_info = tcp->token_infos[token_id];
            if (token_info.category == token_category_t::STRING) {
              const auto keyword = SILVA_EXPECT_FWD(tcp->token_id_unquoted(token_id));
              SILVA_EXPECT_FWD(recognize_keyword(scope_name, keyword));
            }
          }
        }

        if (expr_rule_name == fni_axe) {
          retval->parse_axes[curr_rule_name] =
              SILVA_EXPECT_FWD(create_parse_axe(scope_name, curr_rule_name, pts_expr));
        }
        else {
          for (index_t i = 0; i < pts_expr.size(); ++i) {
            if (pts_expr[i].rule_name == fni_nt) {
              const name_id_t nt_name =
                  SILVA_EXPECT_FWD(derive_name(scope_name, pts_expr.sub_tree_span_at(i)));
              const auto [it, inserted] =
                  retval->nonterminal_rules.emplace(pts_expr.sub_tree_span_at(i), nt_name);
              SILVA_EXPECT(inserted, MAJOR);
            }
          }
        }
      }
      return {};
    }

    expected_t<void> handle_seed(const name_id_t scope_name, const parse_tree_span_t pts_seed)
    {
      SILVA_EXPECT(pts_seed.size() != 0 && pts_seed[0].rule_name == fni_seed,
                   MINOR,
                   "Seed parse_tree should start with Seed node");

      for (const auto [rule_node_index, child_index]: pts_seed.children_range()) {
        SILVA_EXPECT_FWD(handle_rule(scope_name, pts_seed.sub_tree_span_at(rule_node_index)));
      }
      return {};
    }

    expected_t<void> handle_all()
    {
      const parse_tree_span_t pts = seed_parse_tree->span();
      SILVA_EXPECT_FWD(handle_seed(name_id_root, pts));

      // Pre-compile hashmap_t of "regexes".
      for (index_t node_index = 0; node_index < pts.size(); ++node_index) {
        const auto& s_node = pts[node_index];
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

  expected_t<unique_ptr_t<seed_engine_t>>
  seed_engine_t::create(shared_ptr_t<const parse_tree_t> seed_parse_tree)
  {
    seed_engine_create_nursery_t nursery(std::move(seed_parse_tree));
    SILVA_EXPECT_FWD(nursery.handle_all());
    return std::move(nursery).retval;
  }

  expected_t<unique_ptr_t<seed_engine_t>>
  seed_engine_t::create(token_context_ptr_t tcp, filesystem_path_t filepath, string_t text)
  {
    auto tt = SILVA_EXPECT_FWD(tokenize(tcp, std::move(filepath), std::move(text)));
    auto pt = SILVA_EXPECT_FWD(seed_parse(std::move(tt)));
    // const auto x = SILVA_EXPECT_FWD(parse_tree_to_string(*pt));
    // fmt::print("{}\n", x);
    auto retval = SILVA_EXPECT_FWD(seed_engine_t::create(std::move(pt)));
    return retval;
  }

  namespace impl {
    struct seed_engine_nursery_t : public parse_tree_nursery_t {
      const seed_engine_t* root = nullptr;
      token_context_ptr_t tcp   = root->seed_parse_trees.front()->tokenization->context;
      name_id_style_t fnis      = seed_name_style(tcp);

      const tokenization_t& s_tokenization = *root->seed_parse_trees.front()->tokenization;
      const vector_t<token_id_t>& s_tokens = s_tokenization.tokens;

      const tokenization_t& t_tokenization = *tokenization;
      const vector_t<token_id_t>& t_tokens = t_tokenization.tokens;

      const vector_t<shared_ptr_t<const parse_tree_t>>& spts = root->seed_parse_trees;

      int rule_depth = 0;

      const token_id_t ti_id       = tcp->token_id("identifier");
      const token_id_t ti_op       = tcp->token_id("operator");
      const token_id_t ti_string   = tcp->token_id("string");
      const token_id_t ti_number   = tcp->token_id("number");
      const token_id_t ti_any      = tcp->token_id("any");
      const token_id_t ti_eof      = tcp->token_id("end_of_file");
      const token_id_t ti_ques     = tcp->token_id("?");
      const token_id_t ti_star     = tcp->token_id("*");
      const token_id_t ti_plus     = tcp->token_id("+");
      const token_id_t ti_not      = tcp->token_id("not");
      const token_id_t ti_but_then = tcp->token_id("but_then");
      const token_id_t ti_regex    = tcp->token_id("/");
      const token_id_t ti_equal    = tcp->token_id("=");
      const token_id_t ti_alias    = tcp->token_id("=>");

      const name_id_t fni_seed         = tcp->name_id_of("Seed");
      const name_id_t fni_rule         = tcp->name_id_of(fni_seed, "Rule");
      const name_id_t fni_expr         = tcp->name_id_of(fni_seed, "Expr");
      const name_id_t fni_expr_parens  = tcp->name_id_of(fni_expr, "Parens");
      const name_id_t fni_expr_prefix  = tcp->name_id_of(fni_expr, "Prefix");
      const name_id_t fni_expr_postfix = tcp->name_id_of(fni_expr, "Postfix");
      const name_id_t fni_expr_concat  = tcp->name_id_of(fni_expr, "Concat");
      const name_id_t fni_expr_or      = tcp->name_id_of(fni_expr, "Or");
      const name_id_t fni_expr_and     = tcp->name_id_of(fni_expr, "And");
      const name_id_t fni_atom         = tcp->name_id_of(fni_seed, "Atom");
      const name_id_t fni_axe          = tcp->name_id_of(fni_seed, "Axe");
      const name_id_t fni_axe_level    = tcp->name_id_of(fni_axe, "Level");
      const name_id_t fni_axe_assoc    = tcp->name_id_of(fni_axe, "Assoc");
      const name_id_t fni_axe_ops      = tcp->name_id_of(fni_axe, "Ops");
      const name_id_t fni_axe_op_type  = tcp->name_id_of(fni_axe, "OpType");
      const name_id_t fni_axe_op       = tcp->name_id_of(fni_axe, "Op");
      const name_id_t fni_nt           = tcp->name_id_of(fni_seed, "Nonterminal");
      const name_id_t fni_nt_base      = tcp->name_id_of(fni_nt, "Base");
      const name_id_t fni_term         = tcp->name_id_of(fni_seed, "Terminal");

      seed_engine_nursery_t(shared_ptr_t<const tokenization_t> tokenization,
                            const seed_engine_t* root)
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

      expected_t<parse_tree_node_t> s_terminal(const parse_tree_span_t pts)
      {
        auto ss            = stake();
        const auto& s_node = pts[0];
        SILVA_EXPECT(s_node.num_children == 0, MAJOR, "Expected Terminal node have no children");
        SILVA_EXPECT(s_node.rule_name == fni_term, MAJOR);
        const token_id_t s_front_ti = s_tokens[s_node.token_begin];
        if (s_front_ti == ti_eof) {
          SILVA_EXPECT_PARSE(num_tokens_left() == 0, "Expected end of file");
          return ss.commit();
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
        return ss.commit();
      }

      expected_t<parse_tree_node_t> s_expr_prefix(const parse_tree_span_t pts)
      {
        {
          auto ss             = stake();
          const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
          auto result = SILVA_EXPECT_FWD_IF(s_expr(pts.sub_tree_span_at(children[0])), MAJOR);
          SILVA_EXPECT(!result, MINOR, "Successfully parsed 'not' expression");
        }
        auto ss = stake();
        return ss.commit();
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

      expected_t<parse_tree_node_t> s_expr_postfix(const parse_tree_span_t pts)
      {
        auto ss                = stake();
        const auto children    = SILVA_EXPECT_FWD(pts.get_children<1>());
        const token_id_t op_ti = tcp->name_infos[pts[0].rule_name].base_name;
        if (op_ti == ti_ques || op_ti == ti_star || op_ti == ti_plus) {
          const auto [min_repeat, max_repeat] = get_min_max_repeat(op_ti);
          index_t repeat_count                = 0;
          optional_t<error_t> last_error;
          while (repeat_count < max_repeat) {
            if (auto result = s_expr(pts.sub_tree_span_at(children[0])); result.has_value()) {
              ss.add_proto_node(*result);
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
        }
        else {
          SILVA_EXPECT(false, MAJOR);
        }
        return ss.commit();
      }

      expected_t<parse_tree_node_t> s_expr_concat(const parse_tree_span_t pts)
      {
        auto ss = stake();
        for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
          ss.add_proto_node(SILVA_EXPECT_FWD(s_expr(pts.sub_tree_span_at(sub_s_node_index))));
        }
        return ss.commit();
      }

      expected_t<parse_tree_node_t> s_expr_and(const parse_tree_span_t pts)
      {
        optional_t<stake_t> ss;
        for (const auto [child_node_index, child_index]: pts.children_range()) {
          ss.emplace(stake());
          ss->add_proto_node(SILVA_EXPECT_FWD(s_expr(pts.sub_tree_span_at(child_node_index))));
        }
        SILVA_EXPECT(ss.has_value(), MAJOR);
        return ss->commit();
      }

      expected_t<parse_tree_node_t> s_expr_or(const parse_tree_span_t pts)
      {
        const index_t orig_token_index = token_index;
        error_nursery_t error_nursery;
        optional_t<parse_tree_node_t> retval;
        for (const auto [sub_s_node_index, child_index]: pts.children_range()) {
          auto result = s_expr(pts.sub_tree_span_at(sub_s_node_index));
          if (result.has_value()) {
            retval = std::move(result).value();
            break;
          }
          else {
            error_nursery.add_child_error(std::move(result).error());
          }
        }
        if (retval.has_value()) {
          return std::move(retval).value();
        }
        return std::unexpected(std::move(error_nursery)
                                   .finish(MINOR,
                                           "{} Expected to parse alternation '{{...}}'",
                                           token_position_at(orig_token_index)));
      }

      expected_t<parse_tree_node_t> s_expr(const parse_tree_span_t pts)
      {
        const name_id_t s_rule_name = pts[0].rule_name;
        if (tcp->name_id_is_parent(fni_expr_parens, s_rule_name)) {
          const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
          return s_expr(pts.sub_tree_span_at(children[0]));
        }
        else if (tcp->name_id_is_parent(fni_expr_prefix, s_rule_name)) {
          return s_expr_prefix(pts);
        }
        else if (tcp->name_id_is_parent(fni_expr_postfix, s_rule_name)) {
          return s_expr_postfix(pts);
        }
        else if (tcp->name_id_is_parent(fni_expr_concat, s_rule_name)) {
          return s_expr_concat(pts);
        }
        else if (tcp->name_id_is_parent(fni_expr_and, s_rule_name)) {
          return s_expr_and(pts);
        }
        else if (tcp->name_id_is_parent(fni_expr_or, s_rule_name)) {
          return s_expr_or(pts);
        }
        else if (s_rule_name == fni_term) {
          return s_terminal(pts);
        }
        else if (s_rule_name == fni_nt) {
          const auto it = root->nonterminal_rules.find(pts);
          SILVA_EXPECT(it != root->nonterminal_rules.end(), MAJOR, "Couldn't lookup nonterminal");
          const name_id_t t_rule_name = it->second;
          return handle_rule(t_rule_name);
        }
        else {
          SILVA_EXPECT(false, MAJOR);
        }
      }

      expected_t<parse_tree_node_t> handle_rule_axe(const name_id_t t_rule_name)
      {
        const auto it = root->parse_axes.find(t_rule_name);
        SILVA_EXPECT(it != root->parse_axes.end(), MAJOR);
        auto ss{stake()};
        const seed_engine_t::parse_axe_data_t& parse_axe_data = it->second;
        const delegate_t<expected_t<parse_tree_node_t>()>::pack_t pack{
            [&]() { return handle_rule(parse_axe_data.atom_rule_name); },
        };
        ss.add_proto_node(SILVA_EXPECT_FWD(
            parse_axe_data.parse_axe.apply(*this, parse_axe_data.atom_rule_name, pack.delegate)));
        return ss.commit();
      }

      expected_t<parse_tree_node_t> handle_rule(const name_id_t t_rule_name)
      {
        rule_depth += 1;
        scope_exit_t scope_exit([this] { rule_depth -= 1; });
        SILVA_EXPECT(rule_depth <= 50,
                     FATAL,
                     "Stack is getting too deep. Infinite recursion in grammar?");
        const index_t orig_token_index = token_index;
        const auto it{root->rule_exprs.find(t_rule_name)};
        SILVA_EXPECT(it != root->rule_exprs.end(),
                     MAJOR,
                     "Unknown rule: {}",
                     fnis.absolute(t_rule_name));
        const parse_tree_span_t pts = it->second;
        const auto& s_node          = pts[0];
        const name_id_t s_expr_name = s_node.rule_name;
        if (s_expr_name == fni_axe) {
          return SILVA_EXPECT_FWD(handle_rule_axe(t_rule_name), "Expected Axe");
        }
        else {
          const token_id_t rule_token = s_tokenization.tokens[s_node.token_begin];
          SILVA_EXPECT(rule_token == ti_equal || rule_token == ti_alias,
                       MAJOR,
                       "Expected one of [ '=' '=>' ]");
          const auto children = SILVA_EXPECT_FWD(pts.get_children<1>());
          if (rule_token == ti_equal) {
            auto ss_rule = stake();
            ss_rule.create_node(t_rule_name);
            ss_rule.add_proto_node(SILVA_EXPECT_FWD(s_expr(pts.sub_tree_span_at(children[0])),
                                                    "{} Expected {}",
                                                    token_position_at(orig_token_index),
                                                    fnis.absolute(t_rule_name)));
            return ss_rule.commit();
          }
          else {
            auto ss = stake();
            ss.add_proto_node(SILVA_EXPECT_FWD(s_expr(pts.sub_tree_span_at(children[0])),
                                               "{} Expected {}",
                                               token_position_at(orig_token_index),
                                               fnis.absolute(t_rule_name)));
            return ss.commit();
          }
        }
      }
    };
  }

  expected_t<unique_ptr_t<parse_tree_t>>
  seed_engine_t::apply(shared_ptr_t<const tokenization_t> tokenization,
                       const name_id_t goal_rule_name) const
  {
    impl::seed_engine_nursery_t nursery(std::move(tokenization), this);
    SILVA_EXPECT_FWD(nursery.check());
    expected_traits_t expected_traits{.materialize_fwd = true};
    const parse_tree_node_t ptn = SILVA_EXPECT_FWD(nursery.handle_rule(goal_rule_name));
    SILVA_EXPECT(ptn.num_children == 1, ASSERT);
    SILVA_EXPECT(ptn.subtree_size == nursery.tree.size(), ASSERT);
    return {std::make_unique<parse_tree_t>(std::move(nursery).finish())};
  }
}
