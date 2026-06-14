#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::lox {
  const string_view_t seed_str = R"'(
language Lox:
  ⊙ = ( Decl | Stmt ) *
  skip = skip_free_form
  Decl:
    ⊙ = Var | Fun | Class
    Var = 'var' identifier ( '=' Expr ) ? ';'
    Fun = 'fun' Function
    Class:
      ⊙ = 'class' identifier Super '{' Function * '}'
      Super = ( '<' identifier ) ?
  Stmt:
    ⊙ = Print | If | For | While | Return | Block | ExprStmt
    Print = 'print' Expr ';'
    If = 'if' '(' Expr ')' Stmt ( 'else' Stmt ) ?
    For = ( 'for' '('
            ( Decl.Var | ExprStmt | None ';' )
            ( Expr | None ) ';'
            ( Expr | None )
            ')' Stmt )
    While = 'while' '(' Expr ')' Stmt
    Return = 'return' Expr ? ';'
    Block = '{' ( Decl | Stmt ) * '}'
    ExprStmt = Expr ';'
  Expr:
    ⊙ = axe Atom oper
      Call        = ltr postfix_nest -> Arguments '(' ')' infix '.'
      Unary       = rtl prefix '!' '-'
      Factor      = ltr infix '*' '/'
      Term        = ltr infix '+' '-'
      Comparison  = ltr infix '<' '>' '<=' '>='
      Equality    = ltr infix '==' '!='
      LogicAnd    = ltr infix 'and'
      LogicOr     = ltr infix 'or'
      Assign      = ltr infix '='
    Atom = ( 'true' | 'false' | 'nil' | 'this'
           | number | string
           | 'super' '.' identifier | identifier
           | '(' Expr ')' )
    oper = 'and' | 'or' | '<=' | '>=' | '==' | '!=' | operator_single | parenthesis
    Arguments = ( Expr ( ',' Expr ) * ) ?
  Function:
    ⊙ = identifier '(' Parameters ')' Stmt.Block
    Parameters = ( Parameter ( ',' Parameter ) * ) ?
    Parameter = identifier
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
