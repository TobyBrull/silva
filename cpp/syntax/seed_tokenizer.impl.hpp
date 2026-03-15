#pragma once

#include "syntax/fragmentization.hpp"
#include "syntax/parse_tree.hpp"

namespace silva::seed::impl {
  struct matcher_t {
    fragment_category_t category   = fragment_category_t::INVALID;
    fragment_case_mask_t case_mask = fragment_case_mask_t::ANY;
    string_view_t prefix           = "";
    string_view_t postfix          = "";
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
