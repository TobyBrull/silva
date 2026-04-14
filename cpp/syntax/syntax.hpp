#pragma once

#include "seed_interpreter.hpp"

namespace silva {
  using parser_t = silva::function_t<expected_t<parse_tree_ptr_t>(fragment_span_t, name_id_t)>;

  parser_t as_parser(seed::interpreter_t*);

  const string_view_t seed_str = R"'(
tokenizer Silva:
  language_name = IDENTIFIER_PASCAL_CASE
  include tokenizer FreeForm

Silva =
  ⊙ = Section * end_of_file
  Section = language_name language
)'";

  unique_ptr_t<seed::interpreter_t> standard_seed_interpreter(syntax_farm_ptr_t);
}
