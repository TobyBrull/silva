#include "syntax_ward.hpp"

#include "parse_tree.hpp"

namespace silva {

  syntax_ward_t::syntax_ward_t()  = default;
  syntax_ward_t::~syntax_ward_t() = default;

  token_ward_t& syntax_ward_t::token_ward()
  {
    return *this;
  }

  tokenization_ptr_t syntax_ward_t::add(unique_ptr_t<const tokenization_t> x)
  {
    tokenizations.push_back(std::move(x));
    return tokenizations.back()->ptr();
  }

  parse_tree_ptr_t syntax_ward_t::add(unique_ptr_t<const parse_tree_t> x)
  {
    parse_trees.push_back(std::move(x));
    return parse_trees.back()->ptr();
  }
}
