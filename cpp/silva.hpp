#pragma once

#include "parse_tree.hpp"

namespace silva {
  const source_code_t silva_seed_source_code("silva.seed", R"'(
    - Silva = "import" Imports "interface" Interface "implementation" Implementation "end"

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
    - Label = identifier

    - TypeDefn = "type" TypeLabel ":" Type
    - Type,0 = "mutable"? identifier "*"
    - Type,1 = identifier
    - Type,2 = "[" Member* "]"
    - Member = ( Label ":" )? Type
    - TypeLabel = identifier_regex("_t$")

    - NestedNamespace = Namespace ( "." Namespace )*
    - Namespace = identifier

    - FuncProto = Type? RetvalType?
    - RetvalType = "->" Type

    - FuncBody = "{" Statement* "}"

    - Statement,0 = Expr ";"
    - Statement,1 = Assignment ";"

    - Assignment = Name "=" Expr

    - PrimaryExpr,0 = "(" Expr ")"
    - PrimaryExpr,1 = "[" Expr* "]"
    - PrimaryExpr,2 = Name
    - PrimaryExpr,3 = Literal
    - PrimaryExpr,4 = "@" Expr Expr

    - Expr,0 = PrimaryExpr ( ExprOps PrimaryExpr )*
    - ExprOpsInv = { ";" "(" ")" "[" "]" "=" }
    - ExprOps = ( ExprOpsInv! operator )

    - Name = identifier
    - Literal = { string number }
  )'");

  const parse_root_t* silva_parse_root();
}
