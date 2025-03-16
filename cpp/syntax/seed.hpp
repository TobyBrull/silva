#pragma once

#include "parse_tree.hpp"

// Recommended reading:
//
//  * The quite readable original paper by Bryan Ford "Parsing Expression Grammars: A
//    Recognition-Based Syntactic Foundation"
//    https://bford.info/pub/lang/peg.pdf
//
//  * Guido van Rossum's "PEG Parsing Series"
//    https://medium.com/@gvanrossum_83706/peg-parsing-series-de5d41b2ed60

namespace silva {

  // A program in the Seed language describes a way to turn a tokenization
  // into a parse_tree_t. The actual algorithm to do this is implemented
  // in "seed_engine_t".

  const string_view_t seed_seed = R"'(
    - Seed [
      - X = ( '-' Rule ) *
      - Rule = Nonterminal.Base ( ExprOrAlias | Axe | '[' X ']' )
      - ExprOrAlias = ( '=' | '=>' ) Expr
      - Expr =/ Atom [
        - Parens    = nest  atom_nest '(' ')'
        - Postfix   = ltr   postfix '?' '*' '+' '!' '&'
        - Concat    = ltr   infix_flat concat
        - Alt       = ltr   infix_flat '|'
      ]
      - Atom => Nonterminal | Terminal
      - Axe [
        - X = '=/' Up.Nonterminal '[' ( '-' Level ) * ']'
        - Level = Up.Nonterminal.Base '=' Assoc Ops*
        - Assoc = 'nest' | 'ltr' | 'rtl'
        - Ops = OpType Op*
        - OpType = 'atom_nest' | 'prefix' | 'prefix_nest'
                 | 'infix' | 'infix_flat' | 'ternary'
                 | 'postfix' | 'postfix_nest'
        - Op = string | 'concat'
      ]
      - Nonterminal [
        - X = Base ( '.' Base ) *
        - Base = 'Silva' | 'X' | 'Up' | identifier / '^[A-Z]'
      ]
      - Terminal = string
                 | 'identifier' ( '/' string ) ?
                 | 'operator' ( '/' string ) ?
                 | 'string' | 'number'
                 | 'any' | 'end_of_file'
    ]
  )'";

  struct seed_engine_t;

  name_id_style_t seed_name_style(token_context_ptr_t);

  // Invariant (pseudo-code):
  //    seed_seed_engine()->apply(tokenization, "Seed") == seed_parse(tokenization)
  expected_t<unique_ptr_t<parse_tree_t>> seed_parse(shared_ptr_t<const tokenization_t>);
  unique_ptr_t<seed_engine_t> seed_seed_engine(token_context_ptr_t);
}
