#pragma once

#include "parse_tree.hpp"

namespace silva {
  const source_code_t silva_seed_source_code("silva.seed", R"'(
    - File = Command*
    - Command,0 = "import" string
    - Command,1 = "namespace" NestedNamespace
    - Command,2 = "implementation"
    - Command,3 = "type" Label ":" Type
    - Command,4 = "func" Label ":" Func

    - NestedNamespace = Namespace ( "." Namespace )*
    - Namespace = identifier

    - Type,0 = "mutable"? identifier "*"
    - Type,1 = identifier
    - Type,2 = "[" Member* "]"
    - Member = ( Label ":" )? Type
    - Label = identifier

    - Func = Type? ("->" Type)? FuncBody?
    - FuncBody = "{" Statement* "}"
  )'");

  const parse_root_t* silva_parse_root();
}
