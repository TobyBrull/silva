#pragma once

#include "parse_tree.hpp"

namespace silva {
  const source_code_t silva_seed_source_code("silva.seed", R"'(
    - Silva = "import" Imports "interface" Interface "implementation" Implementation

    - Imports = ImportFilename*
    - ImportFilename = string

    - Interface = InterfaceCmd*
    - InterfaceCmd,0 = "namespace" NestedNamespace
    - InterfaceCmd,1 = TypeDefn
    - InterfaceCmd,2 = FuncDecl

    - Implementation = ImplementationCmd*
    - ImplementationCmd,0 = "namespace" NestedNamespace
    - ImplementationCmd,1 = TypeDefn
    - ImplementationCmd,2 = FuncDefn

    - FuncDecl = "func" Label ":" FuncProto
    - FuncDefn,0 = "func" Label ":" FuncProto FuncBody
    - FuncDefn,1 = "func" Label ":" "-*-" FuncBody

    - TypeDefn = "type" Label ":" Type
    - Type,0 = "mutable"? identifier "*"
    - Type,1 = identifier
    - Type,2 = "[" Member* "]"
    - Member = ( Label ":" )? Type

    - NestedNamespace = Namespace ( "." Namespace )*
    - Namespace = identifier
    - Label = identifier

    - FuncProto = Type? RetvalType?
    - RetvalType = "->" Type

    - FuncBody = "{" (Statement ";")* "}"

    - Statement = (";"! "}"! any)+

  )'");

  const parse_root_t* silva_parse_root();
}
