#pragma once

#include "parse_tree.hpp"
#include "syntax_ward.hpp"

#include "canopy/types.hpp"

namespace silva::impl {
  enum class assoc_t {
    INVALID,
    NEST,
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT,
  };

  struct prefix_t {
    constexpr static inline index_t arity = 1;
    token_id_t token_id{0};

    friend auto operator<=>(const prefix_t&, const prefix_t&) = default;
  };

  struct prefix_nest_t {
    constexpr static inline index_t arity = 2;
    token_id_t left_bracket{0};
    token_id_t right_bracket{0};

    optional_t<name_id_t> nest_rule_name;

    friend auto operator<=>(const prefix_nest_t&, const prefix_nest_t&) = default;
  };

  struct atom_nest_t {
    constexpr static inline index_t arity = -1;
    token_id_t left_bracket{0};
    token_id_t right_bracket{0};

    optional_t<name_id_t> nest_rule_name;

    friend auto operator<=>(const atom_nest_t&, const atom_nest_t&) = default;
  };

  struct infix_t {
    constexpr static inline index_t arity = 2;
    token_id_t token_id{0};
    bool concat  = false;
    bool flatten = false;

    friend auto operator<=>(const infix_t&, const infix_t&) = default;
  };

  struct ternary_t {
    constexpr static inline index_t arity = 3;
    token_id_t first{0};
    token_id_t second{0};

    optional_t<name_id_t> nest_rule_name;

    friend auto operator<=>(const ternary_t&, const ternary_t&) = default;
  };

  struct postfix_t {
    constexpr static inline index_t arity = 1;
    token_id_t token_id{0};

    friend auto operator<=>(const postfix_t&, const postfix_t&) = default;
  };

  struct postfix_nest_t {
    constexpr static inline index_t arity = 2;
    token_id_t left_bracket{0};
    token_id_t right_bracket{0};

    optional_t<name_id_t> nest_rule_name;

    friend auto operator<=>(const postfix_nest_t&, const postfix_nest_t&) = default;
  };

  using oper_prefix_t  = variant_t<prefix_t, prefix_nest_t, atom_nest_t>;
  using oper_infix_t   = variant_t<infix_t, ternary_t>;
  using oper_postfix_t = variant_t<postfix_t, postfix_nest_t>;
  using oper_regular_t = variant_join_t<oper_infix_t, oper_postfix_t>;
  using oper_any_t     = variant_join_t<oper_regular_t, oper_prefix_t>;

  using level_index_t = index_t;

  struct precedence_t {
    level_index_t level_index = 0;
    assoc_t assoc             = assoc_t::INVALID;
    optional_t<token_id_t> flatten_id;

    friend auto operator<=>(const precedence_t&, const precedence_t&) = default;
  };
  template<typename Oper>
  struct result_oper_t {
    Oper oper;
    name_id_t name = 0;
    precedence_t precedence;
    parse_tree_span_t pts;

    friend auto operator<=>(const result_oper_t<Oper>&, const result_oper_t<Oper>&) = default;
  };

  struct seed_axe_result_t {
    optional_t<result_oper_t<oper_prefix_t>> prefix;
    optional_t<result_oper_t<oper_regular_t>> regular;
    bool is_right_bracket = false;

    friend auto operator<=>(const seed_axe_result_t&, const seed_axe_result_t&) = default;
  };
}
