#pragma once

#include "fragmentization.hpp"
#include "syntax_ward.hpp"

namespace silva::seed {
  struct tokenizer_t {
    struct rule_item_t {};

    struct rule_tokenizer_t {
      token_id_t included_tokenizer = token_id_none;
    };
    struct rule_ignore_t {
      array_t<rule_item_t> rule_items;
    };
    struct rule_token_t {
      token_id_t defined_token = token_id_none;
      array_t<rule_item_t> rule_items;
    };
    using rule_t = variant_t<rule_tokenizer_t, rule_ignore_t, rule_token_t>;

    array_t<rule_t> rules;
  };

  expected_t<tokenization_ptr_t>
  tokenize(syntax_ward_ptr_t, const tokenizer_t&, const fragmentization_t&);
}

// IMPLEMENTATION

namespace silva::seed {
}
