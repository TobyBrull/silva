#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::lox {
  const string_view_t seed_str = R"'(
    - Lox = tokenizer [
      - include tokenizer FreeForm
      - identifier = IDENTIFIER
      - operator = PARENTHESIS
      - operator = ::: OPERATOR
    ]
    - Lox = [
      - x = ( Decl | Stmt ) *
      - Decl = [
        - x = Var | Fun | Class
        - Var = 'var' identifier ( '=' .Lox.Expr ) ? ';'
        - Fun = 'fun' .Lox.Function
        - Class = [
          - x = 'class' identifier Super '{' .Lox.Function * '}'
          - Super = ( '<' identifier ) ?
        ]
      ]
      - Stmt = [
        - x = Print | If | For | While | Return | Block | ExprStmt
        - Print = 'print' .Lox.Expr ';'
        - If = 'if' '(' .Lox.Expr ')' x ( 'else' x ) ?
        - For = 'for' '('
                ( .Lox.Decl.Var | ExprStmt | .None ';' )
                ( .Lox.Expr | .None ) ';'
                ( .Lox.Expr  | .None )
              ')' x
        - While = 'while' '(' .Lox.Expr ')' x
        - Return = 'return' .Lox.Expr ? ';'
        - Block = '{' ( .Lox.Decl | x ) * '}'
        - ExprStmt = .Lox.Expr ';'
      ]
      - Expr = [
        - x = axe x.Expr.Atom [
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
               | number | string
               | 'super' '.' identifier | identifier
        - Arguments = ( x ( ',' x ) * ) ?
      ]
      - Function = [
        - x = identifier '(' Parameters ')' .Lox.Stmt.Block
        - Parameters = ( Parameter ( ',' Parameter ) * ) ?
        - Parameter = identifier
      ]
    ]
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
