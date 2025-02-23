#include "parse_axe.hpp"
#include "canopy/variant.hpp"
#include <variant>

namespace silva::parse_axe {
  bool operator<(const precedence_t& lhs, const precedence_t& rhs)
  {
    if (lhs.level_index < rhs.level_index) {
      return true;
    }
    else if (lhs.level_index > rhs.level_index) {
      return false;
    }
    else {
      // Each level has a unique associativity.
      SILVA_ASSERT(lhs.assoc == rhs.assoc);
      return lhs.assoc == assoc_t::RIGHT_TO_LEFT;
    }
  };

  expected_t<parse_axe_t> parse_axe_create(token_context_ptr_t tcp,
                                           const full_name_id_t parse_axe_name,
                                           const vector_t<parse_axe_level_desc_t>& level_descs)
  {
    using enum assoc_t;
    using enum error_level_t;
    bool may_be_nest = true;
    for (const parse_axe_level_desc_t& level_desc: level_descs) {
      for (const auto& oper: level_desc.opers) {
        if (level_desc.assoc == NEST) {
          SILVA_EXPECT(may_be_nest,
                       MINOR,
                       "The NEST levels must all occur before all other levels");
          SILVA_EXPECT(variant_holds_t<atom_nest_t>{}(oper),
                       MINOR,
                       "NEST level only allows operators of type atom_nest_t");
        }
        else {
          may_be_nest = false;
          if (level_desc.assoc == LEFT_TO_RIGHT) {
            SILVA_EXPECT((variant_holds_t<postfix_t, postfix_nest_t, infix_t, ternary_t>{}(oper)),
                         MINOR,
                         "LEFT_TO_RIGHT level only allows operators "
                         "of type postfix_t, postfix_nest_t, "
                         "infix_t, and ternary_t");
          }
          else if (level_desc.assoc == RIGHT_TO_LEFT) {
            SILVA_EXPECT((variant_holds_t<prefix_t, prefix_nest_t, infix_t, ternary_t>{}(oper)),
                         MINOR,
                         "RIGHT_TO_LEFT level only allows operators "
                         "of type prefix_t, prefix_nest_t, "
                         "infix_t, and ternary_t");
          }
          else if (level_desc.assoc == FLAT) {
            SILVA_EXPECT(variant_holds_t<infix_t>{}(oper),
                         MINOR,
                         "FLAT level only allows operators of type infix_t");
          }
          else {
            SILVA_EXPECT(false, ASSERT, "Unknown level {}", std::to_underlying(level_desc.assoc));
          }
        }
      }
    }

    parse_axe_t retval{
        .tcp  = tcp,
        .name = parse_axe_name,
    };

    const auto register_op = [&retval](const token_id_t token_id,
                                       const oper_any_t oper,
                                       const full_name_id_t level_name,
                                       const precedence_t precedence) -> expected_t<void> {
      auto& result = retval.results[token_id];

      if (variant_holds<oper_prefix_t>(oper)) {
        SILVA_EXPECT(!result.prefix.has_value(),
                     MINOR,
                     "Trying to use token {} used twice as prefix operator",
                     token_id);
        SILVA_EXPECT(!result.is_right_bracket,
                     MINOR,
                     "Trying to use token {} as right-bracket and prefix",
                     token_id);
        result.prefix = result_oper_t<oper_prefix_t>{
            .oper       = variant_get<oper_prefix_t>(oper),
            .level_name = level_name,
            .precedence = precedence,
        };
      }
      else if (variant_holds<oper_regular_t>(oper)) {
        SILVA_EXPECT(!result.regular.has_value(),
                     MINOR,
                     "Trying to use token {} used twice as regular operator",
                     token_id);
        SILVA_EXPECT(!result.is_right_bracket,
                     MINOR,
                     "Trying to use token {} as right-bracket and prefix",
                     token_id);
        result.regular = result_oper_t<oper_regular_t>{
            .oper       = variant_get<oper_regular_t>(oper),
            .level_name = level_name,
            .precedence = precedence,
        };
      }
      else {
        SILVA_EXPECT(false, MAJOR, "Unexpected level: {}", oper.index());
      }
      return {};
    };

    const auto register_right_op = [&retval](const token_id_t token_id) -> expected_t<void> {
      auto& result = retval.results[token_id];
      SILVA_EXPECT(!result.prefix.has_value(),
                   MINOR,
                   "Trying to use token {} as right-bracket and prefix",
                   token_id);
      SILVA_EXPECT(!result.regular.has_value(),
                   MINOR,
                   "Trying to use token {} as right-bracket and regular",
                   token_id);
      SILVA_EXPECT(!result.is_right_bracket,
                   MINOR,
                   "Trying to use token {} as right-bracket twice",
                   token_id);
      result.is_right_bracket = true;
      return {};
    };

    for (index_t i = 0; i < level_descs.size(); ++i) {
      const auto& level_desc          = level_descs[i];
      const level_index_t level_index = level_descs.size() - i;
      const precedence_t precedence{
          .level_index = level_index,
          .assoc       = level_desc.assoc,
      };
      for (const auto& oper: level_desc.opers) {
        if (const auto* x = std::get_if<prefix_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->token_id, *x, level_desc.name, precedence));
        }
        else if (const auto* x = std::get_if<prefix_nest_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->left_bracket, *x, level_desc.name, precedence));
          SILVA_EXPECT_FWD(register_right_op(x->right_bracket));
        }
        else if (const auto* x = std::get_if<atom_nest_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->left_bracket, *x, level_desc.name, precedence));
          SILVA_EXPECT_FWD(register_right_op(x->right_bracket));
        }
        else if (const auto* x = std::get_if<infix_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->token_id, *x, level_desc.name, precedence));
        }
        else if (const auto* x = std::get_if<ternary_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->first, *x, level_desc.name, precedence));
          SILVA_EXPECT_FWD(register_right_op(x->second));
        }
        else if (const auto* x = std::get_if<postfix_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->token_id, *x, level_desc.name, precedence));
        }
        else if (const auto* x = std::get_if<postfix_nest_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->left_bracket, *x, level_desc.name, precedence));
          SILVA_EXPECT_FWD(register_right_op(x->right_bracket));
        }
        else {
          SILVA_EXPECT(false, MAJOR, "Unexpected variant: {}", oper.index());
        }
      }
    }
    return retval;
  }

  struct parse_axe_run_t {
    // parse_axe_t::apply() params

    const parse_axe_t& parse_axe;
    parse_tree_nursery_t& nursery;
    const full_name_id_t atom_name_id;
    delegate_t<expected_t<parse_tree_sub_t>()> atom;

    // internal state associated with a run

    enum class mode_t {
      ATOM_MODE,
      INFIX_MODE,
    };
    using enum mode_t;

    struct oper_item_t {
      oper_any_t oper;
      index_t arity             = 0;
      full_name_id_t level_name = 0;
      precedence_t precedence;
      optional_t<index_t> covered_token_index;
      optional_t<index_t> min_token_index;
      optional_t<index_t> max_token_index;
    };

    struct atom_item_t {
      index_t atom_tree_node_index = 0;
    };

    struct atom_data_t {
      full_name_id_t name = 0;
      bool flat_flag      = false;
      pair_t<index_t, index_t> token_range;
      optional_t<index_t> atom_child_index;
    };

    tree_inv_t<atom_data_t> atom_tree;
    using atom_tree_node_t = tree_inv_t<atom_data_t>::node_t;

    // functions

    optional_t<index_t> try_parse_atom(parse_tree_guard_for_rule_t& gg_rule)
    {
      auto maybe_atom_result = atom();
      if (!maybe_atom_result) {
        return none;
      }
      auto atom_result               = std::move(maybe_atom_result).value();
      const index_t atom_child_index = gg_rule.sub.num_children;
      gg_rule.sub += atom_result;
      const index_t atom_tree_node_index = atom_tree.nodes.size();
      atom_tree.nodes.push_back(atom_tree_node_t{
          {
              .name             = atom_name_id,
              .flat_flag        = true,
              .token_range      = {atom_result.token_begin, atom_result.token_end},
              .atom_child_index = atom_child_index,
          },
          /* num_children = */ 0,
          /* children_begin = */ index_t(atom_tree.nodes.size()),
      });
      return atom_tree_node_index;
    }

    struct stack_pair_t {
      tree_inv_t<atom_data_t>* atom_tree = nullptr;

      vector_t<oper_item_t> oper_stack;
      vector_t<atom_item_t> atom_stack;

      expected_t<pair_t<index_t, index_t>> consistent_range(optional_t<index_t> covered_token_index,
                                                            const index_t arity)
      {
        SILVA_EXPECT(arity <= atom_stack.size(), MINOR);
        const index_t stack_index_begin = atom_stack.size() - arity;
        pair_t<index_t, index_t> retval =
            atom_tree->nodes[atom_stack[stack_index_begin].atom_tree_node_index].token_range;
        if (covered_token_index.has_value() && covered_token_index.value() < retval.first) {
          SILVA_EXPECT(covered_token_index.value() + 1 == retval.first, MINOR);
          retval.first -= 1;
          covered_token_index.reset();
        }
        const auto try_cti_after = [&]() -> expected_t<void> {
          if (covered_token_index.has_value()) {
            SILVA_EXPECT(retval.second <= covered_token_index.value(), MINOR);
            if (retval.second == covered_token_index.value()) {
              retval.second += 1;
              covered_token_index.reset();
            }
          }
          return {};
        };
        SILVA_EXPECT_FWD(try_cti_after());
        for (index_t stack_index = stack_index_begin + 1; stack_index < atom_stack.size();
             ++stack_index) {
          const auto& atom_data = atom_tree->nodes[atom_stack[stack_index].atom_tree_node_index];
          SILVA_EXPECT(retval.second == atom_data.token_range.first, MINOR);
          retval.second = atom_data.token_range.second;
          SILVA_EXPECT_FWD(try_cti_after());
        }
        SILVA_EXPECT(!covered_token_index.has_value(), MINOR);
        return retval;
      }

      expected_t<void> stack_pop(precedence_t prec)
      {
        while (!oper_stack.empty() && !(oper_stack.back().precedence < prec)) {
          const auto oi = oper_stack.back();
          oper_stack.pop_back();
          const auto [token_begin, token_end] =
              SILVA_EXPECT_FWD(consistent_range(oi.covered_token_index, oi.arity));
          SILVA_EXPECT(!oi.min_token_index.has_value() || oi.min_token_index <= token_begin, MINOR);
          SILVA_EXPECT(!oi.max_token_index.has_value() || token_end <= oi.max_token_index, MINOR);
          SILVA_EXPECT(oi.arity <= atom_stack.size(), MINOR);
          const atom_tree_node_t& base_node =
              atom_tree->nodes[atom_stack[atom_stack.size() - oi.arity].atom_tree_node_index];
          bool flat_flag = false;
          if (prec.assoc == assoc_t::FLAT && base_node.flat_flag) {
            SILVA_EXPECT(false, ASSERT);
            flat_flag = true;
          }
          else {
            ;
          }
          atom_stack.resize(atom_stack.size() - oi.arity);
          atom_stack.push_back(
              atom_item_t{.atom_tree_node_index = index_t(atom_tree->nodes.size())});
          atom_tree->nodes.push_back(atom_tree_node_t{
              {
                  .name             = oi.level_name,
                  .flat_flag        = flat_flag,
                  .token_range      = {token_begin, token_end},
                  .atom_child_index = none,
              },
              /* num_children = */ oi.arity,
              /* children_begin = */ base_node.children_begin,
          });
        }
        return {};
      }
    };

    void hallucinate_concat() { SILVA_ASSERT(false); }

    expected_t<index_t> handle_nest(parse_tree_guard_for_rule_t& gg_rule,
                                    const full_name_id_t nest_name,
                                    const token_id_t left_token,
                                    const token_id_t right_token)
    {
      const index_t token_begin = nursery.token_index;
      SILVA_EXPECT(nursery.num_tokens_left() >= 1, ASSERT);
      SILVA_EXPECT(nursery.token_id_by() == left_token, ASSERT);
      nursery.token_index += 1;
      SILVA_EXPECT_FWD(go_parse(gg_rule));
      SILVA_EXPECT(nursery.num_tokens_left() >= 1, ASSERT);
      SILVA_EXPECT(nursery.token_id_by() == right_token, ASSERT);
      nursery.token_index += 1;
      SILVA_EXPECT(atom_tree.nodes.size() >= 1, ASSERT);
      const index_t token_end = nursery.token_index;
      atom_tree.nodes.push_back(atom_tree_node_t{
          {
              .name             = nest_name,
              .flat_flag        = true,
              .token_range      = {token_begin, token_end},
              .atom_child_index = none,
          },
          /* num_children = */ 1,
          /* children_begin = */ atom_tree.nodes.back().children_begin,
      });
      return atom_tree.nodes.size() - 1;
    }

    expected_t<void> go_parse(parse_tree_guard_for_rule_t& gg_rule)
    {
      stack_pair_t stack_pair{.atom_tree = &atom_tree};
      mode_t mode = ATOM_MODE;
      while (nursery.num_tokens_left() >= 1) {
        const auto it = parse_axe.results.find(nursery.token_id_by());
        if (it == parse_axe.results.end()) {
          // Current token is not one of the known operators
          if (mode == ATOM_MODE) {
            const auto maybe_atom_tree_node_index = try_parse_atom(gg_rule);
            if (!maybe_atom_tree_node_index.has_value()) {
              break;
            }
            mode = INFIX_MODE;
            stack_pair.atom_stack.push_back(atom_item_t{maybe_atom_tree_node_index.value()});
            continue;
          }
          else {
            hallucinate_concat();
            continue;
          }
        }
        else {
          const parse_axe_result_t& pa_result = it->second;
          if (pa_result.is_right_bracket) {
            break;
          }
          if (mode == INFIX_MODE && parse_axe.concat.has_value()) {
            if (pa_result.prefix.has_value() && !pa_result.regular.has_value()) {
              hallucinate_concat();
              continue;
            }
            if (pa_result.prefix.has_value() &&
                variant_holds_t<atom_nest_t>{}(pa_result.prefix.value().oper)) {
              hallucinate_concat();
              continue;
            }
          }
          if (mode == ATOM_MODE) {
            SILVA_EXPECT(pa_result.prefix.has_value(), MINOR);
            const auto& res = pa_result.prefix.value();
            SILVA_EXPECT_FWD(stack_pair.stack_pop(res.precedence));
            if (const auto* x = std::get_if<atom_nest_t>(&res.oper)) {
              const index_t atom_tree_node_index = SILVA_EXPECT_FWD(
                  handle_nest(gg_rule, res.level_name, x->left_bracket, x->right_bracket));
              stack_pair.atom_stack.push_back(atom_item_t{atom_tree_node_index});
              mode = INFIX_MODE;
              continue;
            }
            else if (const auto* x = std::get_if<prefix_t>(&res.oper)) {
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                = *x,
                  .arity               = prefix_t::arity,
                  .level_name          = res.level_name,
                  .precedence          = res.precedence,
                  .covered_token_index = nursery.token_index,
                  .min_token_index     = nursery.token_index,
              });
              nursery.token_index += 1;
              continue;
            }
            else if (const auto* x = std::get_if<prefix_nest_t>(&res.oper)) {
              SILVA_EXPECT_FWD(
                  handle_nest(gg_rule, res.level_name, x->left_bracket, x->right_bracket));
              continue;
            }
          }
          else {
            SILVA_EXPECT(pa_result.regular.has_value(), MINOR);
            const auto& res = pa_result.regular.value();
            SILVA_EXPECT_FWD(stack_pair.stack_pop(res.precedence));
            if (const auto* x = std::get_if<postfix_t>(&res.oper)) {
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                = *x,
                  .arity               = postfix_t::arity,
                  .level_name          = res.level_name,
                  .precedence          = res.precedence,
                  .covered_token_index = nursery.token_index,
                  .max_token_index     = nursery.token_index + 1,
              });
              nursery.token_index += 1;
              continue;
            }
            else if (const auto* x = std::get_if<postfix_nest_t>(&res.oper)) {
              SILVA_EXPECT_FWD(
                  handle_nest(gg_rule, res.level_name, x->left_bracket, x->right_bracket));
              continue;
            }
            else if (const auto* x = std::get_if<infix_t>(&res.oper)) {
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                = *x,
                  .arity               = infix_t::arity,
                  .level_name          = res.level_name,
                  .precedence          = res.precedence,
                  .covered_token_index = nursery.token_index,
              });
              nursery.token_index += 1;
              mode = ATOM_MODE;
              continue;
            }
            else if (const auto* x = std::get_if<ternary_t>(&res.oper)) {
              SILVA_EXPECT_FWD(handle_nest(gg_rule, res.level_name, x->first, x->second));
              continue;
            }
          }
        }
        SILVA_EXPECT(false, ASSERT);
      }
      SILVA_EXPECT_FWD(stack_pair.stack_pop(precedence_min));
      SILVA_EXPECT(stack_pair.oper_stack.empty(), MINOR);
      SILVA_EXPECT(stack_pair.atom_stack.size() == 1, MINOR);
      SILVA_EXPECT(stack_pair.atom_stack.front().atom_tree_node_index + 1 == atom_tree.nodes.size(),
                   MINOR);
      return {};
    }

    expected_t<parse_tree_sub_t>
    generate_output(const index_t atom_tree_node_index,
                    const parse_tree_t& leave_atoms_tree,
                    const vector_t<index_t>& leave_atoms_tree_child_node_indexes)
    {
      const atom_tree_node_t& node = atom_tree.nodes[atom_tree_node_index];
      auto gg_rule                 = nursery.guard_for_rule(false);
      gg_rule.set_rule_name(node.name);
      if (node.atom_child_index.has_value()) {
        const index_t aci            = node.atom_child_index.value();
        const index_t lat_node_index = leave_atoms_tree_child_node_indexes[aci];
        gg_rule.implant(leave_atoms_tree, lat_node_index);
      }
      else {
        SILVA_EXPECT_FWD(atom_tree.visit_children(
            [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
              gg_rule.sub += SILVA_EXPECT_FWD(generate_output(node_index,
                                                              leave_atoms_tree,
                                                              leave_atoms_tree_child_node_indexes));
              return true;
            },
            atom_tree_node_index));
      }
      gg_rule.sub.token_begin = node.token_range.first;
      gg_rule.sub.token_end   = node.token_range.second;
      return gg_rule.release();
    }

    expected_t<parse_tree_sub_t> go()
    {
      auto gg_rule = nursery.guard_for_rule();
      SILVA_EXPECT_FWD(go_parse(gg_rule));

      const auto& root_node          = atom_tree.nodes.back();
      const index_t expr_token_begin = root_node.token_range.first;
      const index_t expr_token_end   = root_node.token_range.second;
      SILVA_EXPECT(gg_rule.orig_token_index == expr_token_begin, MINOR);
      SILVA_EXPECT(*gg_rule.token_index == expr_token_end, MINOR);

      gg_rule.sync();
      parse_tree_t leave_atoms_tree   = nursery.retval.subtree(gg_rule.orig_node_size);
      const index_t final_token_index = nursery.token_index;
      gg_rule.reset();
      const index_t num_children = leave_atoms_tree.nodes.front().num_children;
      vector_t<index_t> leave_atoms_tree_child_node_indexes(num_children);
      SILVA_EXPECT_FWD(leave_atoms_tree.visit_children(
          [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
            leave_atoms_tree_child_node_indexes[child_index] = node_index;
            return true;
          },
          0));

      SILVA_EXPECT(atom_tree.nodes.size() >= 1, ASSERT);
      const parse_tree_sub_t result =
          SILVA_EXPECT_FWD(generate_output(atom_tree.nodes.size() - 1,
                                           leave_atoms_tree,
                                           leave_atoms_tree_child_node_indexes));
      SILVA_EXPECT(result.token_begin == expr_token_begin, ASSERT);
      SILVA_EXPECT(result.token_end == expr_token_end, ASSERT);
      nursery.token_index = final_token_index;
      return result;
    }
  };

  expected_t<parse_tree_sub_t>
  parse_axe_t::apply(parse_tree_nursery_t& nursery,
                     const full_name_id_t atom_name_id,
                     delegate_t<expected_t<parse_tree_sub_t>()> atom) const
  {
    parse_axe_run_t run{
        .parse_axe    = *this,
        .nursery      = nursery,
        .atom_name_id = atom_name_id,
        .atom         = atom,
    };
    return run.go();
  }
}
