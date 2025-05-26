#pragma once

#include "canopy/types.hpp"

namespace silva::lox {
  const string_view_t seed_str = R"'(
    - Lox = [
      - x = ( Decl | Stmt ) *
      - Decl = [
        - x = Var | Fun | Class
        - Var = 'var' identifier ( '=' p.Expr ) ? ';'
        - Fun = 'fun' Function
        - Class = [
          - x = 'class' identifier Super '{' p.Function * '}'
          - Super = ( '<' identifier ) ?
        ]
        - Function = [
          - x = identifier '(' Parameters ')' _.Lox.Stmt.Block
          - Parameters = ( Parameter ( ',' Parameter ) * ) ?
          - Parameter = identifier
        ]
      ]
      - Stmt = [
        - x = Print | If | For | While | Return | Block | ExprStmt
        - Print = 'print' p.Expr ';'
        - If = 'if' '(' p.Expr ')' x ( 'else' x ) ?
        - For = 'for' '('
                ( p.Decl.Var | ExprStmt | _.None ';' )
                ( p.Expr | _.None ) ';'
                ( p.Expr  | _.None )
              ')' x
        - While = 'while' '(' p.Expr ')' x
        - Return = 'return' p.Expr ? ';'
        - Block = '{' ( p.Decl | x ) * '}'
        - ExprStmt = p.Expr ';'
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
