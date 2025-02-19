#include "parse_axe.hpp"

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
                                           optional_t<primary_nest_t> maybe_primary_nest,
                                           const vector_t<parse_axe_level_desc_t>& level_descs)
  {
    using enum assoc_t;
    using enum error_level_t;
    for (const parse_axe_level_desc_t& level_desc: level_descs) {
      for (const auto& oper: level_desc.opers) {
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

    parse_axe_t retval{.tcp = tcp};

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

    if (maybe_primary_nest.has_value()) {
      const auto& primary_nest = maybe_primary_nest.value();
      SILVA_EXPECT_FWD(register_op(primary_nest.left_bracket,
                                   primary_nest,
                                   full_name_id_none,
                                   precedence_infinite));
      SILVA_EXPECT_FWD(register_right_op(primary_nest.right_bracket));
    }
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
        else if (const auto* x = std::get_if<primary_nest_t>(&oper); x) {
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
}
