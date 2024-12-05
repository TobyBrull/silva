#pragma once

#include "parse_tree.hpp"
#include "tokenization.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>

namespace silva {
  struct fern_t;
  struct item_t {
    std::variant<std::nullopt_t, bool, std::string, double, std::unique_ptr<fern_t>> value;

    item_t();

    template<typename T>
    item_t(T&&);

    item_t(item_t&&)            = default;
    item_t& operator=(item_t&&) = default;

    item_t(const item_t&)            = delete;
    item_t& operator=(const item_t&) = delete;

    item_t copy() const;
  };

  struct labeled_item_t {
    std::optional<std::string> label;
    item_t item;
  };

  struct fern_t {
    std::vector<item_t> items;
    std::unordered_map<std::string, index_t> labels;

    fern_t();

    fern_t(fern_t&&)            = default;
    fern_t& operator=(fern_t&&) = default;

    fern_t(const fern_t&)            = delete;
    fern_t& operator=(const fern_t&) = delete;

    void push_back(labeled_item_t&&);

    fern_t copy() const;

    std::string to_str_fern(int indent = 0) const;
    std::string to_str_graphviz() const;
  };

  constexpr source_code_t fern_seed_source_code{
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

  expected_t<parse_tree_t> fern_parse(const tokenization_t*);

  const parse_root_t* fern_parse_root();

  fern_t fern_create(const parse_tree_t*, index_t start_node = 0);

  std::string
  fern_to_string(const parse_tree_t*, index_t start_node = 0, bool with_semicolon = true);

  std::string fern_to_graphviz(const parse_tree_t*, index_t start_node = 0);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  item_t::item_t(T&& x) : value(std::nullopt)
  {
    if constexpr (std::same_as<std::decay_t<T>, fern_t>) {
      value = std::make_unique<fern_t>(std::forward<T>(x));
    }
    else {
      value = std::forward<T>(x);
    }
  }
}
