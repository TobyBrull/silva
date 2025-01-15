#pragma once

#include "canopy/types.hpp"
#include "tokenization.hpp"

namespace silva {
  struct shunting_yard_t {
    using level_index_t = index_t;

    enum class level_type_t {
      NONE,
      BINARY_LEFT_TO_RIGHT,
      BINARY_RIGHT_TO_LEFT,
      PREFIX,
      POSTFIX,
    };

    struct level_t {
      level_type_t type = level_type_t::NONE;
      vector_t<token_id_t> token_ids;
    };
    vector_t<level_t> levels;

    struct mapped_levels_t {
      optional_t<level_index_t> postfix_or_binary;
      optional_t<level_index_t> prefix;
    };
    hashmap_t<token_id_t, mapped_levels_t> mapped_levels;

    bool has_operator(token_id_t) const;
    optional_t<level_index_t>& slot_for(token_id_t, level_type_t);

    level_index_t add_level(level_type_t);
    void add_operator(level_index_t, token_id_t);
  };

  struct Expression {
    string_t repr;
    friend auto operator<=>(const Expression&, const Expression&) = default;
  };
  struct shunting_yard_run_t {
    const shunting_yard_t* shunting_yard = nullptr;
    std::function<Expression(span_t<const Expression>, token_id_t, shunting_yard_t::level_index_t)>
        callback;

    expected_t<void> push_back(Expression);
    expected_t<void> push_back(token_id_t);
    struct op_t {
      token_id_t token_id = 0;
      bool is_prefix      = false;

      friend auto operator<=>(const op_t&, const op_t&) = default;
    };
    using item_t = variant_t<Expression, op_t>;
    vector_t<item_t> items;
    // State transitions: (transitions to error if event not listed)
    //  - PreExpr:
    //      - prefix-op -> unchanged
    //      - expr -> PostExpr
    //  - PostExpr:
    //      - postfix-op -> unchanged
    //      - binary-op -> PreExpr
    //      - finish -> done
    enum class shunting_yard_run_state_t {
      PRE_EXPR,
      POST_EXPR,
    };
    shunting_yard_run_state_t state = shunting_yard_run_state_t ::PRE_EXPR;

    expected_t<Expression> finish();
    expected_t<bool> apply_next(shunting_yard_t::level_index_t);
  };
}
