#pragma once

#include "syntax/parse_tree.hpp"

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
        - x = ExprStmt | For | If | Print | Return | While | Block
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
          - Factor      = ltr infix '/' '*'
          - Term        = ltr infix '-' '+'
          - Comparison  = ltr infix '>' '>=' '<' '<='
          - Equality    = ltr infix '==' '!='
          - LogicAnd    = ltr infix 'and'
          - LogicOr     = ltr infix 'or'
          - Assign      = ltr infix '='
          - Comma       = ltr infix_flat ','
        ]
        - Atom = 'true' | 'false' | 'none' | 'this'
               | number | string | identifier
        - Arguments = ( x ( ',' x ) * ) ?
      ]
      - Function = [
        - x = identifier '(' Parameters ? ')' _.Lox.Stmt.Block
        - Parameters = identifier ( ',' identifier ) *
      ]
    ]
  )'";

  struct value_t {
    variant_t<std::nullopt_t, bool, double, string_t> data;

    template<typename T>
    value_t(T&& data);

    friend expected_t<value_t> operator*(const value_t&, const value_t&);
    friend expected_t<value_t> operator+(const value_t&, const value_t&);

    friend string_or_view_t to_string_impl(const value_t&);
  };

  struct interpreter_t {
    syntax_ward_ptr_t swp;

    token_id_t ti_true  = swp->token_id("true").value();
    token_id_t ti_false = swp->token_id("false").value();
    token_id_t ti_none  = swp->token_id("none").value();
    token_id_t ti_this  = swp->token_id("this").value();

    name_id_t ni_lox          = swp->name_id_of("Lox");
    name_id_t ni_decl         = swp->name_id_of(ni_lox, "Decl");
    name_id_t ni_stmt         = swp->name_id_of(ni_lox, "Stmt");
    name_id_t ni_stmt_print   = swp->name_id_of(ni_stmt, "Print");
    name_id_t ni_expr         = swp->name_id_of(ni_lox, "Expr");
    name_id_t ni_expr_primary = swp->name_id_of(ni_expr, "Primary");
    name_id_t ni_expr_b_mul   = swp->name_id_of(ni_expr, "Factor", "*");
    name_id_t ni_expr_b_add   = swp->name_id_of(ni_expr, "Term", "+");
    name_id_t ni_expr_atom    = swp->name_id_of(ni_expr, "Atom");

    expected_t<value_t> evaluate(parse_tree_span_t);

    expected_t<void> execute(parse_tree_span_t);
  };
}

// IMPLEMENTATION

namespace silva::lox {
  template<typename T>
  value_t::value_t(T&& data) : data(std::forward<T>(data))
  {
  }
}
