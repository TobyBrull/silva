#pragma once

#include "syntax/parse_tree.hpp"

namespace silva::seed {

  // A program in the Seed language describes a way to turn a tokenization_t into a parse_tree_t.
  // The actual algorithm to do this is implemented in "seed::interpreter_t".
  //
  // The language is akin to BNF. The resulting parsing algorithm is essentially a PEG but with one
  // caveat.
  //
  //  * The quite readable original paper by Bryan Ford "Parsing Expression Grammars: A
  //    Recognition-Based Syntactic Foundation"
  //    https://bford.info/pub/lang/peg.pdf
  //
  //  * Guido van Rossum's "PEG Parsing Series"
  //    https://medium.com/@gvanrossum_83706/peg-parsing-series-de5d41b2ed60
  //
  // This caveat we call the "prefix-rule" here. The prefix-rule says that in every concatenated
  // expression, if all leading literals are matched, then the whole concatenated expression has to
  // match; otherwise, the entire parse algorithm results in failure. For example, if the rule
  //
  // « - ConstFunc = 'static' 'func' identifier | 'static' identifier number »
  //
  // is used to parse the language
  //
  // « static func 2 »
  //
  // this will result in a parse error even though the second alternative would be a match. This is
  // because, all leading literals (the "prefix") in the first alternative (of which there are two:
  // 'static' and 'func') are already matched. So at that point, the parsing algorithm commits to
  // the first alternative and aborts the parse if the whole expression does not match.

  const string_view_t seed_str = R"'(
    - Seed = [
      - x = ( '-' Rule ) *
      - Rule = Nonterminal ( '=' '[' x ']' | ExprOrAlias | '=/' Axe )
      - ExprOrAlias = ( '=' | '=>' ) Expr
      - Expr =/ Atom [
        - Parens    = nest  atom_nest '(' ')'
        - Prefix    = rtl   prefix 'not'
        - Postfix   = ltr   postfix '?' '*' '+'
        - Concat    = ltr   infix_flat concat
        - And       = ltr   infix_flat 'but_then'
        - Or        = ltr   infix_flat '|'
      ]
      - Atom => NonterminalMaybeVar | Terminal | Function
      - NonterminalMaybeVar = Nonterminal ( '->' Variable ) ?
      - Variable = identifier / '^[a-z].*_v$'
      - Function = [
         - x = Name '(' Args ')'
         - Name = identifier / '^[a-z].*_f$'
         - Args = Arg ( ',' Arg ) *
         - Arg => p.Expr | p.Variable
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
                 | 'any' | 'epsilon' | 'end_of_file'
    ]
    - None = epsilon
  )'";

  // Invariant (pseudo-code):
  //    standard_seed_engine()->apply(tokenization, "Seed") == seed::parse(tokenization)
  expected_t<parse_tree_ptr_t> parse(tokenization_ptr_t);
}
