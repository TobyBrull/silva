#pragma once

#include "canopy/types.hpp"
#include "tokenization.hpp"

// * https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

// An mechanism for parsing [a]rithmetic e[x]pr[e]ssions. This is a version of the Shunting Yard
// algorithm.

namespace silva::parse_axe {
  enum class assoc_t {
    INVALID,
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT,
    FLAT,
  };

  struct prefix_t {
    constexpr static inline index_t arity = 1;
    token_id_t token_id{0};
  };

  struct prefix_nest_t {
    constexpr static inline index_t arity = 2;
    token_id_t left_bracket{0};
    token_id_t right_bracket{0};
  };

  struct primary_nest_t {
    constexpr static inline index_t arity = -1;
    token_id_t left_bracket{0};
    token_id_t right_bracket{0};
  };

  struct infix_t {
    constexpr static inline index_t arity = 2;
    token_id_t token_id{0};
  };

  struct ternary_t {
    constexpr static inline index_t arity = 3;
    token_id_t first{0};
    token_id_t second{0};
  };

  struct postfix_t {
    constexpr static inline index_t arity = 1;
    token_id_t token_id{0};
  };

  struct postfix_nest_t {
    constexpr static inline index_t arity = 2;
    token_id_t left_bracket{0};
    token_id_t right_bracket{0};
  };

  using oper_prefix_t  = variant_t<prefix_t, prefix_nest_t, primary_nest_t>;
  using oper_infix_t   = variant_t<infix_t, ternary_t>;
  using oper_postfix_t = variant_t<postfix_t, postfix_nest_t>;

  using oper_regular_t = variant_t<infix_t, ternary_t, postfix_t, postfix_nest_t>;
  //                   = oper_infix_t + oper_postfix_t

  using oper_any_t = variant_t<prefix_t,
                               prefix_nest_t,
                               primary_nest_t,
                               infix_t,
                               ternary_t,
                               postfix_t,
                               postfix_nest_t>;
  //               = oper_regular_t + oper_prefix_t

  using level_index_t = index_t;

  struct precedence_t {
    level_index_t level_index = 0;
    assoc_t assoc             = assoc_t::INVALID;

    friend bool operator<(const precedence_t& lhs, const precedence_t& rhs);
  };

  template<typename Oper>
  struct result_oper_t {
    Oper oper;
    full_name_id_t level_name = 0;
    precedence_t precedence;
  };

  struct result_t {
    optional_t<result_oper_t<oper_prefix_t>> prefix;
    optional_t<result_oper_t<oper_regular_t>> regular;
    bool is_right_bracket = false;
  };

  struct parse_axe_t {
    token_context_ptr_t tcp;
    hashmap_t<token_id_t, result_t> results;
    optional_t<precedence_t> concat;
  };

  struct parse_axe_nursery_t {
    token_context_ptr_t tcp;
    optional_t<pair_t<token_id_t, token_id_t>> nest;
    struct level_t {
      full_name_id_t name = 0;
      assoc_t assoc       = assoc_t::INVALID;
    };
    vector_t<level_t> levels;

    expected_t<void> start_level(full_name_id_t, assoc_t);
    expected_t<void> add_oper(oper_any_t);
    parse_axe_t finish() &&;
  };
}
