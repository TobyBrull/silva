#pragma once

#include "canopy/types.hpp"

namespace silva::lox {
  const string_view_t seed_str = R"'(
    - Lox = [
      - x = ( Decl | Stmt ) *
      - Decl = [
        - x = Class | Fun | Var
        - Class = 'class' identifier ( '<' identifier ) ? '{' _.Lox.Function * '}'
        - Fun = 'fun' _.Lox.Function
        - Var = 'var' identifier ( '=' _.Lox.Expr ) ? ';'
      ]
      - Stmt = [
        - x = For | If | Print | Return | While | Block | ExprStmt
        - ExprStmt = _.Lox.Expr ';'
        - For = 'for' '(' ( _.Lox.Decl.Var | ExprStmt | ';' ) _.Lox.Expr ? ';' _.Lox.Expr ? ')' x
        - If = 'if' '(' _.Lox.Expr ')' x ( 'else' x ) ?
        - Print = 'print' _.Lox.Expr ';'
        - Return = 'return' _.Lox.Expr ? ';'
        - While = 'while' '(' _.Lox.Expr ')' x
        - Block = '{' ( _.Lox.Decl | x ) * '}'
      ]
      - Expr = [
        - x =/ x.Expr.Atom [
          - Primary     = nest atom_nest '(' ')'
          - Call        = ltr postfix_nest -> Arguments '(' ')' infix '.'
          - Unary       = rtl prefix '!' '-'
          - Factor      = ltr infix '*' '/'
          - Term        = ltr infix '+' '-'
          - Comparison  = ltr infix '<' '>' '<=' '>='
          - Equality    = ltr infix '==' '!='
          - LogicAnd    = ltr infix 'and'
          - LogicOr     = ltr infix 'or'
          - Assign      = ltr infix '='
        ]
        - Atom = 'true' | 'false' | 'none' | 'this'
               | number | string | identifier
        - Arguments = ( x ( ',' x ) * ) ?
      ]
      - Function = [
        - x = identifier '(' Parameters ? ')' _.Lox.Stmt.Block
        - Parameters = ( Parameter ( ',' Parameter ) * ) ?
        - Parameter = identifier
      ]
    ]
  )'";
}
