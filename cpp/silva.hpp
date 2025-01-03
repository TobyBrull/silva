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

    - Stmt,0 = ExprStmt
    - Stmt,1 = CompStmt
    - Stmt,2 = CondStmt
    - Stmt,3 = LoopStmt

    - ExprStmt = Expr ";"
    - CompStmt = "{" Stmt* "}"
    - CondStmt = "if" major_error "(" Expr ")" "{" Stmt* "}" ( "else" "{" Stmt* "}" )?
    - LoopStmt = "loop" major_error "{" Stmt* "}"

    - PrimaryExpr,0 = "(" Expr ")"
    - PrimaryExpr,1 = "[" Expr* "]"
    - PrimaryExpr,2 = Name
    - PrimaryExpr,3 = Literal
    - PrimaryExpr,4 = ExprOp

    - ExprOpInv = { ";" "(" ")" "[" "]" "{" "}" }
    - ExprOp = ( ExprOpInv! operator )

    - Expr = PrimaryExpr+

    - Name = identifier
    - Literal = { string number }
  )'");

  const parse_root_t* silva_parse_root();
}
