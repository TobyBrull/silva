#pragma once

#include "seed_engine.hpp"

namespace silva {
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

  unique_ptr_t<seed::seed_engine_t> standard_seed_engine(syntax_ward_ptr_t);
}
