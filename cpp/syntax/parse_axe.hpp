#pragma once

#include "canopy/delegate.hpp"
#include "canopy/types.hpp"
#include "parse_tree_nursery.hpp"
#include "tokenization.hpp"

// * https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

// An mechanism for parsing [a]rithmetic e[x]pr[e]ssions. This is a version of the Shunting Yard
// algorithm.

namespace silva::parse_axe {
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

    friend auto operator<=>(const prefix_nest_t&, const prefix_nest_t&) = default;
  };

  struct atom_nest_t {
    constexpr static inline index_t arity = -1;
    token_id_t left_bracket{0};
    token_id_t right_bracket{0};

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
  constexpr static inline precedence_t precedence_max{
      .level_index = std::numeric_limits<level_index_t>::max(),
      .assoc       = assoc_t::INVALID,
  };
  constexpr static inline precedence_t precedence_min{
      .level_index = std::numeric_limits<level_index_t>::min(),
      .assoc       = assoc_t::INVALID,
  };

  template<typename Oper>
  struct result_oper_t {
    Oper oper;
    name_id_t name = 0;
    precedence_t precedence;

    friend auto operator<=>(const result_oper_t<Oper>&, const result_oper_t<Oper>&) = default;
  };

  struct parse_axe_result_t {
    optional_t<result_oper_t<oper_prefix_t>> prefix;
    optional_t<result_oper_t<oper_regular_t>> regular;
    bool is_right_bracket = false;

    friend auto operator<=>(const parse_axe_result_t&, const parse_axe_result_t&) = default;
  };

  struct parse_axe_t {
    token_context_ptr_t tcp;
    hashmap_t<token_id_t, parse_axe_result_t> results;
    optional_t<result_oper_t<oper_regular_t>> concat_result;

    expected_t<parse_tree_nursery_t::proto_node_t>
    apply(parse_tree_nursery_t&,
          name_id_t atom_name_id,
          delegate_t<expected_t<parse_tree_nursery_t::proto_node_t>()> atom) const;
  };

  struct parse_axe_level_desc_t {
    name_id_t name = 0;
    assoc_t assoc  = assoc_t::INVALID;
    vector_t<oper_any_t> opers;
  };
  expected_t<parse_axe_t> parse_axe_create(token_context_ptr_t,
                                           const vector_t<parse_axe_level_desc_t>&);
}
