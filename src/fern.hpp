#pragma once

#include "parse_tree.hpp"

namespace silva {
  const source_code_t fern_seed_source_code{
      .filename = "fern.seed",
      .text     = R"'(
        - Fern = "[" LabeledItem* "]"
        - LabeledItem = ( Label ":" )? Item ";"?
        - Label = { string identifier }
        - Item,0 = Fern
        - Item,1 = { "none" "true" "false" string number }
      )'",
  };

  enum class fern_rule_t {
    FERN,
    LABELED_ITEM,
    LABEL,
    ITEM_0,
    ITEM_1,
  };

  const parse_root_t* fern_parse_root();

  expected_t<parse_tree_t> fern_parse(const tokenization_t*);

  // Fern parse_tree output functions

  std::string
  fern_to_string(const parse_tree_t*, index_t start_node = 0, bool with_semicolon = true);

  std::string fern_to_graphviz(const parse_tree_t*, index_t start_node = 0);

  // Object-oriented interface

  struct fern_t;
  struct item_t {
    std::variant<std::nullopt_t, bool, std::string, double, std::unique_ptr<fern_t>> value;

    item_t();
  };

  struct labeled_item_t {
    std::optional<std::string> label;
    item_t item;
  };

  struct fern_t {
    std::vector<item_t> items;
    std::unordered_map<std::string, index_t> labels;

    void push_back(labeled_item_t&&);

    std::string to_str_fern(int indent = 0) const;
    std::string to_str_graphviz() const;
  };

  fern_t fern_create(const parse_tree_t*, index_t start_node = 0);
}
