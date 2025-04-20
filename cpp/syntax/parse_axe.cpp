#include "parse_axe.hpp"
#include "canopy/variant.hpp"
#include <ranges>
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
      if (lhs.flatten_id.has_value() && lhs.flatten_id == rhs.flatten_id) {
        return true;
      }
      // Each level has a unique associativity.
      SILVA_ASSERT(lhs.assoc == rhs.assoc);
      return lhs.assoc == assoc_t::RIGHT_TO_LEFT;
    }
  };

  expected_t<parse_axe_t> parse_axe_create(syntax_catalog_ptr_t scp,
                                           const name_id_t parse_axe_name,
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
                       "the 'nest' level {} must occur before all non-'nest' levels",
                       level_desc.pts);
          SILVA_EXPECT(variant_holds_t<atom_nest_t>{}(oper),
                       MINOR,
                       "the 'nest' level {} only allows operators of type 'atom_nest'",
                       level_desc.pts);
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
          else {
            SILVA_EXPECT(false, ASSERT, "Unknown level {}", std::to_underlying(level_desc.assoc));
          }
        }
      }
    }

    parse_axe_t retval{
        .scp  = scp,
        .name = parse_axe_name,
    };

    const auto register_op = [&retval](const token_id_t token_id,
                                       const oper_any_t oper,
                                       const name_id_t level_name,
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
            .name       = retval.scp->name_id(level_name, token_id),
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
            .name       = retval.scp->name_id(level_name, token_id),
            .precedence = precedence,
        };
      }
      else {
        SILVA_EXPECT(false, MAJOR, "Unexpected level: {}", oper.index());
      }
      return {};
    };

    const auto register_right_op = [&](const token_id_t token_id) -> expected_t<void> {
      auto& result = retval.results[token_id];
      SILVA_EXPECT(!result.prefix.has_value(),
                   MINOR,
                   "Trying to use token {} as right-bracket and prefix",
                   token_id);
      SILVA_EXPECT(!result.regular.has_value(),
                   MINOR,
                   "Trying to use token {} as right-bracket and regular",
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
        const name_id_t full_name = scp->name_id(parse_axe_name, level_desc.base_name);
        if (const auto* x = std::get_if<prefix_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->token_id, *x, full_name, precedence));
        }
        else if (const auto* x = std::get_if<prefix_nest_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->left_bracket, *x, full_name, precedence));
          SILVA_EXPECT_FWD(register_right_op(x->right_bracket));
        }
        else if (const auto* x = std::get_if<atom_nest_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->left_bracket, *x, full_name, precedence));
          SILVA_EXPECT_FWD(register_right_op(x->right_bracket));
        }
        else if (const auto* x = std::get_if<infix_t>(&oper); x) {
          precedence_t used_prec = precedence;
          if (x->flatten) {
            used_prec.flatten_id = x->token_id;
          }
          if (x->concat) {
            SILVA_EXPECT(!retval.concat_result.has_value(),
                         MINOR,
                         "Trying to use 'concat' level twice");
            retval.concat_result.emplace(result_oper_t<oper_regular_t>{
                .oper       = *x,
                .name       = retval.scp->name_id(full_name, x->token_id),
                .precedence = used_prec,
            });
          }
          else {
            SILVA_EXPECT_FWD(register_op(x->token_id, *x, full_name, used_prec));
          }
        }
        else if (const auto* x = std::get_if<ternary_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->first, *x, full_name, precedence));
          SILVA_EXPECT_FWD(register_right_op(x->second));
        }
        else if (const auto* x = std::get_if<postfix_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->token_id, *x, full_name, precedence));
        }
        else if (const auto* x = std::get_if<postfix_nest_t>(&oper); x) {
          SILVA_EXPECT_FWD(register_op(x->left_bracket, *x, full_name, precedence));
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
    token_catalog_ptr_t tcp = nursery.tcp;
    const name_id_t atom_name_id;
    delegate_t<expected_t<parse_tree_node_t>()> atom;

    token_position_t token_position_by(index_t token_index_offset = 0) const
    {
      return nursery.token_position_by(token_index_offset);
    }

    // internal state associated with a run

    enum class mode_t {
      ATOM_MODE,
      INFIX_MODE,
    };
    using enum mode_t;

    struct oper_item_t {
      oper_any_t oper;
      index_t arity        = 0;
      name_id_t level_name = 0;
      precedence_t precedence;
      small_vector_t<index_t, 2> covered_token_indexes;
      optional_t<index_t> min_token_index;
      optional_t<index_t> max_token_index;
    };

    struct atom_item_t {
      index_t atom_tree_node_index = 0;
    };

    struct atom_data_t {
      name_id_t name = 0;
      pair_t<index_t, index_t> token_range;
      optional_t<index_t> atom_child_index;
    };

    struct atom_tree_node_t
      : public tree_node_t
      , public atom_data_t {};

    vector_t<atom_tree_node_t> atom_tree;
    using atom_tree_span_t = tree_span_t<const atom_tree_node_t>;

    // functions

    expected_t<optional_t<atom_data_t>> try_parse_atom(parse_tree_nursery_t::stake_t& ss_rule)
    {
      auto maybe_atom_result = atom();
      if (!maybe_atom_result) {
        return none;
      }
      auto atom_result = std::move(maybe_atom_result).value();
      SILVA_EXPECT(atom_result.num_children == 1,
                   ASSERT,
                   "The atom function given to parse_axe_t must always parse a single child");
      const index_t atom_child_index = ss_rule.proto_node.num_children;
      const pair_t<index_t, index_t> token_range{atom_result.token_begin, atom_result.token_end};
      ss_rule.add_proto_node(atom_result);
      return {atom_data_t{
          .name             = atom_name_id,
          .token_range      = token_range,
          .atom_child_index = atom_child_index,
      }};
    }

    struct stack_pair_t {
      vector_t<atom_tree_node_t>* atom_tree = nullptr;
      vector_t<oper_item_t> oper_stack;
      vector_t<atom_item_t> atom_stack;

      struct consistent_range_t {
        index_t num_atoms          = 0;
        name_id_t joint_level_name = name_id_root;
        index_t token_begin        = 0;
        index_t token_end          = 0;
      };
      expected_t<consistent_range_t> consistent_range(span_t<const oper_item_t> ois) const
      {
        // The following probably doesn't make sense if ois.size() > 1 and the operators are of type
        // ternary_t, prefix_nest_t, or postfix_nest_t. Should probably rewrite the logic to cover
        // this case explicitly, and then handle ois.size() > 1 only for prefix_t, postfix_t, and
        // infix_t.

        SILVA_EXPECT(!ois.empty(), ASSERT);
        const index_t common_arity = ois.front().arity;
        for (index_t i = 1; i < ois.size(); ++i) {
          SILVA_EXPECT(ois.front().oper == ois[i].oper, ASSERT);
          SILVA_EXPECT(common_arity == ois[i].arity, ASSERT);
        }
        SILVA_EXPECT(common_arity >= 1, ASSERT);
        const index_t combined_arity = (common_arity - 1) * ois.size() + 1;
        SILVA_EXPECT(combined_arity <= atom_stack.size(), MINOR);
        const index_t atom_stack_begin = atom_stack.size() - combined_arity;
        const auto& front_atn = (*atom_tree)[atom_stack[atom_stack_begin].atom_tree_node_index];
        consistent_range_t retval{
            .num_atoms        = combined_arity,
            .joint_level_name = ois.front().level_name,
            .token_begin      = front_atn.token_range.first,
            .token_end        = front_atn.token_range.second,
        };

        const auto check_coverage = [&](const oper_item_t& oi) -> expected_t<void> {
          SILVA_EXPECT(!oi.min_token_index.has_value() || oi.min_token_index <= retval.token_begin,
                       MINOR);
          SILVA_EXPECT(!oi.max_token_index.has_value() || retval.token_end <= oi.max_token_index,
                       MINOR);
          return {};
        };

        index_t oi_index = 0;
        index_t cti_pos  = 0;
        while (oi_index < ois.size() && ois[oi_index].covered_token_indexes.size == 0) {
          SILVA_EXPECT_FWD(check_coverage(ois[oi_index]));
          oi_index += 1;
        }
        const auto cti_peek = [&]() -> optional_t<index_t> {
          if (oi_index < ois.size() && cti_pos < ois[oi_index].covered_token_indexes.size) {
            return ois[oi_index].covered_token_indexes[cti_pos];
          }
          else {
            return none;
          }
        };
        const auto cti_advance = [&]() -> expected_t<void> {
          cti_pos += 1;
          if (cti_pos == ois[oi_index].covered_token_indexes.size) {
            SILVA_EXPECT_FWD(check_coverage(ois[oi_index]));
            oi_index += 1;
            cti_pos = 0;
          }
          return {};
        };
        const auto try_ctis = [&]() -> expected_t<void> {
          if (const auto cti = cti_peek(); cti.has_value() && cti.value() < retval.token_begin) {
            SILVA_EXPECT(cti.value() + 1 == retval.token_begin, MINOR);
            retval.token_begin -= 1;
            SILVA_EXPECT_FWD(cti_advance());
          }
          if (const auto cti = cti_peek(); cti.has_value()) {
            SILVA_EXPECT(retval.token_end <= cti.value(), MINOR);
            if (retval.token_end == cti.value()) {
              retval.token_end += 1;
              SILVA_EXPECT_FWD(cti_advance());
            }
          }
          return {};
        };

        SILVA_EXPECT_FWD(try_ctis());
        for (index_t atom_stack_index = atom_stack_begin + 1; atom_stack_index < atom_stack.size();
             ++atom_stack_index) {
          const auto& atn = (*atom_tree)[atom_stack[atom_stack_index].atom_tree_node_index];
          SILVA_EXPECT(retval.token_end == atn.token_range.first, MINOR);
          retval.token_end = atn.token_range.second;
          SILVA_EXPECT_FWD(try_ctis());
        }

        SILVA_EXPECT(oi_index == ois.size(), MINOR);
        SILVA_EXPECT(cti_pos == 0, MINOR);

        return retval;
      }

      expected_t<void> stack_pop(precedence_t prec)
      {
        while (!oper_stack.empty() && !(oper_stack.back().precedence < prec)) {
          const index_t oper_stack_end = oper_stack.size();
          index_t oper_stack_begin     = oper_stack_end - 1;
          if (const auto* infix_op = std::get_if<infix_t>(&oper_stack[oper_stack_end - 1].oper);
              infix_op != nullptr && infix_op->flatten) {
            while (oper_stack_begin > 0 &&
                   oper_stack[oper_stack_begin - 1].oper == oper_stack[oper_stack_end - 1].oper) {
              oper_stack_begin -= 1;
            }
          }
          const span_t<const oper_item_t> ois{&oper_stack[oper_stack_begin],
                                              &oper_stack[oper_stack_end]};
          const consistent_range_t cr = SILVA_EXPECT_FWD(consistent_range(ois));
          oper_stack.resize(oper_stack.size() - ois.size());
          SILVA_EXPECT(cr.num_atoms <= atom_stack.size(), MINOR);
          index_t subtree_size = 1;
          for (index_t i = atom_stack.size() - cr.num_atoms; i < atom_stack.size(); ++i) {
            const index_t curr_subtree_size =
                (*atom_tree)[atom_stack[i].atom_tree_node_index].subtree_size;
            if (i > atom_stack.size() - cr.num_atoms) {
              SILVA_EXPECT(atom_stack[i - 1].atom_tree_node_index ==
                               atom_stack[i].atom_tree_node_index - curr_subtree_size,
                           ASSERT);
            }
            subtree_size += curr_subtree_size;
          }
          atom_stack.resize(atom_stack.size() - cr.num_atoms);
          atom_stack.push_back(atom_item_t{.atom_tree_node_index = index_t(atom_tree->size())});
          atom_tree->push_back(atom_tree_node_t{
              {
                  .num_children = cr.num_atoms,
                  .subtree_size = subtree_size,
              },
              {
                  .name             = cr.joint_level_name,
                  .token_range      = {cr.token_begin, cr.token_end},
                  .atom_child_index = none,
              },
          });
        }
        return {};
      }
    };

    expected_t<pair_t<index_t, index_t>> handle_nest(parse_tree_nursery_t::stake_t& ss_rule,
                                                     const token_id_t left_token,
                                                     const token_id_t right_token)
    {
      const index_t token_begin = nursery.token_index;
      SILVA_EXPECT(nursery.num_tokens_left() >= 1, ASSERT);
      SILVA_EXPECT(nursery.token_id_by() == left_token, ASSERT);
      nursery.token_index += 1;
      SILVA_EXPECT_FWD(go_parse(ss_rule));
      SILVA_EXPECT(nursery.num_tokens_left() >= 1, ASSERT);
      SILVA_EXPECT(nursery.token_id_by() == right_token, ASSERT);
      nursery.token_index += 1;
      SILVA_EXPECT(atom_tree.size() >= 1, ASSERT);
      const index_t token_end = nursery.token_index;
      return {{token_begin, token_end}};
    }

    expected_t<void> go_parse(parse_tree_nursery_t::stake_t& ss_rule)
    {
      stack_pair_t stack_pair{.atom_tree = &atom_tree};
      mode_t mode = ATOM_MODE;

      const auto hallucinate_concat = [&]() -> expected_t<void> {
        SILVA_EXPECT(parse_axe.concat_result.has_value(), ASSERT);
        const auto& reg = parse_axe.concat_result.value();
        SILVA_EXPECT_FWD(stack_pair.stack_pop(reg.precedence));
        stack_pair.oper_stack.push_back(oper_item_t{
            .oper       = std::get<infix_t>(reg.oper),
            .arity      = infix_t::arity,
            .level_name = reg.name,
            .precedence = reg.precedence,
        });
        mode = ATOM_MODE;
        return {};
      };

      while (nursery.num_tokens_left() >= 1) {
        const auto it = parse_axe.results.find(nursery.token_id_by());
        if (it == parse_axe.results.end()) {
          if (mode == INFIX_MODE && !parse_axe.concat_result.has_value()) {
            break;
          }
          // Current token is not one of the known operators, so it has to be an atom or the end of
          // the expression
          const optional_t<atom_data_t> atom_data = SILVA_EXPECT_FWD(try_parse_atom(ss_rule));
          if (!atom_data.has_value()) {
            break;
          }
          if (mode == INFIX_MODE && parse_axe.concat_result.has_value()) {
            SILVA_EXPECT_FWD(hallucinate_concat());
          }
          if (mode == ATOM_MODE) {
            const index_t atom_tree_node_index = atom_tree.size();
            atom_tree.push_back(atom_tree_node_t{
                {
                    .num_children = 0,
                    .subtree_size = 1,
                },
                atom_data.value(),
            });
            stack_pair.atom_stack.push_back(atom_item_t{atom_tree_node_index});
            mode = INFIX_MODE;
            continue;
          }
        }
        else {
          const parse_axe_result_t& pa_result = it->second;
          if (pa_result.is_right_bracket) {
            break;
          }
          if (mode == INFIX_MODE && parse_axe.concat_result.has_value()) {
            if (pa_result.prefix.has_value() && !pa_result.regular.has_value()) {
              SILVA_EXPECT_FWD(hallucinate_concat());
              continue;
            }
            if (pa_result.prefix.has_value() &&
                variant_holds_t<atom_nest_t>{}(pa_result.prefix.value().oper)) {
              SILVA_EXPECT_FWD(hallucinate_concat());
              continue;
            }
          }
          if (mode == ATOM_MODE) {
            SILVA_EXPECT_PARSE(parse_axe.name,
                               pa_result.prefix.has_value(),
                               "found non-prefix operator {} when expecting next atom",
                               tcp->token_id_wrap(nursery.token_id_by()));
            const auto& res = pa_result.prefix.value();
            SILVA_EXPECT_FWD(stack_pair.stack_pop(res.precedence));

            if (const auto* x = std::get_if<atom_nest_t>(&res.oper)) {
              const auto [token_begin, token_end] =
                  SILVA_EXPECT_FWD(handle_nest(ss_rule, x->left_bracket, x->right_bracket));
              atom_tree.push_back(atom_tree_node_t{
                  {
                      .num_children = 1,
                      .subtree_size = atom_tree.back().subtree_size + 1,
                  },
                  {
                      .name             = res.name,
                      .token_range      = {token_begin, token_end},
                      .atom_child_index = none,
                  },
              });
              stack_pair.atom_stack.push_back(atom_item_t{index_t(atom_tree.size() - 1)});
              mode = INFIX_MODE;
              continue;
            }
            else if (const auto* x = std::get_if<prefix_t>(&res.oper)) {
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                  = *x,
                  .arity                 = prefix_t::arity,
                  .level_name            = res.name,
                  .precedence            = res.precedence,
                  .covered_token_indexes = {nursery.token_index},
                  .min_token_index       = nursery.token_index,
              });
              nursery.token_index += 1;
              continue;
            }
            else if (const auto* x = std::get_if<prefix_nest_t>(&res.oper)) {
              const auto [token_begin, token_end] =
                  SILVA_EXPECT_FWD(handle_nest(ss_rule, x->left_bracket, x->right_bracket));
              stack_pair.atom_stack.push_back(atom_item_t{index_t(atom_tree.size() - 1)});
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                  = *x,
                  .arity                 = prefix_nest_t::arity,
                  .level_name            = res.name,
                  .precedence            = res.precedence,
                  .covered_token_indexes = {token_begin, token_end - 1},
                  .min_token_index       = token_begin,
              });
              continue;
            }
          }
          else {
            SILVA_EXPECT(pa_result.regular.has_value(), MINOR);
            const auto& res = pa_result.regular.value();
            SILVA_EXPECT_FWD(stack_pair.stack_pop(res.precedence));

            if (const auto* x = std::get_if<postfix_t>(&res.oper)) {
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                  = *x,
                  .arity                 = postfix_t::arity,
                  .level_name            = res.name,
                  .precedence            = res.precedence,
                  .covered_token_indexes = {nursery.token_index},
                  .max_token_index       = nursery.token_index + 1,
              });
              nursery.token_index += 1;
              continue;
            }
            else if (const auto* x = std::get_if<postfix_nest_t>(&res.oper)) {
              const auto [token_begin, token_end] =
                  SILVA_EXPECT_FWD(handle_nest(ss_rule, x->left_bracket, x->right_bracket));
              stack_pair.atom_stack.push_back(atom_item_t{index_t(atom_tree.size() - 1)});
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                  = *x,
                  .arity                 = postfix_nest_t::arity,
                  .level_name            = res.name,
                  .precedence            = res.precedence,
                  .covered_token_indexes = {token_begin, token_end - 1},
                  .max_token_index       = nursery.token_index,
              });
              continue;
            }
            else if (const auto* x = std::get_if<infix_t>(&res.oper)) {
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                  = *x,
                  .arity                 = infix_t::arity,
                  .level_name            = res.name,
                  .precedence            = res.precedence,
                  .covered_token_indexes = {nursery.token_index},
              });
              nursery.token_index += 1;
              mode = ATOM_MODE;
              continue;
            }
            else if (const auto* x = std::get_if<ternary_t>(&res.oper)) {
              const auto [token_begin, token_end] =
                  SILVA_EXPECT_FWD(handle_nest(ss_rule, x->first, x->second));
              stack_pair.atom_stack.push_back(atom_item_t{index_t(atom_tree.size() - 1)});
              stack_pair.oper_stack.push_back(oper_item_t{
                  .oper                  = *x,
                  .arity                 = ternary_t::arity,
                  .level_name            = res.name,
                  .precedence            = res.precedence,
                  .covered_token_indexes = {token_begin, token_end - 1},
              });
              mode = ATOM_MODE;
              continue;
            }
          }
        }
        SILVA_EXPECT(false, ASSERT);
      }
      SILVA_EXPECT_FWD(stack_pair.stack_pop(precedence_min));
      SILVA_EXPECT(stack_pair.oper_stack.empty(), MINOR);
      SILVA_EXPECT_PARSE(parse_axe.name, stack_pair.atom_stack.size() > 0, "empty expression");
      SILVA_EXPECT(stack_pair.atom_stack.size() == 1, MINOR);
      SILVA_EXPECT(stack_pair.atom_stack.front().atom_tree_node_index + 1 == atom_tree.size(),
                   MINOR);
      return {};
    }

    index_t generate_output(const atom_tree_span_t ats,
                            const parse_tree_t& leave_atoms_tree,
                            const vector_t<index_t>& leave_atoms_tree_child_node_indexes)
    {
      auto& rv_nodes       = nursery.tree;
      const index_t retval = rv_nodes.size();
      const auto& node     = ats[0];
      if (node.atom_child_index.has_value()) {
        const index_t aci            = node.atom_child_index.value();
        const index_t lat_node_index = leave_atoms_tree_child_node_indexes[aci];
        const auto to_implant        = leave_atoms_tree.span().sub_tree_span_at(lat_node_index);
        rv_nodes.insert(rv_nodes.end(), to_implant.root, to_implant.root + to_implant.size());
      }
      else {
        rv_nodes.emplace_back();
        rv_nodes[retval].rule_name    = node.name;
        rv_nodes[retval].num_children = ats.root->num_children;
        rv_nodes[retval].subtree_size = 1;
        const auto child_node_indexes = ats.get_children_dyn();
        for (const index_t node_index: std::ranges::reverse_view(child_node_indexes)) {
          const index_t sub_node_index = generate_output(ats.sub_tree_span_at(node_index),
                                                         leave_atoms_tree,
                                                         leave_atoms_tree_child_node_indexes);
          rv_nodes[retval].subtree_size += rv_nodes[sub_node_index].subtree_size;
        }
        rv_nodes[retval].token_begin = node.token_range.first;
        rv_nodes[retval].token_end   = node.token_range.second;
      }
      return retval;
    }

    expected_t<index_t> go()
    {
      auto ss      = nursery.stake();
      auto ss_rule = nursery.stake();
      ss_rule.create_node(name_id_root);
      SILVA_EXPECT_FWD(go_parse(ss_rule));

      const auto& root_node          = atom_tree.back();
      const index_t expr_token_begin = root_node.token_range.first;
      const index_t expr_token_end   = root_node.token_range.second;
      SILVA_EXPECT(ss_rule.orig_state.token_index == expr_token_begin, MINOR);
      SILVA_EXPECT(nursery.token_index == expr_token_end, MINOR);

      ss_rule.commit();
      parse_tree_span_t leave_atoms_tree_span{nursery.tree.data(), 1, nursery.tp};
      parse_tree_t leave_atoms_tree =
          leave_atoms_tree_span.sub_tree_span_at(ss.orig_state.tree_size).copy();
      const index_t final_token_index = nursery.token_index;
      ss.clear();
      const index_t num_children = leave_atoms_tree.nodes.front().num_children;
      vector_t<index_t> leave_atoms_tree_child_node_indexes(num_children);
      for (const auto [node_index, child_index]: leave_atoms_tree.span().children_range()) {
        leave_atoms_tree_child_node_indexes[child_index] = node_index;
      }

      SILVA_EXPECT(atom_tree.size() >= 1, ASSERT);
      atom_tree_span_t ats{&atom_tree.back(), -1};
      const index_t retval =
          generate_output(ats, leave_atoms_tree, leave_atoms_tree_child_node_indexes);
      nursery.token_index = final_token_index;
      return {retval};
    }
  };

  expected_t<parse_tree_node_t>
  parse_axe_t::apply(parse_tree_nursery_t& nursery,
                     const name_id_t atom_name_id,
                     delegate_t<expected_t<parse_tree_node_t>()> atom) const
  {
    parse_axe_run_t run{
        .parse_axe    = *this,
        .nursery      = nursery,
        .atom_name_id = atom_name_id,
        .atom         = atom,
    };
    const index_t created_node = SILVA_EXPECT_FWD(run.go());
    auto& rv_nodes             = nursery.tree;
    parse_tree_node_t retval;
    retval.num_children = 1;
    retval.subtree_size = rv_nodes[created_node].subtree_size;
    retval.token_begin  = rv_nodes[created_node].token_begin;
    retval.token_end    = rv_nodes[created_node].token_end;
    return retval;
  }
}
