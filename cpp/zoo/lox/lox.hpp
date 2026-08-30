#pragma once

#include "lox.lexicon.hpp"

#include "syntax/seed_interpreter.hpp"

#include "canopy/types.hpp"

namespace silva::lox {
  const string_view_t seed_str = R"'(
language Lox:
  skip = skip.freeForm

  ⊙ = ( Decl | Stmt ) *
  Decl:
    ⊙ = Var | Fun | Class
    Var = "var" identifier ( '=' Expr ) ? ';'
    Fun = "fun" Function
    Class:
      ⊙ = "class" identifier Super '{' Function * '}'
      Super = ( '<' identifier ) ?
  Stmt:
    ⊙ = Print | If | For | While | Return | Block | ExprStmt
    Print = "print" Expr ';'
    If = "if" '(' Expr ')' Stmt ( "else" Stmt ) ?
    For = ( "for" '('
            ( Decl.Var | ExprStmt | Epsilon ';' )
            ( Expr | Epsilon ) ';'
            ( Expr | Epsilon )
            ')' Stmt )
    While = "while" '(' Expr ')' Stmt
    Return = "return" Expr ? ';'
    Block = '{' ( Decl | Stmt ) * '}'
    ExprStmt = Expr ';'
  Expr:
    ⊙ = axe Atom
      Call        = ltr postfix_nest -> Arguments '(' ')' infix '.'
      Unary       = rtl prefix '!' '-'
      Factor      = ltr infix '*' '/'
      Term        = ltr infix '+' '-'
      Comparison  = ltr infix '<' '>' '<=' '>='
      Equality    = ltr infix '==' '!='
      LogicAnd    = ltr infix "and"
      LogicOr     = ltr infix "or"
      Assign      = ltr infix '='
    Atom = ( literal | number | string
           | "super" '.' identifier | identifier
           | '(' Expr ')' )
    literal = "true" | "false" | "nil" | "this"
    Arguments = ( Expr ( ',' Expr ) * ) ?
  Function:
    ⊙ = identifier '(' Parameters ')' Stmt.Block
    Parameters = ( identifier ( ',' identifier ) * ) ?
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);

  struct atom_token_t {
    token_id_t token_id;
    name_id_t category;
    // The "identifier" in the « = "super" '.' identifier » rule
    token_id_t member_token_id;
  };
  expected_t<atom_token_t> atom_token(const parse_tree_span_t&, const lexicon_t&);
}
