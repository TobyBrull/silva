#pragma once

#include "seed.lexicon.hpp"

#include "parse_tree.hpp"

namespace silva::seed {

  // A program in the Seed language describes a way to turn a tokenization_t into a parse_tree_t.
  // The actual algorithm to do this is implemented in "seed::interpreter_t".
  //
  // The language is akin to BNF. The resulting parsing algorithm is essentially a PEG but with one
  // caveat, which is called the "prefix-rule".
  //
  //  * The quite readable original paper by Bryan Ford "Parsing Expression Grammars: A
  //    Recognition-Based Syntactic Foundation"
  //    https://bford.info/pub/lang/peg.pdf
  //
  //  * Guido van Rossum's "PEG Parsing Series"
  //    https://medium.com/@gvanrossum_83706/peg-parsing-series-de5d41b2ed60
  //
  // The prefix-rule says that in every concatenated expression, if all leading literals are
  // matched, then the whole concatenated expression has to match; otherwise, the entire parse
  // algorithm results in failure. For example, if the rule
  //
  // « - ConstFunc = "static" "func" identifier | "static" identifier number »
  //
  // is used to parse the language
  //
  // « static func 2 »
  //
  // this will result in a parse error even though the second alternative would be a match. This is
  // because all leading literals (the "prefix") in the first alternative (of which there are two:
  // 'static' and 'func') are already matched. So at that point, the parsing algorithm commits to
  // the first alternative and aborts the parse if the whole expression does not match.
  //
  // To disable the prefix rule for a concatenated expression, use the epsilon production. So, the
  // rule
  //
  // « - ConstFunc = ε "static" "func" identifier | "static" identifier number »
  //
  // parses
  //
  // « static func 2 »
  //
  // just fine.
  //
  // Other remarks:
  //  * "any" matches any token and also end-of-file.
  //  * There is a subtle distinction between literals that use double-quotes (") and those that use
  //    single-quotes ('). Double-quotes only accept text as content, while single-quotes maybe
  //    contain any character. With single-quotes, it simply tries to match the code-points in the
  //    literal one by one. With double-quotes it does that but then also checks that the following
  //    character (if it exists) is NOT text. So, the literal "language" doesn't match the beginning
  //    of « language_name » but 'language' does.

  const string_view_t seed_str = R"'(
language Seed:
  skip = skip.offSide

  fragName = identifier.macroCase
  ruleName = identifier.pascalCase
  tokenCategoryName = identifier.camelCase

  ⊙ = [ Language Scope Rule ] *
  Language = "language" ruleName ':' ScopeImpl
  Scope = Nonterminal ':' ScopeImpl
  ScopeImpl = no_node newline indent ( Scope | Rule ) * dedent
  Rule = ( '⊙' | Nonterminal ) '=' ( "axe" Axe | Qualifier * Expr newline )
  Qualifier = [ "no_node" "no_whitespace" ]
  Expr:
    ⊙ = axe Atom operator
      Prefix    = rtl   prefix "not"
      Postfix   = ltr   postfix '?' '*' '+' \
                        postfix_nest -> Quantifier '{' '}'
      Concat    = ltr   infix_flat concat
      And       = ltr   infix_flat "but_then"
      Followup  = ltr   infix_flat '⇒'
      Or        = ltr   infix_flat '|'
    Atom = no_node Terminal | Nonterminal | '(' Expr ')' | Alternation
    Alternation = '[' ( Terminal | Nonterminal ) + ']'
    Quantifier = number ? ',' number ? | number
    operator = [ "not" "but_then" operator.single '{' '}' ]
    NoNode = Expr
  Terminal = [ "ε" "end_of_language" "language" string fragName ]
  Nonterminal = '.' ? ( Name '.' ) * Name
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
