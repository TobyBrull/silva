#pragma once

#include "seed.lexicon.hpp"

#include "parse_tree.hpp"

namespace silva::seed {

  const string_view_t seed_str = R"'(
language Seed:
  skip.@ = offSide
  skip.initial = offSide.initial

  fragName = identifier.macroCase
  ruleName = identifier.pascalCase
  tokenCategoryName = identifier.camelCase

  @ = [ Language Scope Rule ] *
  Language = "language" ruleName ':' ScopeImpl
  Scope = Nonterminal ':' ScopeImpl
  ScopeImpl = no_node newline indent ( Scope | Rule ) * dedent
  Rule = Nonterminal '=' qualifier * ( "axe" Axe | Expr newline )
  qualifier = [ "no_node" "no_whitespace" "literal_nodes" ]
  Expr:
    @ = axe Atom
      Affix         = ltr   infix "ending_with" "starting_with"
      Lookahead     = rtl   prefix '!' '&'
      Repetition    = ltr   postfix '?' '*' '+' \
                            postfix_nest -> Quantifier '{' '}'
      Concat        = ltr   infix_flat concat
      Followup      = ltr   infix_flat '⇒'
      Choice        = ltr   infix_flat '|'
    Atom = no_node Terminal | Nonterminal | '(' Expr ')' | Alternation
    Alternation = '[' ( Terminal | Nonterminal ) + ']'
    Quantifier = literal_nodes number ? ',' number ? | number
    NoNode = Expr
  Terminal = "literals_of" Nonterminal | [ keyword string fragName ]
  keyword = [ "ε" "language" '~' ]
  Nonterminal = literal_nodes '.' ? ( Name '.' ) * [ Name '@' ]
  Name = no_node [ ruleName tokenCategoryName ]
)'";

  // Handcrafted Seed parser for bootstrapping purposes.
  //
  // Unlike silva::seed::interpreter_t, this bootstrap-interpreter can only parse Seed programs.
  //
  // Invariant (pseudo-code):
  //    standard_seed_engine()->apply(x, "Seed") == bootstrap_interpreter_t{}.parse(x)
  //
  struct bootstrap_interpreter_t {
    struct impl_t;
    unique_ptr_t<impl_t> impl;

    bootstrap_interpreter_t(syntax_farm_ptr_t);
    ~bootstrap_interpreter_t();

    const lexicon_t& lexicon() const;

    expected_t<parse_tree_ptr_t> parse(fragment_span_t);
  };
}
