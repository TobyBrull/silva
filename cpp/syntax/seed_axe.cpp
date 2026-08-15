#include "seed_axe.hpp"

#include "canopy/variant.hpp"

#include <ranges>
#include <variant>

namespace silva::seed::impl {
  using enum assoc_t;

  constexpr static inline precedence_t precedence_max{
      .level_index = std::numeric_limits<level_index_t>::max(),
      .assoc       = INVALID,
  };
  constexpr static inline precedence_t precedence_min{
      .level_index = std::numeric_limits<level_index_t>::min(),
      .assoc       = INVALID,
  };

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

  struct axe_create_nursery_t {
    syntax_farm_ptr_t sfp;
    name_id_t axe_name;
    const lexicon_t& lexicon = sfp->get_lexicon<lexicon_t>();

    axe_t retval{
        .lp   = sfp->get_lexicon<lexicon_t>().ptr(),
        .name = axe_name,
    };

    axe_create_nursery_t(syntax_farm_ptr_t sfp, const name_id_t axe_name)
      : sfp(sfp), axe_name(axe_name)
    {
    }

    expected_t<void> register_op(const token_id_t token_id,
                                 const oper_any_t oper,
                                 const name_id_t level_name,
                                 const precedence_t precedence,
                                 const parse_tree_span_t& pts)
    {
      auto& result = retval.results[token_id];

      if (variant_holds<oper_prefix_t>(oper)) {
        SILVA_EXPECT(!result.prefix.has_value(),
                     MINOR,
                     "{}: {} has been used as prefix-style operator before at {}",
                     pts,
                     sfp->token_id_wrap(token_id),
                     result.prefix.value().pts);
        SILVA_EXPECT(!result.is_right_bracket,
                     MINOR,
                     "{}: {} has been used as right-bracket operator before",
                     pts,
                     sfp->token_id_wrap(token_id));
        result.prefix = result_oper_t<oper_prefix_t>{
            .oper       = variant_get<oper_prefix_t>(oper),
            .name       = sfp->name_id(level_name, token_id),
            .precedence = precedence,
            .pts        = pts,
        };
      }
      else if (variant_holds<oper_regular_t>(oper)) {
        SILVA_EXPECT(!result.regular.has_value(),
                     MINOR,
                     "{}: {} has been used as regular-style operator before at {}",
                     pts,
                     sfp->token_id_wrap(token_id),
                     result.regular.value().pts);
        SILVA_EXPECT(!result.is_right_bracket,
                     MINOR,
                     "{}: {} has been used as right-bracket operator before",
                     pts,
                     sfp->token_id_wrap(token_id));
        result.regular = result_oper_t<oper_regular_t>{
            .oper       = variant_get<oper_regular_t>(oper),
            .name       = sfp->name_id(level_name, token_id),
            .precedence = precedence,
            .pts        = pts,
        };
      }
      else {
        SILVA_EXPECT(false, ASSERT);
      }
      return {};
    }

    expected_t<void> register_right_op(const token_id_t token_id, const parse_tree_span_t& pts)
    {
      auto& result = retval.results[token_id];
      SILVA_EXPECT(!result.prefix.has_value(),
                   MINOR,
                   "{}: {} has been used as prefix-style operator before at {}",
                   pts,
                   sfp->token_id_wrap(token_id),
                   result.prefix.value().pts);
      SILVA_EXPECT(!result.regular.has_value(),
                   MINOR,
                   "{}: {} has been used as right-bracket operator before at {}",
                   pts,
                   sfp->token_id_wrap(token_id),
                   result.regular.value().pts);
      result.is_right_bracket = true;
      return {};
    }

    expected_t<void> op(const token_id_t axe_op_type, const parse_tree_span_t pts_op)
    {
      const auto op_tok = SILVA_EXPECT_FWD(pts_op.token());
      SILVA_EXPECT(pts_op[0].rule_name == lexicon.ni_axe_op, BROKEN_SEED);
      const auto sub_ptses = SILVA_EXPECT_FWD(pts_op.get_children_up_to_pts<1>());
      if (sub_ptses.size == 0) {
        SILVA_EXPECT(op_tok == lexicon.ti_concat.token_id, BROKEN_SEED);
      }
      else {
        SILVA_EXPECT(sub_ptses.size == 1, BROKEN_SEED);
        const parse_tree_span_t sub_pts = sub_ptses[0];
        SILVA_EXPECT(sub_pts[0].rule_name == lexicon.ni_string, BROKEN_SEED);
      }
      if (op_tok == lexicon.ti_concat.token_id) {
        SILVA_EXPECT(
            axe_op_type == lexicon.ti_infix.token_id ||
                axe_op_type == lexicon.ti_infix_flat.token_id,
            MINOR,
            "{} the 'concat' token may only be used with 'infix' or 'infix_flat' operations.",
            pts_op);
      }
      return {};
    }

    expected_t<void> ops(const level_index_t level_index,
                         const name_id_t full_name,
                         const assoc_t assoc,
                         const parse_tree_span_t pts_ops)
    {
      SILVA_EXPECT(pts_ops[0].rule_name == lexicon.ni_axe_ops, BROKEN_SEED);
      auto [it, end] = pts_ops.children_range();
      SILVA_EXPECT(it != end, BROKEN_SEED);
      const auto pts_op_type = pts_ops.sub_tree_span_at(it.pos);
      SILVA_EXPECT(pts_op_type[0].rule_name == lexicon.ni_axe_op_type, BROKEN_SEED);
      const token_id_t axe_op_type = SILVA_EXPECT_FWD(pts_op_type.token());
      SILVA_EXPECT(axe_op_type == lexicon.ti_prefix.token_id ||
                       axe_op_type == lexicon.ti_prefix_n.token_id ||
                       axe_op_type == lexicon.ti_infix.token_id ||
                       axe_op_type == lexicon.ti_infix_flat.token_id ||
                       axe_op_type == lexicon.ti_ternary.token_id ||
                       axe_op_type == lexicon.ti_postfix.token_id ||
                       axe_op_type == lexicon.ti_postfix_n.token_id,
                   BROKEN_SEED);
      ++it;

      optional_t<index_t> nest_rule_name;
      optional_t<parse_tree_span_t> pts_nt;
      if (it != end) {
        const auto curr_pts = pts_ops.sub_tree_span_at(it.pos);
        if (curr_pts[0].rule_name == lexicon.ni_nt) {
          pts_nt = curr_pts;
          ++it;
        }
      }

      if (axe_op_type == lexicon.ti_prefix_n.token_id ||
          axe_op_type == lexicon.ti_ternary.token_id ||
          axe_op_type == lexicon.ti_postfix_n.token_id) {
        const index_t num_op_tokens = pts_ops[0].num_children - it.child_index;
        SILVA_EXPECT(num_op_tokens % 2 == 0,
                     MINOR,
                     "{} for operations [ atom_nest atom_nest_transparent prefix_nest ternary "
                     "postfix_nest ] an even number of operators is expected",
                     pts_ops);
      }
      else {
        SILVA_EXPECT(!nest_rule_name.has_value(),
                     MINOR,
                     "{} nested rule is only allowed on operation types [ atom_nest "
                     "atom_nest_transparent prefix_nest ternary postfix_nest ]",
                     pts_ops);
      }

      if (assoc == LEFT_TO_RIGHT) {
        SILVA_EXPECT(axe_op_type == lexicon.ti_postfix.token_id ||
                         axe_op_type == lexicon.ti_postfix_n.token_id ||
                         axe_op_type == lexicon.ti_infix.token_id ||
                         axe_op_type == lexicon.ti_infix_flat.token_id ||
                         axe_op_type == lexicon.ti_ternary.token_id,
                     MINOR,
                     "{} an 'ltr' level requires operators of type [ postfix postfix_nest_t "
                     "infix_t ternary_t ], not {}",
                     pts_ops,
                     sfp->token_id_wrap(axe_op_type));
      }
      else if (assoc == RIGHT_TO_LEFT) {
        SILVA_EXPECT(axe_op_type == lexicon.ti_prefix.token_id ||
                         axe_op_type == lexicon.ti_prefix_n.token_id ||
                         axe_op_type == lexicon.ti_infix.token_id ||
                         axe_op_type == lexicon.ti_infix_flat.token_id ||
                         axe_op_type == lexicon.ti_ternary.token_id,
                     MINOR,
                     "{} 'rtl' levels require operators of type [ prefix_t prefix_nest_t "
                     "infix_t ternary_t ], not {}",
                     pts_ops,
                     sfp->token_id_wrap(axe_op_type));
      }
      else {
        SILVA_EXPECT(false, BROKEN_SEED);
      }

      const precedence_t precedence{
          .level_index = level_index,
          .assoc       = assoc,
      };

      const auto& get_next_not_concat =
          [&]() -> expected_t<tuple_t<token_id_t, parse_tree_span_t>> {
        SILVA_EXPECT(it != end, ASSERT);
        const auto pts_op = pts_ops.sub_tree_span_at(it.pos);
        SILVA_EXPECT_FWD(op(axe_op_type, pts_op));
        const token_id_t ti = SILVA_EXPECT_FWD(pts_op.token());
        ++it;
        SILVA_EXPECT(ti != lexicon.ti_concat.token_id, ASSERT);
        const token_id_t retval = SILVA_EXPECT_FWD(sfp->token_id_in_string(ti));
        return {{retval, pts_op}};
      };

      const auto& get_next_or_concat =
          [&]() -> expected_t<tuple_t<optional_t<token_id_t>, parse_tree_span_t>> {
        SILVA_EXPECT(it != end, ASSERT);
        const auto pts_op = pts_ops.sub_tree_span_at(it.pos);
        SILVA_EXPECT_FWD(op(axe_op_type, pts_op));
        const token_id_t ti = SILVA_EXPECT_FWD(pts_op.token());
        ++it;
        if (ti == lexicon.ti_concat.token_id) {
          return {{std::nullopt, pts_op}};
        }
        const token_id_t retval = SILVA_EXPECT_FWD(sfp->token_id_in_string(ti));
        return {{retval, pts_op}};
      };

      // Conceptually, it would make more sense to lower the "while" loop into each of the "if-else"
      // conditions, but the code is a bit shorter this way.
      while (it != end) {
        if (axe_op_type == lexicon.ti_prefix.token_id) {
          const auto [ti_op, pts_op] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_op,
                                       prefix_t{
                                           .token_id = ti_op,
                                       },
                                       full_name,
                                       precedence,
                                       pts_op));
        }
        else if (axe_op_type == lexicon.ti_prefix_n.token_id) {
          const auto [ti_left, pts_left]   = SILVA_EXPECT_FWD(get_next_not_concat());
          const auto [ti_right, pts_right] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_left,
                                       prefix_nest_t{
                                           .left_bracket   = ti_left,
                                           .right_bracket  = ti_right,
                                           .nest_rule_name = pts_nt,
                                       },
                                       full_name,
                                       precedence,
                                       pts_left));
          SILVA_EXPECT_FWD(register_right_op(ti_right, pts_right));
        }
        else if (axe_op_type == lexicon.ti_infix.token_id ||
                 axe_op_type == lexicon.ti_infix_flat.token_id) {
          const auto [maybe_ti_op, pts_op] = SILVA_EXPECT_FWD(get_next_or_concat());
          const bool is_flatten            = (axe_op_type == lexicon.ti_infix_flat.token_id);
          const bool is_concat             = !maybe_ti_op.has_value();
          precedence_t used_prec           = precedence;
          const token_id_t ti_op           = maybe_ti_op.value_or(lexicon.ti_concat.token_id);
          const infix_t op{
              .token_id = ti_op,
              .concat   = is_concat,
              .flatten  = is_flatten,
          };
          if (is_flatten) {
            used_prec.flatten_id = ti_op;
          }
          if (is_concat) {
            SILVA_EXPECT(!retval.concat_result.has_value(),
                         MINOR,
                         "{} the 'concat' operator may only be used once per seed-axe, was used "
                         "before at {}",
                         pts_op,
                         retval.concat_result.value().pts);
            retval.concat_result.emplace(result_oper_t<oper_regular_t>{
                .oper       = op,
                .name       = sfp->name_id(full_name, ti_op),
                .precedence = used_prec,
                .pts        = pts_op,
            });
          }
          else {
            SILVA_EXPECT_FWD(register_op(ti_op, op, full_name, used_prec, pts_op));
          }
        }
        else if (axe_op_type == lexicon.ti_ternary.token_id) {
          const auto [ti_first, pts_first]   = SILVA_EXPECT_FWD(get_next_not_concat());
          const auto [ti_second, pts_second] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_first,
                                       ternary_t{
                                           .first          = ti_first,
                                           .second         = ti_second,
                                           .nest_rule_name = pts_nt,
                                       },
                                       full_name,
                                       precedence,
                                       pts_first));
          SILVA_EXPECT_FWD(register_right_op(ti_second, pts_second));
        }
        else if (axe_op_type == lexicon.ti_postfix.token_id) {
          const auto [ti_op, pts_op] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_op,
                                       postfix_t{
                                           .token_id = ti_op,
                                       },
                                       full_name,
                                       precedence,
                                       pts_op));
        }
        else if (axe_op_type == lexicon.ti_postfix_n.token_id) {
          const auto [ti_left, pts_left]   = SILVA_EXPECT_FWD(get_next_not_concat());
          const auto [ti_right, pts_right] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_left,
                                       postfix_nest_t{
                                           .left_bracket   = ti_left,
                                           .right_bracket  = ti_right,
                                           .nest_rule_name = pts_nt,
                                       },
                                       full_name,
                                       precedence,
                                       pts_left));
          SILVA_EXPECT_FWD(register_right_op(ti_right, pts_right));
        }
        else {
          SILVA_EXPECT(false, MAJOR, "Unexpected variant: {}", sfp->token_id_wrap(axe_op_type));
        }
      }
      return {};
    }

    expected_t<void> add_to_level_map(const name_id_t full_name, const level_index_t level_index)
    {
      const auto [it, inserted] = retval.level_map.emplace(full_name, level_index);
      SILVA_EXPECT(inserted, MINOR, "duplicate level name {}", lexicon.name_id_wrap(full_name));
      return {};
    }

    expected_t<void> level(const level_index_t level_index, const parse_tree_span_t pts_level)
    {
      SILVA_EXPECT(pts_level[0].rule_name == lexicon.ni_axe_level, BROKEN_SEED);
      const auto pts_children = pts_level.get_children_dyn_pts();
      SILVA_EXPECT(pts_children.size() >= 2, BROKEN_SEED);
      const token_id_t base_name = SILVA_EXPECT_FWD(pts_children[0].token());
      const name_id_t full_name  = sfp->name_id(axe_name, base_name);
      SILVA_EXPECT_FWD(add_to_level_map(full_name, level_index));

      assoc_t assoc = INVALID;
      SILVA_EXPECT(pts_children[1][0].rule_name == lexicon.ni_axe_assoc, BROKEN_SEED);
      const token_id_t ti_assoc = SILVA_EXPECT_FWD(pts_children[1].token());
      if (ti_assoc == lexicon.ti_ltr.token_id) {
        assoc = LEFT_TO_RIGHT;
      }
      else if (ti_assoc == lexicon.ti_rtl.token_id) {
        assoc = RIGHT_TO_LEFT;
      }
      else {
        SILVA_EXPECT(false, BROKEN_SEED);
      }

      for (index_t i = 2; i < pts_children.size(); ++i) {
        SILVA_EXPECT_FWD(ops(level_index, full_name, assoc, pts_children[i]));
      }
      return {};
    }

    expected_t<void> run(const parse_tree_span_t pts_axe)
    {
      auto [it, end] = pts_axe.children_range();
      SILVA_EXPECT(it != end, MINOR, "{} should have at least one child", pts_axe);
      const auto pts_axe_atom_nt = pts_axe.sub_tree_span_at(it.pos);
      SILVA_EXPECT(pts_axe_atom_nt[0].rule_name == lexicon.ni_nt, MINOR);
      retval.atom_rule = pts_axe_atom_nt;
      ++it;

      SILVA_EXPECT(it != end, MINOR, "{} should have at least two children", pts_axe);
      const auto pts_axe_oper_nt = pts_axe.sub_tree_span_at(it.pos);
      SILVA_EXPECT(pts_axe_oper_nt[0].rule_name == lexicon.ni_nt, MINOR);
      retval.oper_rule = pts_axe_oper_nt;
      ++it;

      level_index_t curr_level = pts_axe[0].num_children - 1;
      while (it != end) {
        curr_level -= 1;
        SILVA_EXPECT_FWD(level(curr_level, pts_axe.sub_tree_span_at(it.pos)));
        ++it;
      }
      SILVA_EXPECT_FWD(add_to_level_map(axe_name, curr_level));
      SILVA_EXPECT(curr_level == 1, ASSERT);
      return {};
    }
  };
}

namespace silva::seed {
  expected_t<axe_t>
  axe_create(syntax_farm_ptr_t sfp, const name_id_t axe_name, const parse_tree_span_t pts)
  {
    impl::axe_create_nursery_t nursery(sfp, axe_name);
    SILVA_EXPECT_FWD(nursery.run(pts));
    return std::move(nursery.retval);
  }
}

namespace silva::seed::impl {
  struct axe_run_t {
    const axe_t& axe;
    parse_tree_nursery_t& nursery;
    const lexicon_t& lexicon = *axe.lp;
    delegate_t<expected_t<parse_tree_node_t>(name_id_t)> rule_parser;
    level_index_t min_prec_level = {};

    syntax_farm_ptr_t sfp = nursery.sfp;

    fragment_location_t fragment_location_by(index_t fragment_index_offset = 0) const
    {
      return nursery.fragment_location_by(fragment_index_offset);
    }

    // internal state associated with a run

    enum class mode_t {
      // next token expected to be an atom
      ATOM_MODE,

      // next token expected to be an infix operator
      INFIX_MODE,
    };
    using enum mode_t;
    mode_t mode = ATOM_MODE;

    struct oper_item_t {
      oper_any_t oper;
      index_t arity = 0;
      name_id_t level_name;
      precedence_t precedence;

      struct symbol_t {
        index_t fragment_begin = 0;
        index_t fragment_end   = 0;
        struct related_atom_t {
          enum way_t {
            NONE = 0,
            LEFTWARD,
            RIGHTWARD,
          };
          way_t way        = NONE;
          index_t atom_idx = 0;
        };
        array_small_t<related_atom_t, 2> related_atoms;
      };
      array_small_t<symbol_t, 2> symbols;

      // index into the "output_tree" array
      array_small_t<index_t, 2> oper_output_indexes;
    };
    array_t<oper_item_t> oper_stack;

    using enum oper_item_t::symbol_t::related_atom_t::way_t;

    // Indexes into the "output_tree" vector.
    array_t<index_t> open_term_stack;

    struct term_node_t : public parse_tree_node_t {
      // If the term is a subtree that was parsed via the "rule_parser", this is an index into
      // "nursery.tree" (and later, during "generate_output()", an index into the temporary vector_t
      // "leaf_terms_tree"). Otherwise, i.e., if the term is an expression derived from an operator,
      // this value is "none".
      optional_t<index_t> tree_index;
    };
    // This is the leaves-down tree that gets constructed inside the "shunting_yard()" function.
    // Eventually, this tree is turned into the final root-up tree via the "generate_output()"
    // function.
    array_t<term_node_t> output_tree;

    // functions

    struct rule_parser_result_t {
      token_id_t token_id;
      parse_tree_node_t ptn;
      term_node_t tn;
    };
    expected_t<rule_parser_result_t> invoke_rule_parser_oper()
    {
      auto retval = SILVA_EXPECT_FWD(invoke_rule_parser(axe.oper_rule));
      SILVA_EXPECT(retval.tn.tree_index.has_value(), MAJOR);
      const parse_tree_node_t& oper_node = nursery.tree[retval.tn.tree_index.value()];
      retval.token_id =
          fragment_span_t{
              nursery.fp,
              oper_node.fragment_begin,
              oper_node.fragment_end,
          }
              .derive_token_id();
      return retval;
    }
    expected_t<rule_parser_result_t> invoke_rule_parser(const name_id_t rule_name)
    {
      SILVA_EXPECT(rule_name.is_valid(), MAJOR, "trying to invoke empty rule");
      auto ss = nursery.stake();
      ss.add_proto_node(SILVA_EXPECT_FWD(rule_parser(rule_name)));
      SILVA_EXPECT(
          ss.proto_node.num_children == 1,
          MAJOR,
          "The 'atom' or 'oper' function given to seed::axe_t must always parse a single child");
      const index_t tree_index = ss.orig_state.tree_size;
      parse_tree_node_t ptn    = ss.commit();
      term_node_t tn{ptn, tree_index};
      tn.num_children = 0;
      tn.subtree_size = 1;
      return rule_parser_result_t{
          .token_id = {},
          .ptn      = ptn,
          .tn       = tn,
      };
    }

    struct nest_result_t {
      parse_tree_node_t ptn;
      array_small_t<index_t, 2> oper_output_indexes;
      rule_parser_result_t right_res;
    };

    // The left-bracket/first operator token has already been produced by the caller. This function
    // then parses the nested expression and the matching right token.
    expected_t<nest_result_t> handle_nest(const term_node_t& left_tn,
                                          const token_id_t right_token,
                                          const optional_t<name_id_ref_t>& nest_rule)
    {
      auto ss = nursery.stake();

      const name_id_t used_rule_name = nest_rule ? *nest_rule : axe.name;

      const auto [_, ptn, tn] = SILVA_EXPECT_FWD(invoke_rule_parser(used_rule_name));
      ss.add_proto_node(ptn);

      const auto right = SILVA_EXPECT_FWD(invoke_rule_parser_oper());
      SILVA_EXPECT_PARSE(used_rule_name,
                         right.token_id == right_token,
                         "expected {}, got {}",
                         sfp->token_id_wrap(right_token),
                         sfp->token_id_wrap(right.token_id));
      ss.add_proto_node(right.ptn);

      const index_t left_out_idx = output_tree.size();
      output_tree.push_back(left_tn);

      open_term_stack.push_back(output_tree.size());
      output_tree.push_back(tn);

      const index_t right_out_idx = output_tree.size();
      output_tree.push_back(right.tn);

      nest_result_t retval{
          .ptn = ss.commit(),
      };
      retval.oper_output_indexes.emplace_back(left_out_idx);
      retval.oper_output_indexes.emplace_back(right_out_idx);
      retval.right_res = right;
      return retval;
    }

    struct consistent_range_t {
      index_t num_atoms = 0;
      name_id_t joint_level_name;
      index_t fragment_begin = 0;
      index_t fragment_end   = 0;
    };
    expected_t<consistent_range_t> consistent_range(span_t<const oper_item_t> ois) const
    {
      SILVA_EXPECT(!ois.empty(), ASSERT);
      const index_t common_arity = ois.front().arity;
      for (index_t i = 1; i < ois.size(); ++i) {
        SILVA_EXPECT(ois.front().oper == ois[i].oper, ASSERT);
        SILVA_EXPECT(common_arity == ois[i].arity, ASSERT);
      }

      SILVA_EXPECT(common_arity >= 1, ASSERT);
      const index_t combined_arity = (common_arity - 1) * ois.size() + 1;
      SILVA_EXPECT(combined_arity <= open_term_stack.size(),
                   MINOR,
                   "[{}] Operator(s) expected at total of {} operands, but only found {}",
                   nursery.fragment_location_by(),
                   combined_arity,
                   open_term_stack.size());

      const index_t ots_front     = open_term_stack.size() - combined_arity;
      const term_node_t& front_tn = output_tree[open_term_stack[ots_front]];
      consistent_range_t retval{
          .num_atoms        = combined_arity,
          .joint_level_name = ois.front().level_name,
          .fragment_begin   = front_tn.fragment_begin,
          .fragment_end     = front_tn.fragment_end,
      };
      for (index_t idx = ots_front + 1; idx < open_term_stack.size(); ++idx) {
        const term_node_t& tn = output_tree[open_term_stack[idx]];
        retval.fragment_begin = std::min(retval.fragment_begin, tn.fragment_begin);
        retval.fragment_end   = std::max(retval.fragment_end, tn.fragment_end);
      }

      const auto handle_symbol = [&](const index_t atom_offset,
                                     const oper_item_t::symbol_t& symbol) -> expected_t<void> {
        for (const auto& ra: symbol.related_atoms) {
          const term_node_t& tn =
              output_tree[open_term_stack[ots_front + atom_offset + ra.atom_idx]];
          if (ra.way == LEFTWARD) {
            SILVA_EXPECT(
                tn.fragment_end <= symbol.fragment_begin,
                MINOR,
                "LEFTWARD condition tn.fragment_end={} <= symbol.fragment_begin={} violated",
                tn.fragment_end,
                symbol.fragment_begin);
          }
          else if (ra.way == RIGHTWARD) {
            SILVA_EXPECT(
                symbol.fragment_end <= tn.fragment_begin,
                MINOR,
                "RIGHTWARD condition symbol.fragment_end={} <= tn.fragment_begin={} violated",
                symbol.fragment_end,
                tn.fragment_begin);
          }
          else {
            SILVA_EXPECT(false, ASSERT);
          }
        }
        retval.fragment_begin = std::min(retval.fragment_begin, symbol.fragment_begin);
        retval.fragment_end   = std::max(retval.fragment_end, symbol.fragment_end);
        return {};
      };

      if (ois.size() == 1) {
        const oper_item_t& oi = ois.front();
        SILVA_EXPECT(oi.arity == combined_arity, ASSERT);
        for (const auto& symbol: oi.symbols) {
          SILVA_EXPECT_FWD(handle_symbol(0, symbol));
        }
        return retval;
      }
      else {
        SILVA_EXPECT(common_arity == 2, MINOR, "only infix operator can be flat");
        for (index_t oi_idx = 0; oi_idx < ois.size(); ++oi_idx) {
          for (const auto& symbol: ois[oi_idx].symbols) {
            SILVA_EXPECT_FWD(handle_symbol(oi_idx, symbol));
          }
        }
      }

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
                                            static_cast<size_t>(oper_stack_end - oper_stack_begin)};
        const consistent_range_t cr =
            SILVA_EXPECT_PARSE_FWD(oper_stack[oper_stack_begin].level_name, consistent_range(ois));
        SILVA_EXPECT(cr.num_atoms <= open_term_stack.size(), MINOR);

        array_t<index_t> child_indexes;
        for (index_t i = open_term_stack.size() - cr.num_atoms; i < open_term_stack.size(); ++i) {
          child_indexes.push_back(open_term_stack[i]);
        }
        for (const auto& oi: ois) {
          for (const index_t oper_out_idx: oi.oper_output_indexes) {
            child_indexes.push_back(oper_out_idx);
          }
        }
        std::sort(child_indexes.begin(), child_indexes.end());

        oper_stack.resize(oper_stack.size() - ois.size());
        index_t subtree_size = 1;
        for (index_t i = 0; i < index_t(child_indexes.size()); ++i) {
          const index_t curr_subtree_size = output_tree[child_indexes[i]].subtree_size;
          if (i > 0) {
            SILVA_EXPECT(child_indexes[i - 1] == child_indexes[i] - curr_subtree_size, ASSERT);
          }
          subtree_size += curr_subtree_size;
        }
        open_term_stack.resize(open_term_stack.size() - cr.num_atoms);
        open_term_stack.push_back(output_tree.size());
        term_node_t tn;
        tn.num_children   = child_indexes.size();
        tn.subtree_size   = subtree_size;
        tn.rule_name      = cr.joint_level_name;
        tn.fragment_begin = cr.fragment_begin;
        tn.fragment_end   = cr.fragment_end;
        tn.tree_index     = none;
        output_tree.push_back(tn);
      }
      return {};
    }

    expected_t<void> hallucinate_concat()
    {
      SILVA_EXPECT(axe.concat_result.has_value(), ASSERT);
      const auto& reg = axe.concat_result.value();
      SILVA_EXPECT_FWD(stack_pop(reg.precedence));
      const auto& rhs = output_tree[open_term_stack.back()];
      oper_stack.push_back(oper_item_t{
          .oper       = std::get<infix_t>(reg.oper),
          .arity      = infix_t::arity,
          .level_name = reg.name,
          .precedence = reg.precedence,
          .symbols    = {{
              .fragment_begin = rhs.fragment_end,
              .fragment_end   = rhs.fragment_end,
              .related_atoms =
                  {
                      {.way = LEFTWARD, .atom_idx = 0},
                      {.way = RIGHTWARD, .atom_idx = 1},
                  },
          }},
      });
      mode = ATOM_MODE;
      return {};
    };

    expected_t<parse_tree_node_t> shunting_yard()
    {
      auto ss = nursery.stake();

      while (true) {
        const auto oper_state = nursery.get_state();
        auto res              = SILVA_EXPECT_FWD_IF(MAJOR, invoke_rule_parser_oper());
        if (res.has_value()) {
          const auto commit_oper = [&]() -> index_t {
            ss.add_proto_node(res->ptn);
            const index_t oper_out_idx = output_tree.size();
            output_tree.push_back(res->tn);
            return oper_out_idx;
          };
          const auto it = axe.results.find(res->token_id);
          if (it != axe.results.end()) {
            const axe_result_t& axe_result = it->second;
            if (axe_result.is_right_bracket) {
              nursery.set_state(oper_state);
              break;
            }
            if (mode == INFIX_MODE && axe.concat_result.has_value()) {
              if (axe_result.prefix.has_value() && !axe_result.regular.has_value()) {
                nursery.set_state(oper_state);
                SILVA_EXPECT_FWD(hallucinate_concat());
                continue;
              }
            }

            if (mode == ATOM_MODE && axe_result.prefix.has_value()) {
              const auto& prefix_result = axe_result.prefix.value();
              if (prefix_result.precedence.level_index < min_prec_level) {
                nursery.set_state(oper_state);
                break;
              }
              SILVA_EXPECT_FWD(stack_pop(prefix_result.precedence));

              if (const auto* x = std::get_if<prefix_t>(&prefix_result.oper)) {
                const index_t oper_out_idx = commit_oper();
                oper_stack.push_back(oper_item_t{
                    .oper                = *x,
                    .arity               = prefix_t::arity,
                    .level_name          = prefix_result.name,
                    .precedence          = prefix_result.precedence,
                    .symbols             = {{
                        .fragment_begin = res->ptn.fragment_begin,
                        .fragment_end   = res->ptn.fragment_end,
                        .related_atoms  = {{.way = RIGHTWARD, .atom_idx = 0}},
                    }},
                    .oper_output_indexes = {oper_out_idx},
                });
                continue;
              }
              else if (const auto* x = std::get_if<prefix_nest_t>(&prefix_result.oper)) {
                auto nest_res =
                    SILVA_EXPECT_FWD_IF(MAJOR,
                                        handle_nest(res->tn, x->right_bracket, x->nest_rule_name));
                if (nest_res.has_value()) {
                  ss.add_proto_node(res->ptn);
                  ss.add_proto_node(nest_res->ptn);
                  oper_stack.push_back(oper_item_t{
                      .oper       = *x,
                      .arity      = prefix_nest_t::arity,
                      .level_name = prefix_result.name,
                      .precedence = prefix_result.precedence,
                      .symbols =
                          {
                              {
                                  .fragment_begin = res->ptn.fragment_begin,
                                  .fragment_end   = res->ptn.fragment_end,
                                  .related_atoms  = {{.way = RIGHTWARD, .atom_idx = 0}},
                              },
                              {
                                  .fragment_begin = nest_res->right_res.ptn.fragment_begin,
                                  .fragment_end   = nest_res->right_res.ptn.fragment_end,
                                  .related_atoms =
                                      {
                                          {.way = LEFTWARD, .atom_idx = 0},
                                          {.way = RIGHTWARD, .atom_idx = 1},
                                      },
                              },
                          },
                      .oper_output_indexes = nest_res->oper_output_indexes,
                  });
                  continue;
                }
              }
            }
            else if (mode == INFIX_MODE && axe_result.regular.has_value()) {
              const auto& regular_result = axe_result.regular.value();
              if (regular_result.precedence.level_index < min_prec_level) {
                nursery.set_state(oper_state);
                break;
              }
              SILVA_EXPECT_FWD(stack_pop(regular_result.precedence));

              if (const auto* x = std::get_if<postfix_t>(&regular_result.oper)) {
                const index_t oper_out_idx = commit_oper();
                oper_stack.push_back(oper_item_t{
                    .oper                = *x,
                    .arity               = postfix_t::arity,
                    .level_name          = regular_result.name,
                    .precedence          = regular_result.precedence,
                    .symbols             = {{
                        .fragment_begin = res->ptn.fragment_begin,
                        .fragment_end   = res->ptn.fragment_end,
                        .related_atoms  = {{.way = LEFTWARD, .atom_idx = 0}},
                    }},
                    .oper_output_indexes = {oper_out_idx},
                });
                continue;
              }
              else if (const auto* x = std::get_if<postfix_nest_t>(&regular_result.oper)) {
                auto nest_res =
                    SILVA_EXPECT_FWD_IF(MAJOR,
                                        handle_nest(res->tn, x->right_bracket, x->nest_rule_name));
                if (nest_res.has_value()) {
                  ss.add_proto_node(res->ptn);
                  ss.add_proto_node(nest_res->ptn);
                  oper_stack.push_back(oper_item_t{
                      .oper       = *x,
                      .arity      = postfix_nest_t::arity,
                      .level_name = regular_result.name,
                      .precedence = regular_result.precedence,
                      .symbols =
                          {
                              {
                                  .fragment_begin = res->ptn.fragment_begin,
                                  .fragment_end   = res->ptn.fragment_end,
                                  .related_atoms =
                                      {
                                          {.way = LEFTWARD, .atom_idx = 0},
                                          {.way = RIGHTWARD, .atom_idx = 1},
                                      },
                              },
                              {
                                  .fragment_begin = nest_res->right_res.ptn.fragment_begin,
                                  .fragment_end   = nest_res->right_res.ptn.fragment_end,
                                  .related_atoms  = {{.way = LEFTWARD, .atom_idx = 1}},
                              },
                          },
                      .oper_output_indexes = nest_res->oper_output_indexes,
                  });
                  continue;
                }
              }
              else if (const auto* x = std::get_if<infix_t>(&regular_result.oper)) {
                const index_t oper_out_idx = commit_oper();
                oper_stack.push_back(oper_item_t{
                    .oper                = *x,
                    .arity               = infix_t::arity,
                    .level_name          = regular_result.name,
                    .precedence          = regular_result.precedence,
                    .symbols             = {{
                        .fragment_begin = res->ptn.fragment_begin,
                        .fragment_end   = res->ptn.fragment_end,
                        .related_atoms =
                            {
                                {.way = LEFTWARD, .atom_idx = 0},
                                {.way = RIGHTWARD, .atom_idx = 1},
                            },
                    }},
                    .oper_output_indexes = {oper_out_idx},
                });
                mode = ATOM_MODE;
                continue;
              }
              else if (const auto* x = std::get_if<ternary_t>(&regular_result.oper)) {
                auto nest_res =
                    SILVA_EXPECT_FWD_IF(MAJOR, handle_nest(res->tn, x->second, x->nest_rule_name));
                if (nest_res.has_value()) {
                  ss.add_proto_node(res->ptn);
                  ss.add_proto_node(nest_res->ptn);
                  oper_stack.push_back(oper_item_t{
                      .oper       = *x,
                      .arity      = ternary_t::arity,
                      .level_name = regular_result.name,
                      .precedence = regular_result.precedence,
                      .symbols =
                          {
                              {
                                  .fragment_begin = res->ptn.fragment_begin,
                                  .fragment_end   = res->ptn.fragment_end,
                                  .related_atoms =
                                      {
                                          {.way = LEFTWARD, .atom_idx = 0},
                                          {.way = RIGHTWARD, .atom_idx = 1},
                                      },
                              },
                              {
                                  .fragment_begin = nest_res->right_res.ptn.fragment_begin,
                                  .fragment_end   = nest_res->right_res.ptn.fragment_end,
                                  .related_atoms =
                                      {
                                          {.way = LEFTWARD, .atom_idx = 1},
                                          {.way = RIGHTWARD, .atom_idx = 2},
                                      },
                              },
                          },
                      .oper_output_indexes = nest_res->oper_output_indexes,
                  });
                  mode = ATOM_MODE;
                  continue;
                }
              }
            }
          }
          nursery.set_state(oper_state);
        }
        else {
          // TODO: should ideally remember the error here, potentionally combine it with errors
          // below, and then return the total error if this function returns in error.
          res.error().clear();
        }

        if (mode == INFIX_MODE && !axe.concat_result.has_value()) {
          break;
        }

        auto maybe_res = SILVA_EXPECT_FWD_IF(MAJOR, invoke_rule_parser(axe.atom_rule));
        if (!maybe_res.has_value()) {
          break;
        }
        auto [_, ptn, tn] = *std::move(maybe_res);
        ss.add_proto_node(ptn);
        if (mode == INFIX_MODE && axe.concat_result.has_value()) {
          SILVA_EXPECT_FWD(hallucinate_concat());
        }
        SILVA_EXPECT(mode == ATOM_MODE, ASSERT);
        open_term_stack.push_back(output_tree.size());
        output_tree.push_back(tn);
        mode = INFIX_MODE;
      }
      SILVA_EXPECT_FWD(stack_pop(precedence_min),
                       "[{}] at the end of the expression",
                       fragment_location_by());
      SILVA_EXPECT(oper_stack.empty(), MINOR);
      SILVA_EXPECT_PARSE(axe.name, open_term_stack.size() > 0, "empty expression");
      SILVA_EXPECT(open_term_stack.size() == 1,
                   MINOR,
                   "had open_term_stack of size {}",
                   open_term_stack.size());
      SILVA_EXPECT(open_term_stack.front() + 1 == output_tree.size(), MINOR);
      return ss.commit();
    }

    index_t generate_output(const tree_span_t<const term_node_t> ats,
                            const parse_tree_t& leaf_terms_tree)
    {
      auto& rv_nodes       = nursery.tree;
      const index_t retval = rv_nodes.size();
      const auto& node     = ats[0];
      if (node.tree_index.has_value()) {
        const auto to_implant = leaf_terms_tree.span().sub_tree_span_at(node.tree_index.value());
        rv_nodes.insert(rv_nodes.end(),
                        to_implant.root,
                        to_implant.root + to_implant.subtree_size());
      }
      else {
        rv_nodes.emplace_back();
        rv_nodes[retval].rule_name    = node.rule_name;
        rv_nodes[retval].num_children = ats.root->num_children;
        rv_nodes[retval].subtree_size = 1;
        const auto child_node_indexes = ats.get_children_dyn();
        for (const index_t node_index: std::ranges::reverse_view(child_node_indexes)) {
          const index_t sub_node_index =
              generate_output(ats.sub_tree_span_at(node_index), leaf_terms_tree);
          rv_nodes[retval].subtree_size += rv_nodes[sub_node_index].subtree_size;
        }
        rv_nodes[retval].fragment_begin = node.fragment_begin;
        rv_nodes[retval].fragment_end   = node.fragment_end;
      }
      return retval;
    }

    expected_t<index_t> run()
    {
      auto ss = nursery.stake();
      {
        auto ss_rule = nursery.stake();
        ss_rule.create_node(name_id_t{});
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(axe.name, shunting_yard()));

        const auto& root_node = output_tree.back();
        SILVA_EXPECT(ss_rule.orig_state.fragment_index == root_node.fragment_begin, ASSERT);
        SILVA_EXPECT(nursery.fragment_index == root_node.fragment_end, ASSERT);

        ss_rule.commit();
      }

      parse_tree_t temp_tree{.fp = {}, .nodes = std::move(nursery.tree)};
      const parse_tree_t leaf_terms_tree =
          temp_tree.span().sub_tree_span_at(ss.orig_state.tree_size).copy();
      nursery.tree = std::move(temp_tree.nodes);
      nursery.tree.resize(ss.orig_state.tree_size);

      for (auto& atn: output_tree) {
        if (atn.tree_index.has_value()) {
          atn.tree_index.value() -= ss.orig_state.tree_size;
        }
      }

      ss.commit();

      SILVA_EXPECT(output_tree.size() >= 1, ASSERT);
      tree_span_t<const term_node_t> ats{&output_tree.back(), -1};
      const index_t retval = generate_output(ats, leaf_terms_tree);
      return {retval};
    }
  };
}

namespace silva::seed {
  void axe_t::compile_reset()
  {
    atom_rule.resolve_clear();
    oper_rule.resolve_clear();
    auto res = impl::for_each_name_id_ref(*this, [&](name_id_ref_t& nir) -> expected_t<void> {
      nir.resolve_clear();
      return {};
    });
    SILVA_ASSERT(res.has_value());
  }

  expected_t<parse_tree_node_t>
  axe_t::apply(parse_tree_nursery_t& nursery,
               const name_id_t rule_name,
               delegate_t<expected_t<parse_tree_node_t>(name_id_t)> rule_parser) const
  {
    const auto level_it = level_map.find(rule_name);
    SILVA_EXPECT(level_it != level_map.end(),
                 MINOR,
                 "unknown rule-name {}",
                 lp->name_id_wrap(rule_name));
    const impl::level_index_t min_prec_level = level_it->second;
    impl::axe_run_t run{
        .axe            = *this,
        .nursery        = nursery,
        .rule_parser    = rule_parser,
        .min_prec_level = min_prec_level,
    };
    const index_t orig_fragment_index = nursery.fragment_index;
    const index_t created_node =
        SILVA_EXPECT_FWD(run.run(),
                         "[{}] when parsing expression starting here",
                         nursery.fragment_location_at(orig_fragment_index));
    auto& rv_nodes = nursery.tree;
    parse_tree_node_t retval;
    retval.num_children   = 1;
    retval.subtree_size   = rv_nodes[created_node].subtree_size;
    retval.fragment_begin = rv_nodes[created_node].fragment_begin;
    retval.fragment_end   = rv_nodes[created_node].fragment_end;
    return retval;
  }
}
