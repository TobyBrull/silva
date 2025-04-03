#pragma once

#include "syntax/parse_tree.hpp"

namespace silva {
  const string_view_t fern_seed = R"'(
    - Fern = '[' LabeledItem* ']'
    - LabeledItem = ( Label ':' )? (Fern | Value)
    - Label = string | identifier
    - Value = 'none' | 'true' | 'false' | string | number
  )'";

  // Invariant:
  //    fern_seed_engine()->apply(tokenization, "Fern") == fern_parse(tokenization)
  expected_t<unique_ptr_t<parse_tree_t>> fern_parse(shared_ptr_t<const tokenization_t>);
  unique_ptr_t<seed_engine_t> fern_seed_engine(token_context_ptr_t);

  // Fern parse_tree output functions
  expected_t<string_t> fern_to_string(const parse_tree_t*, index_t start_node = 0);
  expected_t<string_t> fern_to_graphviz(const parse_tree_t*, index_t start_node = 0);

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
    vector_t<fern_item_t> items;
    hashmap_t<string_t, index_t> labels;

    void push_back(fern_labeled_item_t&&);
    string_t to_string(int indent = 0) const;
    string_t to_graphviz() const;
  };
  expected_t<fern_t> fern_create(const parse_tree_t*, index_t start_node = 0);
}
