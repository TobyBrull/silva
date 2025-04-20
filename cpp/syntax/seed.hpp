#pragma once

#include "parse_tree.hpp"

// Recommended reading:
//
//  * The quite readable paper by Bryan Ford "Parsing Expression Grammars: A Recognition-Based
//    Syntactic Foundation"
//    https://bford.info/pub/lang/peg.pdf
//
//  * Guido van Rossum's "PEG Parsing Series"
//    https://medium.com/@gvanrossum_83706/peg-parsing-series-de5d41b2ed60

namespace silva {

  // A program in the Seed language describes a way to turn a tokenization_t
  // into a parse_tree_t. The actual algorithm to do this is implemented
  // in "seed_engine_t".

  const string_view_t seed_seed = R"'(
    - Seed = [
      - x = ( '-' Rule ) *
      - Rule = Nonterminal.Base ( '=' '[' x ']' | ExprOrAlias | Axe )
      - ExprOrAlias = ( '=' | '=>' ) Expr
      - Expr =/ Atom [
        - Parens    = nest  atom_nest '(' ')'
        - Prefix    = rtl   prefix 'not'
        - Postfix   = ltr   postfix '?' '*' '+'
        - Concat    = ltr   infix_flat concat
        - And       = ltr   infix_flat 'but_then'
        - Or        = ltr   infix_flat '|'
      ]
      - Atom => Nonterminal | Terminal
      - Axe = [
        - x = '=/' p.Nonterminal '[' ( '-' Level ) * ']'
        - Level = p.Nonterminal.Base '=' Assoc Ops *
        - Assoc = 'nest' | 'ltr' | 'rtl'
        - Ops = OpType Op *
        - OpType = 'atom_nest' | 'prefix' | 'prefix_nest'
                 | 'infix' | 'infix_flat' | 'ternary'
                 | 'postfix' | 'postfix_nest'
        - Op = string | 'concat'
      ]
      - Nonterminal = [
        - x = Base ( '.' Base ) *
        - Base = '_' | 'x' | 'p' | identifier / '^[A-Z]'
      ]
      - Terminal = string
                 | 'identifier' ( '/' string ) ?
                 | 'operator' ( '/' string ) ?
                 | 'keywords_of' Nonterminal
                 | 'string' | 'number'
                 | 'any' | 'end_of_file'
    ]
  )'";

  struct seed_engine_t;

  // Invariant (pseudo-code):
  //    seed_seed_engine()->apply(tokenization, "Seed") == seed_parse(tokenization)
  expected_t<parse_tree_ptr_t> seed_parse(syntax_catalog_t&, tokenization_ptr_t);
  unique_ptr_t<seed_engine_t> seed_seed_engine(syntax_catalog_t&);
}
