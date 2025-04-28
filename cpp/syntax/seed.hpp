#pragma once

#include "parse_tree_nursery.hpp"

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
      - Rule = Nonterminal ( '=' '[' x ']' | ExprOrAlias | '=/' AxeWithAtom )
      - ExprOrAlias = ( '=' | '=>' ) Expr
      - AxeWithAtom = Nonterminal Axe
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
         - Arg = p.Expr | p.Variable
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
  expected_t<parse_tree_ptr_t> seed_parse(tokenization_ptr_t);
  unique_ptr_t<seed_engine_t> seed_seed_engine(syntax_ward_ptr_t);

  struct name_id_style_t {
    syntax_ward_ptr_t swp;
    token_id_t root      = *swp->token_id("_");
    token_id_t current   = *swp->token_id("x");
    token_id_t parent    = *swp->token_id("p");
    token_id_t separator = *swp->token_id(".");

    name_id_t ni_nonterminal      = swp->name_id_of("Seed", "Nonterminal");
    name_id_t ni_nonterminal_base = swp->name_id_of("Seed", "Nonterminal", "Base");

    name_id_t from_token_span(name_id_t current, span_t<const token_id_t>) const;

    string_t absolute(name_id_t) const;
    string_t relative(name_id_t current, name_id_t) const;
    string_t readable(name_id_t current, name_id_t) const;

    expected_t<token_id_t> derive_base_name(const name_id_t scope_name,
                                            const parse_tree_span_t pts_nonterminal_base) const;

    expected_t<name_id_t> derive_relative_name(const name_id_t scope_name,
                                               const parse_tree_span_t pts_nonterminal_base) const;

    expected_t<name_id_t> derive_name(const name_id_t scope_name,
                                      const parse_tree_span_t pts_nonterminal) const;
  };
}
