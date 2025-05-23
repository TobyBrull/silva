#pragma once

#include "canopy/types.hpp"

namespace silva::lox {
  const string_view_t seed_str = R"'(
    - Lox = [
      - x = ( Decl | Stmt ) *
      - Decl = [
        - x = Var | Fun | Class
        - Var = 'var' identifier ( '=' _.Lox.Expr ) ? ';'
        - Fun = 'fun' _.Lox.Function
        - Class = 'class' identifier ( '<' identifier ) ? '{' _.Lox.Function * '}'
      ]
      - Function = [
        - x = identifier '(' Parameters ')' _.Lox.Stmt.Block
        - Parameters = ( Parameter ( ',' Parameter ) * ) ?
        - Parameter = identifier
      ]
      - Stmt = [
        - x = Print | If | For | While | Return | Block | ExprStmt
        - Print = 'print' _.Lox.Expr ';'
        - If = 'if' '(' _.Lox.Expr ')' x ( 'else' x ) ?
        - For = 'for' '(' ( _.Lox.Decl.Var | ExprStmt | ';' ) _.Lox.Expr ? ';' _.Lox.Expr ? ')' x
        - While = 'while' '(' _.Lox.Expr ')' x
        - Return = 'return' _.Lox.Expr ? ';'
        - Block = '{' ( _.Lox.Decl | x ) * '}'
        - ExprStmt = _.Lox.Expr ';'
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
    ]
  )'";
}
