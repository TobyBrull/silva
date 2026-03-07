#pragma once

#include "seed_interpreter.hpp"

namespace silva {
  using parser_t = silva::function_t<expected_t<parse_tree_ptr_t>(tokenization_ptr_t, name_id_t)>;

  parser_t as_parser(const seed::interpreter_t*);

  const string_view_t seed_str = R"'(
    - Silva = [
      - x = Section * end_of_file
      - Section = '<$'
        _.Seed.Nonterminal -> nt_v
        parse_and_callback_f(_, nt_v)
      '$>'
    ]
  )'";

  expected_t<name_id_t> infer_goal_rule_name(syntax_ward_t&, const filesystem_path_t&);

  unique_ptr_t<seed::interpreter_t> standard_seed_interpreter(syntax_ward_ptr_t);
}
