#pragma once

#include "parse_tree.hpp"

namespace silva {
  const source_code_t fern_seed_source_code("fern.seed", R"'(
    - Fern = "[" LabeledItem* "]"
    - LabeledItem = ( Label ":" )? Item ";"?
    - Label = { string identifier }
    - Item,0 = Fern
    - Item,1 = { "none" "true" "false" string number }
  )'");

  enum class fern_rule_t {
    FERN,
    LABELED_ITEM,
    LABEL,
    ITEM_0,
    ITEM_1,
  };

  const parse_root_t* fern_parse_root();

  expected_t<parse_tree_t> fern_parse(const_ptr_t<tokenization_t>);

  // Fern parse_tree output functions

  string_t fern_to_string(const parse_tree_t* fern_parse_tree,
                          index_t start_node  = 0,
                          bool with_semicolon = true);

  string_t fern_to_graphviz(const parse_tree_t* fern_parse_tree, index_t start_node = 0);

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

  fern_t fern_create(const parse_tree_t*, index_t start_node = 0);
}
