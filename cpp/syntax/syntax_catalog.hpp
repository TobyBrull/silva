#pragma once

#include "parse_tree.hpp"

namespace silva {
  struct syntax_catalog_t : public token_catalog_t {
    vector_t<unique_ptr_t<const tokenization_t>> tokenizations;
    vector_t<unique_ptr_t<const parse_tree_t>> parse_trees;

    token_catalog_t& token_catalog();

    tokenization_ptr_t add(unique_ptr_t<const tokenization_t>);
    parse_tree_ptr_t add(unique_ptr_t<const parse_tree_t>);
  };
  using syntax_catalog_ptr_t = ptr_t<syntax_catalog_t>;
}
