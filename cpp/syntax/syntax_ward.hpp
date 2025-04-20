#pragma once

#include "parse_tree.hpp"

namespace silva {
  struct syntax_ward_t : public token_ward_t {
    vector_t<unique_ptr_t<const tokenization_t>> tokenizations;
    vector_t<unique_ptr_t<const parse_tree_t>> parse_trees;

    token_ward_t& token_ward();

    tokenization_ptr_t add(unique_ptr_t<const tokenization_t>);
    parse_tree_ptr_t add(unique_ptr_t<const parse_tree_t>);
  };
  using syntax_ward_ptr_t = ptr_t<syntax_ward_t>;
}
