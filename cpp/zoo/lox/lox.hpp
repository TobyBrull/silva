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
               | number | string
               | 'super' '.' identifier | identifier
        - Arguments = ( x ( ',' x ) * ) ?
      ]
    ]
  )'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_ward_ptr_t);

  struct lexicon_t {
    syntax_ward_ptr_t swp;

    token_id_t ti_true  = swp->token_id("true").value();
    token_id_t ti_false = swp->token_id("false").value();
    token_id_t ti_none  = swp->token_id("none").value();
    token_id_t ti_this  = swp->token_id("this").value();
    token_id_t ti_super = swp->token_id("super").value();
    token_id_t ti_init  = swp->token_id("init").value();

    name_id_t ni_none                = swp->name_id_of("None");
    name_id_t ni_lox                 = swp->name_id_of("Lox");
    name_id_t ni_decl                = swp->name_id_of(ni_lox, "Decl");
    name_id_t ni_decl_var            = swp->name_id_of(ni_decl, "Var");
    name_id_t ni_decl_fun            = swp->name_id_of(ni_decl, "Fun");
    name_id_t ni_decl_class          = swp->name_id_of(ni_decl, "Class");
    name_id_t ni_decl_class_s        = swp->name_id_of(ni_decl_class, "Super");
    name_id_t ni_decl_function       = swp->name_id_of(ni_decl, "Function");
    name_id_t ni_decl_function_param = swp->name_id_of(ni_decl_function, "Parameter");
    name_id_t ni_stmt                = swp->name_id_of(ni_lox, "Stmt");
    name_id_t ni_stmt_print          = swp->name_id_of(ni_stmt, "Print");
    name_id_t ni_stmt_if             = swp->name_id_of(ni_stmt, "If");
    name_id_t ni_stmt_for            = swp->name_id_of(ni_stmt, "For");
    name_id_t ni_stmt_while          = swp->name_id_of(ni_stmt, "While");
    name_id_t ni_stmt_return         = swp->name_id_of(ni_stmt, "Return");
    name_id_t ni_stmt_block          = swp->name_id_of(ni_stmt, "Block");
    name_id_t ni_stmt_expr           = swp->name_id_of(ni_stmt, "ExprStmt");
    name_id_t ni_expr                = swp->name_id_of(ni_lox, "Expr");
    name_id_t ni_expr_primary        = swp->name_id_of(ni_expr, "Primary", "(");
    name_id_t ni_expr_call           = swp->name_id_of(ni_expr, "Call", "(");
    name_id_t ni_expr_member         = swp->name_id_of(ni_expr, "Call", ".");
    name_id_t ni_expr_u_exc          = swp->name_id_of(ni_expr, "Unary", "!");
    name_id_t ni_expr_u_sub          = swp->name_id_of(ni_expr, "Unary", "-");
    name_id_t ni_expr_b_mul          = swp->name_id_of(ni_expr, "Factor", "*");
    name_id_t ni_expr_b_div          = swp->name_id_of(ni_expr, "Factor", "/");
    name_id_t ni_expr_b_add          = swp->name_id_of(ni_expr, "Term", "+");
    name_id_t ni_expr_b_sub          = swp->name_id_of(ni_expr, "Term", "-");
    name_id_t ni_expr_b_lt           = swp->name_id_of(ni_expr, "Comparison", "<");
    name_id_t ni_expr_b_gt           = swp->name_id_of(ni_expr, "Comparison", ">");
    name_id_t ni_expr_b_lte          = swp->name_id_of(ni_expr, "Comparison", "<=");
    name_id_t ni_expr_b_gte          = swp->name_id_of(ni_expr, "Comparison", ">=");
    name_id_t ni_expr_b_eq           = swp->name_id_of(ni_expr, "Equality", "==");
    name_id_t ni_expr_b_neq          = swp->name_id_of(ni_expr, "Equality", "!=");
    name_id_t ni_expr_b_and          = swp->name_id_of(ni_expr, "LogicAnd", "and");
    name_id_t ni_expr_b_or           = swp->name_id_of(ni_expr, "LogicOr", "or");
    name_id_t ni_expr_b_assign       = swp->name_id_of(ni_expr, "Assign", "=");
    name_id_t ni_expr_atom           = swp->name_id_of(ni_expr, "Atom");
  };
}
