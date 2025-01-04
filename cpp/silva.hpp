#pragma once

#include "parse_tree.hpp"

namespace silva {
  const source_code_t silva_seed_source_code("silva.seed", R"'(
    - Silva = "import" major_error Imports "interface" Interface "implementation" Implementation "end"

    - Imports = ImportFilename*
    - ImportFilename = string

    - Interface = Command*
    - Implementation = Command*

    - Command,0 = "namespace" major_error NestedNamespace
    - Command,1 = SoilDefn
    - Command,2 = ToilDefn

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

    - Type,0 = SoilLabel
    - Type,1 = "*" SoilLabel
    - Type,2 = SoilBody

    - Soil = "[" (Expr ","?)* "]"     # could also be called SoilValue
    - Toil = "{" (Stmt ";"?)* "}"     # could also be called ToilValue

    - Stmt,0 = Toil
    - Stmt,1 = StmtLoop
    - Stmt,2 = StmtIf
    - Stmt,3 = Expr

    - StmtLoop = "loop" major_error Toil
    - StmtIf = "if" major_error "(" Expr ")" Toil ( "else" ElseBranch )?
    - ElseBranch,0 = Toil
    - ElseBranch,1 = StmtIf

    - PrimaryExpr,0 = "(" Expr ")"
    - PrimaryExpr,1 = Soil
    - PrimaryExpr,2 = { identifier string number }
    - PrimaryExpr,3 = ( ExprOpInv! operator )

    - ExprOpInv = { ";" "(" ")" "[" "]" "{" "}" }

    - Expr = PrimaryExpr+
  )'");

  const parse_root_t* silva_parse_root();
}
