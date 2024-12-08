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

  // The Seed language describes a way to turn a stream of tokens into a parse_tree_t. The
  // actual mechanism/algorithm to turn a stream of tokens into a parse_tree_t is
  // encapsulated/represented by the class "parse_root_t".

  const source_code_t seed_seed_source_code("seed.seed", R"'(
    - Seed = ("-" Rule)*
    - Rule = Nonterminal ( "," RulePrecedence )? "=" Expr
    - RulePrecedence = number
    - Expr,0 = "{" Terminal+ "}"
    - Expr,1 = Atom*
    - Atom = Primary Suffix?
    - Suffix = { "?" "*" "+" "!" "&" }
    - Primary,0 = "(" Atom+ ")"
    - Primary,1 = Terminal
    - Primary,2 = Nonterminal
    - Nonterminal = identifier
    - Terminal = { string "identifier" "operator" "string" "number" "any" }
  )'");

  enum class seed_rule_t {
    SEED,
    RULE,
    RULE_PRECEDENCE,
    EXPR_0,
    EXPR_1,
    ATOM,
    SUFFIX,
    PRIMARY_0,
    PRIMARY_1,
    PRIMARY_2,
    NONTERMINAL,
    TERMINAL,
  };

  struct parse_root_t;

  // parse_root_t of the parse_trees returned by "seed_parse". Only has the "rules" vector populated
  // with entries equivalent to "seed_self_representation" and the entries in "seed_rule_t".
  const parse_root_t* seed_parse_root_primordial();

  expected_t<parse_tree_t> seed_parse(const tokenization_t*);

  // parse_root_t for the Seed language itself.
  //
  // Silva invariant:
  //    seed_parse_root()->apply(tokens) == seed_parse(tokens)
  //
  const parse_root_t* seed_parse_root();
}
