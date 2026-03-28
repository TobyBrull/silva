#pragma once

#include "syntax/fragmentization.hpp"

namespace silva::seed::impl {
  enum class case_mask_t {
    EMPTY       = 0,
    SILVA_CASE  = 0b0000001, // 'hello-world'
    SNAKE_CASE  = 0b0000010, // 'hello_world'
    CAMEL_CASE  = 0b0000100, // 'helloWorld'
    PASCAL_CASE = 0b0001000, // 'HelloWorld'
    MACRO_CASE  = 0b0010000, // 'HELLO_WORLD'
    UPPER_CASE  = 0b0100000, // EVERY character is upper-case, no '_' '-'
    LOWER_CASE  = 0b1000000, // EVERY character is lower-case, no '_' '-'
    ANY         = ~EMPTY,
  };
  expected_t<case_mask_t> compute_case_mask(string_view_t);

  struct matcher_t {
    fragment_category_t category = fragment_category_t::INVALID;
    case_mask_t case_mask        = case_mask_t::ANY;
    string_t prefix              = "";
    string_t postfix             = "";

    expected_t<bool> matches(index_t fragment_idx, const fragmentization_t&) const;

    friend auto operator<=>(const matcher_t&, const matcher_t&) = default;
  };

  // If token_name == token_id_none, this is an 'ignore' rule.
  // If prefix_matchers and repeat_matchers are empty, this is an include rule.
  struct rule_t {
    token_id_t token_name = token_id_none;

    array_t<matcher_t> prefix_matchers;
    array_t<matcher_t> repeat_matchers;

    friend auto operator<=>(const rule_t&, const rule_t&) = default;
  };
}
