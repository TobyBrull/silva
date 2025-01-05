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
    - Rule = Nonterminal ( "," RulePrecedence )? Derivation
    - RulePrecedence = number
    - Derivation,0 = "=" major_error Atom+
    - Derivation,1 = "=~" major_error Terminal+
    - Derivation,2 = "=>" major_error Nonterminal ( "," RulePrecedence )?
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
  )'");

  enum class seed_rule_t {
    SEED,
    RULE,
    RULE_PRECEDENCE,
    DERIVATION_0,
    DERIVATION_1,
    DERIVATION_2,
    ATOM_0,
    ATOM_1,
    SUFFIX,
    PRIMARY_0,
    PRIMARY_1,
    PRIMARY_2,
    NONTERMINAL,
    TERMINAL_0,
    TERMINAL_1,
    REGEX,
  };

  struct parse_root_t;

  // parse_root_t of the parse_trees returned by "seed_parse". Only has the "rules" vector populated
  // with entries equivalent to "seed_self_representation" and the entries in "seed_rule_t".
  const parse_root_t* seed_parse_root_primordial();

  expected_t<parse_tree_t> seed_parse(const_ptr_t<tokenization_t>);

  // parse_root_t for the Seed language itself.
  //
  // Silva invariant:
  //    seed_parse_root()->apply(tokens) == seed_parse(tokens)
  //
  const parse_root_t* seed_parse_root();
}
