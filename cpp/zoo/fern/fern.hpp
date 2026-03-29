#pragma once

#include "syntax/parse_tree.hpp"

namespace silva::fern {
  const string_view_t seed_str = R"'(
    - Fern = tokenizer [
      - include tokenizer FreeForm
      - operator = [ OPERATOR PARENTHESIS ]
      - identifier = IDENTIFIER
    ]
    - Fern = [
      - x = '[' LabeledItem * ']'
      - LabeledItem = ( Label ':' ) ? ( x | Value )
      - Label = string | identifier
      - Value = 'none' | 'true' | 'false' | string | number
    ]
)'";

  // Object-oriented interface
  struct fern_t;
  struct fern_item_t {
    variant_t<none_t, bool, string_t, double, unique_ptr_t<fern_t>> value;
    fern_item_t();
  };
  struct fern_labeled_item_t {
    optional_t<string_t> label;
    fern_item_t item;
  };
  struct fern_t {
    array_t<fern_item_t> items;
    hash_map_t<string_t, index_t> labels;

    void push_back(fern_labeled_item_t&&);
    string_t to_string(int indent = 0) const;
    string_t to_graphviz() const;
  };
  expected_t<fern_t> create(const parse_tree_t*, index_t start_node = 0);
}
