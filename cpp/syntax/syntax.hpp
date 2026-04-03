#pragma once

#include "seed_interpreter.hpp"

namespace silva {
  using parser_t = silva::function_t<expected_t<parse_tree_ptr_t>(fragment_span_t, name_id_t)>;

  parser_t as_parser(seed::interpreter_t*);

  const string_view_t seed_str = R"'(
    - Silva = tokenizer [
      - language_name = IDENTIFIER
      - include tokenizer FreeForm
    ]
    - Silva = [
      - x = Section * end_of_file
      - Section = language_name language
    ]
)'";

  expected_t<name_id_t> infer_goal_rule_name(syntax_farm_t&, const filepath_t&);

  unique_ptr_t<seed::interpreter_t> standard_seed_interpreter(syntax_farm_ptr_t);
}
