#include "syntax_catalog.hpp"

namespace silva {
  token_catalog_t& syntax_catalog_t::token_catalog()
  {
    return *this;
  }

  tokenization_ptr_t syntax_catalog_t::add(unique_ptr_t<const tokenization_t> x)
  {
    tokenizations.push_back(std::move(x));
    return tokenizations.back()->ptr();
  }

  parse_tree_ptr_t syntax_catalog_t::add(unique_ptr_t<const parse_tree_t> x)
  {
    parse_trees.push_back(std::move(x));
    return parse_trees.back()->ptr();
  }
}
