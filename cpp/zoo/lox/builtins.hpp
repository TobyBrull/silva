#pragma once

#include "object.hpp"

#include "syntax/syntax.hpp"

namespace silva::lox {
  // The parser needs to be able to parse Lox.
  expected_t<hash_map_t<token_id_t, object_ref_t>>
  make_builtins(syntax_ward_ptr_t, const parser_t&, object_pool_t&);
}
