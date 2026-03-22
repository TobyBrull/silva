#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::lox {
  const string_view_t seed_str = R"'(
    - Lox = [
      - x = ( Decl | Stmt ) *
      - Decl = [
        - x = Var | Fun | Class
        - Var = 'var' identifier ( '=' p.Expr ) ? ';'
        - Fun = 'fun' p.Function
        - Class = [
          - x = 'class' identifier Super '{' p.p.Function * '}'
          - Super = ( '<' identifier ) ?
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
               | number | string
               | 'super' '.' identifier | identifier
        - Arguments = ( x ( ',' x ) * ) ?
      ]
      - Function = [
        - x = identifier '(' Parameters ')' _.Lox.Stmt.Block
        - Parameters = ( Parameter ( ',' Parameter ) * ) ?
        - Parameter = identifier
      ]
    ]
  )'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);

  struct lexicon_t {
    syntax_farm_ptr_t sfp;

    token_id_t ti_true  = sfp->token_id("true").value();
    token_id_t ti_false = sfp->token_id("false").value();
    token_id_t ti_none  = sfp->token_id("none").value();
    token_id_t ti_this  = sfp->token_id("this").value();
    token_id_t ti_super = sfp->token_id("super").value();
    token_id_t ti_init  = sfp->token_id("init").value();

    name_id_t ni_none           = sfp->name_id_of("None");
    name_id_t ni_lox            = sfp->name_id_of("Lox");
    name_id_t ni_decl           = sfp->name_id_of(ni_lox, "Decl");
    name_id_t ni_decl_var       = sfp->name_id_of(ni_decl, "Var");
    name_id_t ni_decl_fun       = sfp->name_id_of(ni_decl, "Fun");
    name_id_t ni_decl_class     = sfp->name_id_of(ni_decl, "Class");
    name_id_t ni_decl_class_s   = sfp->name_id_of(ni_decl_class, "Super");
    name_id_t ni_stmt           = sfp->name_id_of(ni_lox, "Stmt");
    name_id_t ni_stmt_print     = sfp->name_id_of(ni_stmt, "Print");
    name_id_t ni_stmt_if        = sfp->name_id_of(ni_stmt, "If");
    name_id_t ni_stmt_for       = sfp->name_id_of(ni_stmt, "For");
    name_id_t ni_stmt_while     = sfp->name_id_of(ni_stmt, "While");
    name_id_t ni_stmt_return    = sfp->name_id_of(ni_stmt, "Return");
    name_id_t ni_stmt_block     = sfp->name_id_of(ni_stmt, "Block");
    name_id_t ni_stmt_expr      = sfp->name_id_of(ni_stmt, "ExprStmt");
    name_id_t ni_expr           = sfp->name_id_of(ni_lox, "Expr");
    name_id_t ni_expr_primary   = sfp->name_id_of(ni_expr, "Primary", "(");
    name_id_t ni_expr_call      = sfp->name_id_of(ni_expr, "Call", "(");
    name_id_t ni_expr_member    = sfp->name_id_of(ni_expr, "Call", ".");
    name_id_t ni_expr_u_exc     = sfp->name_id_of(ni_expr, "Unary", "!");
    name_id_t ni_expr_u_sub     = sfp->name_id_of(ni_expr, "Unary", "-");
    name_id_t ni_expr_b_mul     = sfp->name_id_of(ni_expr, "Factor", "*");
    name_id_t ni_expr_b_div     = sfp->name_id_of(ni_expr, "Factor", "/");
    name_id_t ni_expr_b_add     = sfp->name_id_of(ni_expr, "Term", "+");
    name_id_t ni_expr_b_sub     = sfp->name_id_of(ni_expr, "Term", "-");
    name_id_t ni_expr_b_lt      = sfp->name_id_of(ni_expr, "Comparison", "<");
    name_id_t ni_expr_b_gt      = sfp->name_id_of(ni_expr, "Comparison", ">");
    name_id_t ni_expr_b_lte     = sfp->name_id_of(ni_expr, "Comparison", "<=");
    name_id_t ni_expr_b_gte     = sfp->name_id_of(ni_expr, "Comparison", ">=");
    name_id_t ni_expr_b_eq      = sfp->name_id_of(ni_expr, "Equality", "==");
    name_id_t ni_expr_b_neq     = sfp->name_id_of(ni_expr, "Equality", "!=");
    name_id_t ni_expr_b_and     = sfp->name_id_of(ni_expr, "LogicAnd", "and");
    name_id_t ni_expr_b_or      = sfp->name_id_of(ni_expr, "LogicOr", "or");
    name_id_t ni_expr_b_assign  = sfp->name_id_of(ni_expr, "Assign", "=");
    name_id_t ni_expr_atom      = sfp->name_id_of(ni_expr, "Atom");
    name_id_t ni_function       = sfp->name_id_of(ni_lox, "Function");
    name_id_t ni_function_param = sfp->name_id_of(ni_function, "Parameter");
  };
}
