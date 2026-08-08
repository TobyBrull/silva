#pragma once

#include "canopy/types.hpp"

#include "lox.lexicon.hpp"
#include "syntax/seed_interpreter.hpp"

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
    ⊙ = axe Atom oper
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
    oper = "and" | "or" | '<=' | '>=' | '==' | '!=' | operator.single | parenthesis
    Arguments = ( Expr ( ',' Expr ) * ) ?
  Function:
    ⊙ = identifier '(' Parameters ')' Stmt.Block
    Parameters = ( Parameter ( ',' Parameter ) * ) ?
    Parameter = identifier
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);

  // The token-id of the "identifier" that is the given child (0-based, counting children) of the
  // given node.
  expected_t<token_id_t> child_token(const parse_tree_span_t&, index_t child_index = 0);

  // Whether the given node covers any fragments at all. Used to detect empty optional sub-rules.
  bool is_non_empty(const parse_tree_span_t&);

  // Describes the leading token of an "Expr.Atom" node.
  struct atom_token_t {
    // The token-id of the leading token of the atom.
    token_id_t token_id;

    // One of "identifier", "number", "string", or "literal" (for keywords and '(').
    name_id_t category;

    // Only for the "super" '.' identifier form: the token-id of the member being accessed.
    token_id_t member_token_id;
  };
  expected_t<atom_token_t> atom_token(const parse_tree_span_t&, const lexicon_t&);
}
