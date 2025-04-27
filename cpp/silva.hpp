#pragma once

#include "syntax/parse_tree.hpp"

namespace silva {
  const string_view_t silva_seed = R"'(
    - Silva = [
      - x = Imports Interface Impls end_of_file
      - Imports = 'import' Module *
      - Interface = 'interface' string *
      - Impls = Impl *
      - Impl = 'impl' ':' _.Seed.Nonterminal -> nt_v parse_f(_, nt_v)
      - Module = [
        - x = Base ( '.' Base ) *
        - Base = '_' | 'x' | 'p' | identifier / '^[a-z]'
      ]
    ]
  )'";
}
