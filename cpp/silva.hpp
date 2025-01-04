#pragma once

#include "parse_tree.hpp"

namespace silva {
  const source_code_t silva_seed_source_code("silva.seed", R"'(
    - Silva = "import" major_error Imports "interface" Interface "implementation" Implementation "end"

    - Imports = ImportFilename*
    - ImportFilename = string

    - Interface = InterfaceCmd*
    - InterfaceCmd,0 = "namespace" major_error NestedNamespace
    - InterfaceCmd,1 = "type" major_error TypeDefn
    - InterfaceCmd,2 = "func" major_error FuncDecl

    - Implementation = ImplementationCmd*
    - ImplementationCmd,0 = "namespace" major_error NestedNamespace
    - ImplementationCmd,1 = "type" major_error TypeDefn
    - ImplementationCmd,2 = "func" major_error FuncDefn

    - FuncDecl = Label ":" FuncProto
    - FuncDefn,0 = Label ":" "-*-" FuncBody
    - FuncDefn,1 = Label ":" FuncProto FuncBody
    - Label = identifier

    - TypeDefn = TypeLabel ":" Type
    - Type,0 = "mutable"? identifier "*"
    - Type,1 = identifier
    - Type,2 = "[" Member* "]"
    - Member = ( Label ":" )? Type
    - TypeLabel = identifier_regex("_t$")

    - NestedNamespace = Namespace ( "." Namespace )*
    - Namespace = identifier

    - FuncProto = Type? RetvalType?
    - RetvalType = "->" Type

    - FuncBody = "{" Stmt* "}"

    - Soil = "[" Expr* "]"
    - Toil = "{" Stmt* "}"

    - Stmt,0 = Expr ";"
    - Stmt,1 = Toil
    - Stmt,2 = "if" major_error "(" Expr ")" Toil ( "else" Toil )?
    - Stmt,3 = "loop" major_error Toil

    - PrimaryExpr,0 = "(" Expr ")"
    - PrimaryExpr,1 = Soil
    - PrimaryExpr,2 = { identifier string number }
    - PrimaryExpr,3 = ExprOp

    - ExprOpInv = { ";" "(" ")" "[" "]" "{" "}" }
    - ExprOp = ( ExprOpInv! operator )

    - Expr = PrimaryExpr+

    - Name = identifier
    - Literal = { string number }
  )'");

  const parse_root_t* silva_parse_root();
}
