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
    - Seed = ( "-" Rule ) *
    - Rule = Nonterminal ( "=" Expr | "=/" Axe | "=>" Alias )
    - Expr =/ Atom [
      - Parens    = nest  atom_nest "(" ")"
      - Postfix   = ltr   postfix "?" "*" "+" "!" "&"
      - Concat    = ltr   infix_flat none
      - Alt       = ltr   infix_flat "|"
    ]
    - Atom => [ Nonterminal Terminal ]
    - Alias = "[" Nonterminal + "]"

    - Axe = Nonterminal "[" ( "-" AxeLevel ) * "]"
    - AxeLevel = Nonterminal "=" AxeAssoc AxeOps*
    - AxeAssoc = "nest" | "ltr" | "rtl"
    - AxeOps = AxeOpType AxeOp *
    - AxeOpType = "atom_nest" | "prefix" | "prefix_nest"
                | "infix" | "infix_flat" | "ternary"
                | "postfix" | "postfix_nest"
    - AxeOp = string | "none"

    - Nonterminal = identifier_regex ( "^[A-Z]" )
    - Terminal = string
               | "identifier" | "operator" | "string" | "number"
               | "any" | "identifier_regex" "(" string ")"
  )'";

  struct parse_root_t;

  expected_t<unique_ptr_t<parse_tree_t>> seed_parse(shared_ptr_t<const tokenization_t>);

  // parse_root_t for the Seed language itself.
  //
  // Silva invariant:
  //    seed_parse_root()->apply(tokens) == seed_parse(tokens)
  //
  unique_ptr_t<parse_root_t> seed_parse_root(token_context_ptr_t);
}
