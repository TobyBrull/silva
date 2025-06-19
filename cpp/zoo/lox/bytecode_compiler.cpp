#include "bytecode_compiler.hpp"

namespace silva::lox::bytecode {
  compiler_t::compiler_t(syntax_ward_ptr_t swp) : lexicon(std::move(swp)) {}

  expected_t<chunk_t> compiler_t::compile(const parse_tree_span_t pts)
  {
    return {};
  }
}
