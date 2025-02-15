#pragma once

#include "parse_tree.hpp"

namespace silva {
  const string_view_t silva_seed = R"'(
    - Silva = "import" major_error Imports "interface" Interface "implementation" Implementation "end"

    - Imports = ImportFilename*
    - ImportFilename = string

    - Interface = Command*
    - Implementation = Command*

    - Command,0 = "namespace" major_error NestedNamespace
    - Command,1 => SoilDefn,0
    - Command,2 => ToilDefn,0

    - NestedNamespace = Namespace ( "." Namespace )*
    - Namespace = identifier

    - SoilDefn = "soil" major_error SoilLabel ":" SoilBody
    - ToilDefn = "toil" major_error ToilLabel ":" ToilHeader ToilBody

    - SoilLabel = identifier_regex("_s$")
    - ToilLabel = identifier_regex("_t$")

    - ToilHeader,0 = "-*-"
    - ToilHeader,1 = Type ("->" Type)?

    - ToilBody,0 = "proto"
    - ToilBody,1 = Toil

    - SoilBody = "[" (Member ","?)* "]"
    - Member = (MemberLabel ":")? Type
    - MemberLabel = identifier

    - Type,0 => SoilLabel,0
    - Type,1 = "*" SoilLabel
    - Type,2 => SoilBody,0

    - Soil = "[" (Expr ","?)* "]"     # could also be called SoilValue
    - Toil = "{" (Stmt ";"?)* "}"     # could also be called ToilValue

    - Stmt,0 => Toil,0
    - Stmt,1 => StmtLoop,0
    - Stmt,2 => StmtIf,0
    - Stmt,3 => Expr,0

    - StmtLoop = "loop" major_error Toil
    - StmtIf = "if" major_error "(" Expr ")" Toil ( "else" ElseBranch )?
    - ElseBranch,0 => Toil,0
    - ElseBranch,1 => StmtIf,0

    - PrimaryExpr,0 = "(" Expr ")"
    - PrimaryExpr,1 => Soil,0
    - PrimaryExpr,2 =~ identifier string number
    - PrimaryExpr,3 = ( ExprOpInv! operator )

    - ExprOpInv =~ ";" "(" ")" "[" "]" "{" "}" "," "|"

    - Expr = PrimaryExpr+
  )'";

  unique_ptr_t<parse_root_t> silva_parse_root(token_context_ptr_t);
}
