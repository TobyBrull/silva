#pragma once

#include "canopy/types.hpp"
#include "tokenization.hpp"

// * https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

// The Parse Axe algorithm is meant to parse arithmetic expressions. An arithmetic expression is
// defined to be a string of Items with each Item being either an Operator (Op) or an Atom; also,
// the string is assumed to contain at least one Atom.
//
// For example, if singular, upper-case characters A-Z represent Atoms and all other tokens (like
// '+', '-', ..., 'if', 'then', 'else') represent operators, the following would be examples of
// arithmetic expressions.
//
//    A + B * C + D
//    + - A * B ! +
//    A . B . C
//    A B C
//    A + B C + D
//    A ? B : C ? D : E
//    A = B ? C : D = E
//    if A then B else C
//    A if B else C
//    A , B , C
//    A + B = C ( D , E )
//    ) ( A ) (
//    ( ( ( A ) ) )
//    - + A
//    A + + - B
//    - - A * B
//    - - A . B
//    A [ B ]
//    A [ B , C ]
//    A [ B ] [ C ]
//    A * ( B + C ) * D
//    - A !
//    A . B !
//    ( A + B + C )
//    ( ( A + B ) + C)
//
// The Parse Axe algorithm assumes that the distinction into what is an Atom and what is an Op has
// already been made somewhere else. For example, the source-code corresponding to an Atom could
// contain the '+' character or parenthesised expression in the source-code could be handled as
// Atoms.
//
// The Parse Axe algorithm can be configured by specifying an ordered list of precedence levels,
// with each precedence level having type either PREFIX, POSTFIX, BINARY_LTR, or BINARY_RTL. This is
// called the Config.
//
// Given a Config, the intent of the Parse Axe algorithm is to create a ParseTree from a string of
// Items, if possible. This is conceptually done by working through the list of precedence levels
// (from high to low) and joining (i.e., creating sub-expressions) at each step. If the Config uses
// parenthes in the usual way, and depending on the rest of the Config, some of the expressions
// could be parsed as follows.
//
//  ( A + ( B * C ) + D )
//  ( + ( - A ) ) * ( ( B ! ) + )
//
// If an Op would be allowed to be used as PREFIX, POSTFIX, and BINARY (_LTR or _RTL), this could be
// confusing. For example, assume '+' would be such an operator and consider the expression
//
//    A + + B + + C
//
// If the PREFIX-'+' or POSTFIX-'+' would have highest precedence, this would surely be equivalent
// to the following, respectively.
//
//    ( A + ( + B ) ) + ( + C )
//    ( ( A + ) + ( B + ) ) + C
//
// However, it is not clear what would happen if BINARY-'+' would have highest precedence, so this
// is not allowed.

namespace silva {
  struct parse_axe_t {
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
  struct parse_axe_run_t {
    const parse_axe_t* parse_axe = nullptr;
    std::function<Expression(span_t<const Expression>, token_id_t, parse_axe_t::level_index_t)>
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
    enum class parse_axe_run_state_t {
      PRE_EXPR,
      POST_EXPR,
    };
    parse_axe_run_state_t state = parse_axe_run_state_t ::PRE_EXPR;

    expected_t<Expression> finish();
    expected_t<bool> apply_next(parse_axe_t::level_index_t);
  };
}
