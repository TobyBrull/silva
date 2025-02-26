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
    - Seed = ("-" Rule)*
    - Rule = Nonterminal ( "," RulePrecedence )? Derivation
    - RulePrecedence = number
    - Derivation,0 = "=" major_error Atom+
    - Derivation,1 = "=~" major_error Terminal+
    - Derivation,2 = "=>" major_error Nonterminal "," RulePrecedence
    - Derivation,3 = "=%" major_error Nonterminal AxeScope
    - Atom,0 = "major_error"
    - Atom,1 = Primary Suffix?
    - Suffix =~ "?" "*" "+" "!" "&"
    - Primary,0 = "(" Atom+ ")"
    - Primary,1 = Terminal
    - Primary,2 = Nonterminal
    - Nonterminal = identifier_regex("^[A-Z]")
    - Terminal,0 = "identifier_regex" "(" Regex ")"
    - Terminal,1 =~ string "identifier" "operator" "string" "number" "any"
    - Regex = string
    - AxeScope = "[" ("-" AxeLevel)* "]"
    - AxeLevel = Nonterminal "=" AxeAssoc AxeOps*
    - AxeAssoc =~ "nest" "ltr" "rtl" "flat"
    - AxeOps = AxeOpType AxeOp*
    - AxeOpType =~ "atom_nest" "prefix" "prefix_nest" "infix" "ternary" "postfix" "postfix_nest"
    - AxeOp =~ string "none"
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
