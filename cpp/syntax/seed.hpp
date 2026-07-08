#pragma once

#include "seed.lexicon.hpp"

#include "parse_tree.hpp"

namespace silva::seed {

  // The Seed language offers a way to define parsing functions (called "rules" here) that call each
  // other (it's also possible to hand-write additional parsing functions that call and are called
  // by Seed functions/rules). As such, Seed describes a way to turn a fragmentization_t into a
  // parse_tree_t.
  //
  // (Currently Seed does not yet support hand-written functions, and only a tree-walker is
  // implemented in "seed::interpreter_t".)
  //
  // The language is akin to BNF. The corresponding parsing functions are essentially PEG (greedy
  // repetition, ordered choice) but with a few twists:
  //  * The "prefix directive", see below.
  //  * There is a dedicated "skip" rule to skip whitespace.
  //  * There are two types of rules: branch-rules (indicated by PascalCase) and twig-rules
  //    (indicated by camelCase). Both types of nodes (if successfully parsed) create nodes in the
  //    resulting parse_tree_t (unless they specify the "no_node" qualifier). Branch-rules may call
  //    branch-rules or twig-rules. Twig-rules may only call twig-rules. Branch-rules correspond to
  //    the usual rules that you would find in a grammar for parsing a stream of tokens; twig-rules
  //    correspond to the tokenization.
  //    The skip rule is invoked at the very beginning of a parse, and then also whenever a
  //    *branch-rule* successfully applied a *twig-rule* (but a twig-rule successfully applying
  //    another twig-rule does *not* trigger the skip rule).
  //  * There is a subtle distinction between literals that use double-quotes (") and those that use
  //    single-quotes ('). Double-quotes only accept text (silva::is_fragment_category_text) as
  //    content, while single-quotes maybe contain any character. With single-quotes, it simply
  //    tries to match the code-points in the literal one by one. With double-quotes it does that
  //    but then also checks that the codepoint following the last matched codepoint (if it exists)
  //    is NOT text. So, the literal "language" doesn't match the beginning of « language_name » but
  //    'language' does.
  //
  //
  // References:
  //  * The quite readable original paper by Bryan Ford "Parsing Expression Grammars: A
  //    Recognition-Based Syntactic Foundation"
  //    https://bford.info/pub/lang/peg.pdf
  //
  //  * Guido van Rossum's "PEG Parsing Series"
  //    https://medium.com/@gvanrossum_83706/peg-parsing-series-de5d41b2ed60
  //
  //
  // # The Prefix Directive
  //
  // The prefix directive says that in every concatenated expression, if all leading literals are
  // matched, then the whole concatenated expression has to match; otherwise, the entire parse
  // is failed. For example, if the Seed rule
  //    ⎢ ConstFunc = "static" "func" identifier | "static" identifier number
  // is used to parse the language
  //    ⎢ static func 2
  // this will result in a parse error even though the second alternative would be a match. This is
  // because all leading literals (the "prefix") in the first alternative (of which there are two:
  // "static" and "func") are already matched. So at that point, the parsing algorithm commits to
  // the first alternative and aborts the entire parse if the whole *concatenated expression* does
  // not match.
  //
  // To disable the prefix directive for a concatenated expression, use the epsilon production. So,
  // the rule
  //    ⎢ ConstFunc = ε "static" "func" identifier | "static" identifier number
  // parses « static func 2 » just fine.
  //
  //
  // # Other remarks
  //
  //  * "any" matches any token and also end-of-file.
  //

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
