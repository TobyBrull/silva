#include "seed_axe.hpp"

#include "seed.hpp"

#include "canopy/variant.hpp"

#include <ranges>
#include <variant>

namespace silva::impl {
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

  struct seed_axe_create_nursery_t {
    syntax_ward_ptr_t swp;
    name_id_t seed_axe_name     = name_id_root;
    name_id_t atom_rule_name_id = name_id_root;
    const name_id_style_t& nis  = swp->default_name_id_style();

    const token_id_t ti_atom_nest    = *swp->token_id("atom_nest");
    const token_id_t ti_prefix       = *swp->token_id("prefix");
    const token_id_t ti_prefix_nest  = *swp->token_id("prefix_nest");
    const token_id_t ti_infix        = *swp->token_id("infix");
    const token_id_t ti_infix_flat   = *swp->token_id("infix_flat");
    const token_id_t ti_ternary      = *swp->token_id("ternary");
    const token_id_t ti_postfix      = *swp->token_id("postfix");
    const token_id_t ti_postfix_nest = *swp->token_id("postfix_nest");
    const token_id_t ti_concat       = *swp->token_id("concat");
    const token_id_t ti_nest         = *swp->token_id("nest");
    const token_id_t ti_ltr          = *swp->token_id("ltr");
    const token_id_t ti_rtl          = *swp->token_id("rtl");

    const name_id_t ni_seed        = swp->name_id_of("Seed");
    const name_id_t ni_axe         = swp->name_id_of(ni_seed, "Axe");
    const name_id_t ni_axe_level   = swp->name_id_of(ni_axe, "Level");
    const name_id_t ni_axe_assoc   = swp->name_id_of(ni_axe, "Assoc");
    const name_id_t ni_axe_ops     = swp->name_id_of(ni_axe, "Ops");
    const name_id_t ni_axe_op_type = swp->name_id_of(ni_axe, "OpType");
    const name_id_t ni_axe_op      = swp->name_id_of(ni_axe, "Op");
    const name_id_t ni_nt          = swp->name_id_of(ni_seed, "Nonterminal");
    const name_id_t ni_nt_base     = swp->name_id_of(ni_nt, "Base");
    const name_id_t ni_term        = swp->name_id_of(ni_seed, "Terminal");

    seed_axe_t retval{
        .swp  = swp,
        .name = seed_axe_name,
    };

    seed_axe_create_nursery_t(syntax_ward_ptr_t swp, const name_id_t seed_axe_name)
      : swp(swp), seed_axe_name(seed_axe_name)
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
                     swp->token_id_wrap(token_id),
                     result.prefix.value().pts);
        SILVA_EXPECT(!result.is_right_bracket,
                     MINOR,
                     "{}: {} has been used as right-bracket operator before",
                     pts,
                     swp->token_id_wrap(token_id));
        result.prefix = result_oper_t<oper_prefix_t>{
            .oper       = variant_get<oper_prefix_t>(oper),
            .name       = retval.swp->name_id(level_name, token_id),
            .precedence = precedence,
            .pts        = pts,
        };
      }
      else if (variant_holds<oper_regular_t>(oper)) {
        SILVA_EXPECT(!result.regular.has_value(),
                     MINOR,
                     "{}: {} has been used as regular-style operator before at {}",
                     pts,
                     swp->token_id_wrap(token_id),
                     result.regular.value().pts);
        SILVA_EXPECT(!result.is_right_bracket,
                     MINOR,
                     "{}: {} has been used as right-bracket operator before",
                     pts,
                     swp->token_id_wrap(token_id));
        result.regular = result_oper_t<oper_regular_t>{
            .oper       = variant_get<oper_regular_t>(oper),
            .name       = retval.swp->name_id(level_name, token_id),
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
                   swp->token_id_wrap(token_id),
                   result.prefix.value().pts);
      SILVA_EXPECT(!result.regular.has_value(),
                   MINOR,
                   "{}: {} has been used as right-bracket operator before at {}",
                   pts,
                   swp->token_id_wrap(token_id),
                   result.regular.value().pts);
      result.is_right_bracket = true;
      return {};
    }

    expected_t<void> op(const token_id_t axe_op_type, const parse_tree_span_t pts_op)
    {
      SILVA_EXPECT(pts_op[0].rule_name == ni_axe_op, BROKEN_SEED);
      const token_id_t axe_op = pts_op.first_token_id();
      SILVA_EXPECT(swp->token_infos[axe_op].category == token_category_t::STRING ||
                       axe_op == ti_concat,
                   BROKEN_SEED);
      if (axe_op_type != ti_infix && axe_op_type != ti_infix_flat) {
        SILVA_EXPECT(
            axe_op != ti_concat,
            MINOR,
            "{} the 'concat' token may only be used with 'infix' or 'infix_flat' operations.",
            pts_op);
      }
      return {};
    }

    expected_t<void> ops(const index_t level_index,
                         const token_id_t base_name,
                         const assoc_t assoc,
                         const parse_tree_span_t pts_ops)
    {
      SILVA_EXPECT(pts_ops[0].rule_name == ni_axe_ops, BROKEN_SEED);
      auto [it, end] = pts_ops.children_range();
      SILVA_EXPECT(it != end, BROKEN_SEED);
      const auto pts_op_type = pts_ops.sub_tree_span_at(it.pos);
      SILVA_EXPECT(pts_op_type[0].rule_name == ni_axe_op_type, BROKEN_SEED);
      const token_id_t axe_op_type = pts_op_type.first_token_id();
      SILVA_EXPECT(axe_op_type == ti_atom_nest || axe_op_type == ti_prefix ||
                       axe_op_type == ti_prefix_nest || axe_op_type == ti_infix ||
                       axe_op_type == ti_infix_flat || axe_op_type == ti_ternary ||
                       axe_op_type == ti_postfix || axe_op_type == ti_postfix_nest,
                   BROKEN_SEED);
      ++it;

      optional_t<index_t> nest_rule_name;
      if (it != end) {
        const auto pts_nt = pts_ops.sub_tree_span_at(it.pos);
        if (pts_nt[0].rule_name == ni_nt) {
          nest_rule_name = SILVA_EXPECT_FWD(nis.derive_name(retval.name, pts_nt));
          ++it;
        }
      }

      if (axe_op_type == ti_atom_nest || axe_op_type == ti_prefix_nest ||
          axe_op_type == ti_ternary || axe_op_type == ti_postfix_nest) {
        const index_t num_op_tokens = pts_ops[0].num_children - it.child_index;
        SILVA_EXPECT(num_op_tokens % 2 == 0,
                     MINOR,
                     "{} for operations [ atom_nest prefix_nest ternary postfix_nest ] "
                     "an even number of operators is expected",
                     pts_ops);
      }
      else {
        SILVA_EXPECT(!nest_rule_name.has_value(),
                     MINOR,
                     "{} nested rule is only allowed on operation types [ atom_nest prefix_nest "
                     "ternary postfix_nest ]",
                     pts_ops);
      }

      if (assoc == NEST) {
        SILVA_EXPECT(axe_op_type == ti_atom_nest,
                     MINOR,
                     "{} a 'nest' level only allows operators of type 'atom_nest', not {}",
                     pts_ops,
                     swp->token_id_wrap(axe_op_type));
      }
      else {
        if (assoc == LEFT_TO_RIGHT) {
          SILVA_EXPECT(axe_op_type == ti_postfix || axe_op_type == ti_postfix_nest ||
                           axe_op_type == ti_infix || axe_op_type == ti_infix_flat ||
                           axe_op_type == ti_ternary,
                       MINOR,
                       "{} an 'ltr' level requires operators of type [ postfix postfix_nest_t "
                       "infix_t ternary_t ], not {}",
                       pts_ops,
                       swp->token_id_wrap(axe_op_type));
        }
        else if (assoc == RIGHT_TO_LEFT) {
          SILVA_EXPECT(axe_op_type == ti_prefix || axe_op_type == ti_prefix_nest ||
                           axe_op_type == ti_infix || axe_op_type == ti_infix_flat ||
                           axe_op_type == ti_ternary,
                       MINOR,
                       "{} 'rtl' levels require operators of type [ prefix_t prefix_nest_t "
                       "infix_t ternary_t ], not {}",
                       pts_ops,
                       swp->token_id_wrap(axe_op_type));
        }
        else {
          SILVA_EXPECT(false, BROKEN_SEED);
        }
      }

      const precedence_t precedence{
          .level_index = level_index,
          .assoc       = assoc,
      };

      const name_id_t full_name = swp->name_id(seed_axe_name, base_name);

      const auto& get_next_not_concat =
          [&]() -> expected_t<tuple_t<token_id_t, parse_tree_span_t>> {
        SILVA_EXPECT(it != end, ASSERT);
        const auto pts_op = pts_ops.sub_tree_span_at(it.pos);
        SILVA_EXPECT_FWD(op(axe_op_type, pts_op));
        const token_id_t ti = pts_op.first_token_id();
        ++it;
        SILVA_EXPECT(ti != ti_concat, ASSERT);
        const token_id_t retval = SILVA_EXPECT_FWD(swp->token_id_in_string(ti));
        return {{retval, pts_op}};
      };

      const auto& get_next_or_concat =
          [&]() -> expected_t<tuple_t<optional_t<token_id_t>, parse_tree_span_t>> {
        SILVA_EXPECT(it != end, ASSERT);
        const auto pts_op = pts_ops.sub_tree_span_at(it.pos);
        SILVA_EXPECT_FWD(op(axe_op_type, pts_op));
        const token_id_t ti = pts_op.first_token_id();
        ++it;
        if (ti == ti_concat) {
          return {{std::nullopt, pts_op}};
        }
        const token_id_t retval = SILVA_EXPECT_FWD(swp->token_id_in_string(ti));
        return {{retval, pts_op}};
      };

      // Conceptually, it would make more sense to lower the "while" loop into each of the "if-else"
      // conditions, but the code is a bit shorter this way.
      while (it != end) {
        if (axe_op_type == ti_atom_nest) {
          const auto [ti_left, pts_left]   = SILVA_EXPECT_FWD(get_next_not_concat());
          const auto [ti_right, pts_right] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_left,
                                       atom_nest_t{
                                           .left_bracket   = ti_left,
                                           .right_bracket  = ti_right,
                                           .nest_rule_name = nest_rule_name.value_or(name_id_root),
                                       },
                                       full_name,
                                       precedence,
                                       pts_left));
          SILVA_EXPECT_FWD(register_right_op(ti_right, pts_right));
        }
        else if (axe_op_type == ti_prefix) {
          const auto [ti_op, pts_op] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_op,
                                       prefix_t{
                                           .token_id = ti_op,
                                       },
                                       full_name,
                                       precedence,
                                       pts_op));
        }
        else if (axe_op_type == ti_prefix_nest) {
          const auto [ti_left, pts_left]   = SILVA_EXPECT_FWD(get_next_not_concat());
          const auto [ti_right, pts_right] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_left,
                                       prefix_nest_t{
                                           .left_bracket   = ti_left,
                                           .right_bracket  = ti_right,
                                           .nest_rule_name = nest_rule_name.value_or(name_id_root),
                                       },
                                       full_name,
                                       precedence,
                                       pts_left));
          SILVA_EXPECT_FWD(register_right_op(ti_right, pts_right));
        }
        else if (axe_op_type == ti_infix || axe_op_type == ti_infix_flat) {
          const auto [maybe_ti_op, pts_op] = SILVA_EXPECT_FWD(get_next_or_concat());
          const bool is_flatten            = (axe_op_type == ti_infix_flat);
          const bool is_concat             = !maybe_ti_op.has_value();
          precedence_t used_prec           = precedence;
          const token_id_t ti_op           = maybe_ti_op.value_or(ti_concat);
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
                .name       = retval.swp->name_id(full_name, ti_op),
                .precedence = used_prec,
                .pts        = pts_op,
            });
          }
          else {
            SILVA_EXPECT_FWD(register_op(ti_op, op, full_name, used_prec, pts_op));
          }
        }
        else if (axe_op_type == ti_ternary) {
          const auto [ti_first, pts_first]   = SILVA_EXPECT_FWD(get_next_not_concat());
          const auto [ti_second, pts_second] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_first,
                                       ternary_t{
                                           .first          = ti_first,
                                           .second         = ti_second,
                                           .nest_rule_name = nest_rule_name.value_or(name_id_root),
                                       },
                                       full_name,
                                       precedence,
                                       pts_first));
          SILVA_EXPECT_FWD(register_right_op(ti_second, pts_second));
        }
        else if (axe_op_type == ti_postfix) {
          const auto [ti_op, pts_op] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_op,
                                       postfix_t{
                                           .token_id = ti_op,
                                       },
                                       full_name,
                                       precedence,
                                       pts_op));
        }
        else if (axe_op_type == ti_postfix_nest) {
          const auto [ti_left, pts_left]   = SILVA_EXPECT_FWD(get_next_not_concat());
          const auto [ti_right, pts_right] = SILVA_EXPECT_FWD(get_next_not_concat());
          SILVA_EXPECT_FWD(register_op(ti_left,
                                       postfix_nest_t{
                                           .left_bracket   = ti_left,
                                           .right_bracket  = ti_right,
                                           .nest_rule_name = nest_rule_name.value_or(name_id_root),
                                       },
                                       full_name,
                                       precedence,
                                       pts_left));
          SILVA_EXPECT_FWD(register_right_op(ti_right, pts_right));
        }
        else {
          SILVA_EXPECT(false, MAJOR, "Unexpected variant: {}", swp->token_id_wrap(axe_op_type));
        }
      }
      return {};
    }

    bool may_be_nest = true;

    expected_t<void> level(const index_t level_index, const parse_tree_span_t pts_level)
    {
      SILVA_EXPECT(pts_level[0].rule_name == ni_axe_level, BROKEN_SEED);
      token_id_t base_name = token_id_none;
      assoc_t assoc        = INVALID;
      for (const auto [child_node_index, child_index]: pts_level.children_range()) {
        if (child_index == 0) {
          SILVA_EXPECT(pts_level[child_node_index].rule_name == ni_nt_base, BROKEN_SEED);
          base_name = SILVA_EXPECT_FWD(
              nis.derive_base_name(seed_axe_name, pts_level.sub_tree_span_at(child_node_index)));
        }
        else if (child_index == 1) {
          const auto pts_assoc = pts_level.sub_tree_span_at(child_node_index);
          SILVA_EXPECT(pts_assoc[0].rule_name == ni_axe_assoc, BROKEN_SEED);
          const token_id_t ti_assoc = pts_assoc.first_token_id();
          if (ti_assoc == ti_nest) {
            assoc = NEST;
          }
          else if (ti_assoc == ti_ltr) {
            assoc = LEFT_TO_RIGHT;
          }
          else if (ti_assoc == ti_rtl) {
            assoc = RIGHT_TO_LEFT;
          }
          else {
            SILVA_EXPECT(false, BROKEN_SEED);
          }

          if (assoc == NEST) {
            SILVA_EXPECT(may_be_nest,
                         MINOR,
                         "{} the 'nest' levels must occur before all non-'nest' levels",
                         pts_level);
          }
          else {
            may_be_nest = false;
          }
        }
        else {
          const auto pts_ops = pts_level.sub_tree_span_at(child_node_index);
          SILVA_EXPECT_FWD(ops(level_index, base_name, assoc, pts_ops));
        }
      }
      return {};
    }

    expected_t<void> run(const parse_tree_span_t pts_axe)
    {
      auto [it, end] = pts_axe.children_range();
      SILVA_EXPECT(it != end, MINOR, "{} should have at least one child", pts_axe);
      const auto pts_axe_nt = pts_axe.sub_tree_span_at(it.pos);
      SILVA_EXPECT(pts_axe_nt[0].rule_name == ni_nt, MINOR);
      const name_id_t parent_scope = swp->name_infos[seed_axe_name].parent_name;
      retval.atom_rule             = SILVA_EXPECT_FWD(nis.derive_name(parent_scope, pts_axe_nt));
      ++it;

      index_t curr_level = pts_axe[0].num_children;
      while (it != end) {
        curr_level -= 1;
        SILVA_EXPECT_FWD(level(curr_level, pts_axe.sub_tree_span_at(it.pos)));
        ++it;
      }
      return {};
    }
  };
}

namespace silva {
  expected_t<seed_axe_t>
  seed_axe_create(syntax_ward_ptr_t swp, const name_id_t seed_axe_name, const parse_tree_span_t pts)
  {
    impl::seed_axe_create_nursery_t nursery(swp, seed_axe_name);
    SILVA_EXPECT_FWD(nursery.run(pts));
    return std::move(nursery.retval);
  }
}

namespace silva::impl {
  struct seed_axe_run_t {
    const seed_axe_t& seed_axe;
    parse_tree_nursery_t& nursery;
    delegate_t<expected_t<parse_tree_node_t>(name_id_t)> rule_parser;

    syntax_ward_ptr_t swp = nursery.swp;

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
    mode_t mode = ATOM_MODE;

    struct oper_item_t {
      oper_any_t oper;
      index_t arity        = 0;
      name_id_t level_name = 0;
      precedence_t precedence;
      small_vector_t<index_t, 2> covered_token_indexes;
      optional_t<index_t> min_token_index;
      optional_t<index_t> max_token_index;
    };
    vector_t<oper_item_t> oper_stack;

    // Indexes into the "output_tree" vector.
    vector_t<index_t> open_term_stack;

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
    vector_t<term_node_t> output_tree;

    // functions

    // struct rule_parser_result_t {};
    // expected_t<term_data_t> invoke_rule_parser()
    // {
    //   auto ss                  = nursery.stake();
    //   const index_t tree_index = nursery.tree.size();
    //   auto atom_result         = SILVA_EXPECT_FWD(rule_parser(seed_axe.atom_rule));
    //   SILVA_EXPECT(atom_result.num_children == 1,
    //                ASSERT,
    //                "The atom function given to seed_axe_t must always parse a single child");
    //   ss.add_proto_node(atom_result);
    //   return term_data_t{
    //       .rule_name   = seed_axe.atom_rule,
    //       .token_begin = atom_result.token_begin,
    //       .token_end   = atom_result.token_end,
    //       .tree_index  = tree_index,
    //   };
    // }

    expected_t<term_node_t> try_parse_atom(parse_tree_nursery_t::stake_t& ss_rule)
    {
      const index_t atom_child_node_index = nursery.tree.size();
      auto atom_result                    = SILVA_EXPECT_FWD(rule_parser(seed_axe.atom_rule));
      SILVA_EXPECT(atom_result.num_children == 1,
                   MAJOR,
                   "The atom function given to seed_axe_t must always parse a single child");
      ss_rule.add_proto_node(atom_result);
      term_node_t retval;
      retval.num_children = 0;
      retval.subtree_size = 1;
      retval.rule_name    = seed_axe.atom_rule;
      retval.token_begin  = atom_result.token_begin;
      retval.token_end    = atom_result.token_end;
      retval.tree_index   = atom_child_node_index;
      return retval;
    }

    expected_t<pair_t<index_t, index_t>> handle_nest(parse_tree_nursery_t::stake_t& ss_rule,
                                                     const token_id_t left_token,
                                                     const token_id_t right_token,
                                                     const name_id_t nest_rule_name)
    {
      const index_t token_begin = nursery.token_index;
      SILVA_EXPECT_PARSE(ss_rule.proto_node.rule_name,
                         nursery.num_tokens_left() >= 1 && nursery.token_id_by() == left_token,
                         "expected {}, got {}",
                         swp->token_id_wrap(left_token),
                         swp->token_id_wrap(nursery.token_id_by()));
      nursery.token_index += 1;

      const name_id_t used_rule_name =
          (nest_rule_name != name_id_root) ? nest_rule_name : seed_axe.name;
      const index_t atom_child_node_index = nursery.tree.size();
      const auto sub_expr                 = SILVA_EXPECT_FWD(rule_parser(used_rule_name));
      SILVA_EXPECT(sub_expr.num_children == 1,
                   MAJOR,
                   "nest expression must always result in a single node");

      SILVA_EXPECT_PARSE(ss_rule.proto_node.rule_name,
                         nursery.num_tokens_left() >= 1 && nursery.token_id_by() == right_token,
                         "expected {}, got {}",
                         swp->token_id_wrap(right_token),
                         swp->token_id_wrap(nursery.token_id_by()));
      nursery.token_index += 1;

      ss_rule.add_proto_node(sub_expr);

      const index_t token_end = nursery.token_index;
      term_node_t tn;
      tn.num_children = 0;
      tn.subtree_size = 1;
      tn.rule_name    = used_rule_name;
      tn.token_begin  = token_begin + 1;
      tn.token_end    = token_end - 1;
      tn.tree_index   = atom_child_node_index;
      output_tree.push_back(tn);
      return {{token_begin, token_end}};
    }

    struct consistent_range_t {
      index_t num_atoms          = 0;
      name_id_t joint_level_name = name_id_root;
      index_t token_begin        = 0;
      index_t token_end          = 0;
    };
    expected_t<consistent_range_t> consistent_range(span_t<const oper_item_t> ois) const
    {
      // The following probably doesn't make sense if ois.size() > 1 and the operators are of
      // type ternary_t, prefix_nest_t, or postfix_nest_t. Should probably rewrite the logic to
      // cover this case explicitly, and then handle ois.size() > 1 only for prefix_t,
      // postfix_t, and infix_t.

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
                   nursery.token_position_by(),
                   combined_arity,
                   open_term_stack.size());
      const index_t atom_stack_begin = open_term_stack.size() - combined_arity;
      const auto& front_atn          = output_tree[open_term_stack[atom_stack_begin]];
      consistent_range_t retval{
          .num_atoms        = combined_arity,
          .joint_level_name = ois.front().level_name,
          .token_begin      = front_atn.token_begin,
          .token_end        = front_atn.token_end,
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
      for (index_t atom_stack_index = atom_stack_begin + 1;
           atom_stack_index < open_term_stack.size();
           ++atom_stack_index) {
        const auto& atn = output_tree[open_term_stack[atom_stack_index]];
        SILVA_EXPECT(retval.token_end == atn.token_begin, MINOR);
        retval.token_end = atn.token_end;
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
        const consistent_range_t cr =
            SILVA_EXPECT_PARSE_FWD(oper_stack[oper_stack_begin].level_name, consistent_range(ois));
        oper_stack.resize(oper_stack.size() - ois.size());
        SILVA_EXPECT(cr.num_atoms <= open_term_stack.size(), MINOR);
        index_t subtree_size = 1;
        for (index_t i = open_term_stack.size() - cr.num_atoms; i < open_term_stack.size(); ++i) {
          const index_t curr_subtree_size = output_tree[open_term_stack[i]].subtree_size;
          if (i > open_term_stack.size() - cr.num_atoms) {
            SILVA_EXPECT(open_term_stack[i - 1] == open_term_stack[i] - curr_subtree_size, ASSERT);
          }
          subtree_size += curr_subtree_size;
        }
        open_term_stack.resize(open_term_stack.size() - cr.num_atoms);
        open_term_stack.push_back(output_tree.size());
        term_node_t tn;
        tn.num_children = cr.num_atoms;
        tn.subtree_size = subtree_size;
        tn.rule_name    = cr.joint_level_name;
        tn.token_begin  = cr.token_begin;
        tn.token_end    = cr.token_end;
        tn.tree_index   = none;
        output_tree.push_back(tn);
      }
      return {};
    }

    expected_t<void> hallucinate_concat()
    {
      SILVA_EXPECT(seed_axe.concat_result.has_value(), ASSERT);
      const auto& reg = seed_axe.concat_result.value();
      SILVA_EXPECT_FWD(stack_pop(reg.precedence));
      oper_stack.push_back(oper_item_t{
          .oper       = std::get<infix_t>(reg.oper),
          .arity      = infix_t::arity,
          .level_name = reg.name,
          .precedence = reg.precedence,
      });
      mode = ATOM_MODE;
      return {};
    };

    expected_t<parse_tree_node_t> shunting_yard()
    {
      auto ss = nursery.stake();

      while (nursery.num_tokens_left() >= 1) {
        const auto it = seed_axe.results.find(nursery.token_id_by());
        if (it == seed_axe.results.end()) {
          if (mode == INFIX_MODE && !seed_axe.concat_result.has_value()) {
            break;
          }
          // Current token is not one of the known operators, so it has to be an atom or the end
          // of the expression
          auto maybe_tn = SILVA_EXPECT_FWD_IF(try_parse_atom(ss), MAJOR);
          if (!maybe_tn.has_value()) {
            break;
          }
          if (mode == INFIX_MODE && seed_axe.concat_result.has_value()) {
            SILVA_EXPECT_FWD(hallucinate_concat());
          }
          if (mode == ATOM_MODE) {
            const index_t atom_tree_node_index = output_tree.size();
            output_tree.push_back(std::move(maybe_tn).value());
            open_term_stack.push_back(atom_tree_node_index);
            mode = INFIX_MODE;
            continue;
          }
        }
        else {
          const seed_axe_result_t& pa_result = it->second;
          if (pa_result.is_right_bracket) {
            break;
          }
          if (mode == INFIX_MODE && seed_axe.concat_result.has_value()) {
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
            SILVA_EXPECT_PARSE(seed_axe.name,
                               pa_result.prefix.has_value(),
                               "found non-prefix operator {} when expecting next atom",
                               swp->token_id_wrap(nursery.token_id_by()));
            const auto& res = pa_result.prefix.value();
            SILVA_EXPECT_FWD(stack_pop(res.precedence));

            if (const auto* x = std::get_if<atom_nest_t>(&res.oper)) {
              const auto [token_begin, token_end] = SILVA_EXPECT_FWD(
                  handle_nest(ss, x->left_bracket, x->right_bracket, x->nest_rule_name));
              term_node_t tn;
              tn.num_children = 1;
              tn.subtree_size = output_tree.back().subtree_size + 1;
              tn.rule_name    = res.name;
              tn.token_begin  = token_begin;
              tn.token_end    = token_end;
              tn.tree_index   = none;
              output_tree.push_back(tn);
              open_term_stack.push_back(output_tree.size() - 1);
              mode = INFIX_MODE;
              continue;
            }
            else if (const auto* x = std::get_if<prefix_t>(&res.oper)) {
              oper_stack.push_back(oper_item_t{
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
              const auto [token_begin, token_end] = SILVA_EXPECT_FWD(
                  handle_nest(ss, x->left_bracket, x->right_bracket, x->nest_rule_name));
              open_term_stack.push_back(output_tree.size() - 1);
              oper_stack.push_back(oper_item_t{
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
            SILVA_EXPECT_FWD(stack_pop(res.precedence));

            if (const auto* x = std::get_if<postfix_t>(&res.oper)) {
              oper_stack.push_back(oper_item_t{
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
              const auto [token_begin, token_end] = SILVA_EXPECT_FWD(
                  handle_nest(ss, x->left_bracket, x->right_bracket, x->nest_rule_name));
              open_term_stack.push_back(output_tree.size() - 1);
              oper_stack.push_back(oper_item_t{
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
              oper_stack.push_back(oper_item_t{
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
                  SILVA_EXPECT_FWD(handle_nest(ss, x->first, x->second, x->nest_rule_name));
              open_term_stack.push_back(output_tree.size() - 1);
              oper_stack.push_back(oper_item_t{
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
      SILVA_EXPECT_FWD(stack_pop(precedence_min),
                       "[{}] at the end of the expression",
                       token_position_by());
      SILVA_EXPECT(oper_stack.empty(), MINOR);
      SILVA_EXPECT_PARSE(seed_axe.name, open_term_stack.size() > 0, "empty expression");
      SILVA_EXPECT(open_term_stack.size() == 1, MINOR);
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
        rv_nodes.insert(rv_nodes.end(), to_implant.root, to_implant.root + to_implant.size());
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
        rv_nodes[retval].token_begin = node.token_begin;
        rv_nodes[retval].token_end   = node.token_end;
      }
      return retval;
    }

    expected_t<index_t> run()
    {
      auto ss = nursery.stake();
      {
        auto ss_rule = nursery.stake();
        ss_rule.create_node(name_id_root);
        ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(seed_axe.name, shunting_yard()));

        const auto& root_node          = output_tree.back();
        const index_t expr_token_begin = root_node.token_begin;
        const index_t expr_token_end   = root_node.token_end;
        SILVA_EXPECT(ss_rule.orig_state.token_index == expr_token_begin, ASSERT);
        SILVA_EXPECT(nursery.token_index == expr_token_end, ASSERT);

        ss_rule.commit();
      }

      const parse_tree_t leaf_terms_tree = parse_tree_span_t{nursery.tree.data(), 1, nursery.tp}
                                               .sub_tree_span_at(ss.orig_state.tree_size)
                                               .copy();
      for (auto& atn: output_tree) {
        if (atn.tree_index.has_value()) {
          atn.tree_index.value() -= ss.orig_state.tree_size;
        }
      }
      const index_t final_token_index = nursery.token_index;
      ss.clear();
      nursery.token_index = final_token_index;

      SILVA_EXPECT(output_tree.size() >= 1, ASSERT);
      tree_span_t<const term_node_t> ats{&output_tree.back(), -1};
      const index_t retval = generate_output(ats, leaf_terms_tree);
      return {retval};
    }
  };
}

namespace silva {
  expected_t<parse_tree_node_t>
  seed_axe_t::apply(parse_tree_nursery_t& nursery,
                    delegate_t<expected_t<parse_tree_node_t>(name_id_t)> rule_parser) const
  {
    impl::seed_axe_run_t run{
        .seed_axe    = *this,
        .nursery     = nursery,
        .rule_parser = rule_parser,
    };
    const index_t orig_token_index = nursery.token_index;
    const index_t created_node     = SILVA_EXPECT_FWD(run.run(),
                                                  "[{}] when parsing expression starting here",
                                                  nursery.token_position_at(orig_token_index));
    auto& rv_nodes                 = nursery.tree;
    parse_tree_node_t retval;
    retval.num_children = 1;
    retval.subtree_size = rv_nodes[created_node].subtree_size;
    retval.token_begin  = rv_nodes[created_node].token_begin;
    retval.token_end    = rv_nodes[created_node].token_end;
    return retval;
  }
}
