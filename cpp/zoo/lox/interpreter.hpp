#pragma once

#include "lox.hpp"
#include "object.hpp"

#include "syntax/parse_tree.hpp"
#include "syntax/syntax.hpp"

namespace silva::lox {
  template<typename T>
  using return_t = optional_t<T>;

  struct interpreter_t : public menhir_t {
    lexicon_t lexicon;

    object_pool_t pool;
    cactus_t<token_id_t, object_ref_t> scopes;

    hashmap_t<parse_tree_span_t, object_ref_t> literals;
    hashmap_t<parse_tree_span_t, index_t> resolution;

    interpreter_t(syntax_ward_ptr_t);
    ~interpreter_t();

    // The parser needs to be able to parse Lox.
    expected_t<void> load_builtins(const parser_t&);

    expected_t<void> resolve(parse_tree_span_t);

    expected_t<object_ref_t> evaluate(parse_tree_span_t, scope_ptr_t);
    expected_t<return_t<object_ref_t>> execute(parse_tree_span_t, scope_ptr_t&);
  };
}
