#pragma once

#include "syntax/parse_tree.hpp"

namespace silva {
  expected_t<name_id_t> infer_goal_rule_name(syntax_ward_t&, const filesystem_path_t&);
}
