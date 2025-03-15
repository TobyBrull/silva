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

  // A program in the Seed language describes a way to turn a stream of tokens into a parse_tree_t.
  // The actual mechanism/algorithm to turn a stream of tokens into a parse_tree_t is
  // encapsulated/represented by the class "parse_root_t".

  const string_view_t seed_seed = R"'(
    - Seed [
      - X = ( '-' Rule ) *
      - Rule = Nonterminal.Base ( '=' Expr
                                | '=/' Axe
                                | '=>' Alias
                                | '[' Silva.Seed ']' )
      - Expr =/ Atom [
        - Parens    = nest  atom_nest '(' ')'
        - Postfix   = ltr   postfix '?' '*' '+' '!' '&'
        - Concat    = ltr   infix_flat concat
        - Alt       = ltr   infix_flat '|'
      ]
      - Atom => [ Nonterminal Terminal ]
      - Alias = '[' Nonterminal + ']'
      - Axe [
        - X = Up.Nonterminal '[' ( '-' Level ) * ']'
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

  struct parse_root_t;

  full_name_id_style_t seed_full_name_style(token_context_ptr_t);

  expected_t<unique_ptr_t<parse_tree_t>> seed_parse(shared_ptr_t<const tokenization_t>);

  // parse_root_t for the Seed language itself.
  //
  // Silva invariant:
  //    seed_parse_root()->apply(tokens) == seed_parse(tokens)
  //
  unique_ptr_t<parse_root_t> seed_parse_root(token_context_ptr_t);
}
