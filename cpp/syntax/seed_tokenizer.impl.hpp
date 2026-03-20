#pragma once

#include "syntax/fragmentization.hpp"
#include "syntax/parse_tree.hpp"

namespace silva::seed::impl {
  enum class case_mask_t {
    INVALID     = 0,
    SILVA_CASE  = 0b0000001, // 'hello-world'
    SNAKE_CASE  = 0b0000010, // 'hello_world'
    CAMEL_CASE  = 0b0000100, // 'helloWorld'
    PASCAL_CASE = 0b0001000, // 'HelloWorld'
    MACRO_CASE  = 0b0010000,
    UPPER_CASE  = 0b0100000, // EVERY character is upper-case, no '_' '-'
    LOWER_CASE  = 0b1000000, // EVERY character is lower-case, no '_' '-'
  };

  struct matcher_t {
    fragment_category_t category = fragment_category_t::INVALID;
    case_mask_t case_mask        = case_mask_t::INVALID;
    string_t prefix              = "";
    string_t postfix             = "";
  };

  struct rule_t {
    // token_id_none means an 'ignore' rule.
    token_id_t token_name = token_id_none;

    array_t<matcher_t> prefix_matchers;
    array_t<matcher_t> repeat_matchers;
  };
}

// IMPLEMENTATION

namespace silva::seed::impl {
}
