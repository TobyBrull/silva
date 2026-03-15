#pragma once

#include "seed_tokenizer.impl.hpp"

#include "fragmentization.hpp"
#include "syntax/parse_tree.hpp"
#include "syntax_ward.hpp"

namespace silva::seed {

  // A tokenizer turns a sequence of fragments into a sequence of tokens (or returns an error)
  // according to a simple sorted list of rules. The rules in a tokenizer are sorted by priority,
  // from highest lowest. Each rule consists of:
  //  * a non-empty list of "prefix-matchers" that must match exactly in order,
  //  * an optional set (i.e., unsorted list) of "repeat-matchers", and
  //  * a "token-name" that is assigned to the sequence of matched fragments if the rule matches.
  //
  // The ::: operator is used to delimit prefix-matchers and repeat-matchers. Three types of rules
  // are supported: include-rules, ignore-rules, and the basic token-rules.
  //
  // Here is an example tokenizer:
  //  ⎢ignore NUMBER                            # ignore NUMBER fragments
  //  ⎢include tokenzier FreeForm               # include the rules from another tokenizer, in order
  //  ⎢name = [ '$' '@' ] IDENTIFIER
  //  ⎢name = IDENTIFIER\'_t'
  //  ⎢rel-path = IDENTIFIER ::: '/' '.' IDENTIFIER
  //  ⎢op = ::: '=' '+'
  //
  // This tokenizer would tokenize the code
  //  ⎢ $hello =+++= 42 array_t var/file.txt
  // as the following sequence of tokens: "name op name rel-path", as explained in the
  // following.
  //
  // The tokenization algorithm works as follows:
  // While the current remaining set of fragments (=: input-fragment-rest) is not empty, find the
  // first rule whose prefix-matchers match the input-fragment-rest. After this, also gobble up as
  // many fragments as possible as long as they match any of the repeat-matchers. The whole sequence
  // of matched fragments is then turned into a token with the token-name of the matching rule. If
  // none of the rules matches, return an error.
  //
  // It follows that each token corresponds to one or more fragments.
  //
  // The basic available set of matchers is
  //  ⎢WHITESPACE
  //  ⎢COMMENT
  //  ⎢NUMBER
  //  ⎢STRING
  //  ⎢INDENT
  //  ⎢DEDENT
  //  ⎢NEWLINE
  //  ⎢PARENTHESES
  //  ⎢OPERATOR
  //  ⎢IDENTIFIER_SILVA_CASE          # matches, e.g., silva hello-world
  //  ⎢IDENTIFIER_SNAKE_CASE          # matches, e.g., silva hello_world
  //  ⎢IDENTIFIER_CAMEL_CASE          # matches, e.g., silva helloWorld
  //  ⎢IDENTIFIER_PASCAL_CASE         # matches, e.g., Silva HelloWorld
  //  ⎢IDENTIFIER_MACRO_CASE          # matches, e.g., SILVA HELLO_WORLD
  //  ⎢IDENTIFIER_UPPER_CASE          # matches, e.g., SILVA HELLOWORLD
  //  ⎢IDENTIFIER_LOWER_CASE          # matches, e.g., silva helloworld
  //  ⎢IDENTIFIER                     # matches any identifier, e.g., any of the above examples
  //  ⎢LANGUAGE
  //
  // Each matcher may be amended by a required prefix (operator /), a required postfix (operator \),
  // or a required exact string (operator |). This mostly makes sense for operators and identifiers.
  // For example,
  //  ⎢IDENTIFIER\'_t'                # matches, e.g., tree_t ARRAY_t
  //  ⎢IDENTIFIER_SILVA_CASE/'tok-'   # matches, e.g., tok-full-name tok-category
  //  ⎢IDENTIFIER/'a'\'z'             # matches, e.g., aBCz abcz
  //  ⎢OPERATOR|'+'                   # matches only the fragment +
  // Note that « OPERATOR|'++' » would never match anything, because each input codepoint that's an
  // operator is always fragmented as its own fragment.
  //
  // Several shortcuts are available.
  //
  // 1) A plain string is interpreted by first fragmenting said string, and then replacing it with
  // the sequence of matchers matching exactly the resulting fragments. For example « '++' » is
  // equivalent to the sequence of matchers « OPERATOR|'+' OPERATOR|'+' ». When used in the
  // repeat-matchers part, a string must result in exactly a single fragment.
  //
  // 2) Lists of the form [...] can be used in the prefix-matchers part to generate multiple rules.
  // Multiple lists of this form result in the Cartesian product of the rules. For example,
  //  ⎢op = [ '++' '+' ] [ '==' '=' ] ::: IDENTIFIER '.'
  // is equivalent to the following rules (note the order):
  //  ⎢op = '+' '+' '=' '=' ::: IDENTIFIER '.'  # matches, e.g., ++== ++==... ++==abc.def
  //  ⎢op = '+' '+' '=' ::: IDENTIFIER '.'      # matches, e.g., ++=  ++=...  ++=abc.def
  //  ⎢op = '+' '=' '=' ::: IDENTIFIER '.'      # matches, e.g., +==  +==...  +==abc.def
  //  ⎢op = '+' '=' ::: IDENTIFIER '.'          # matches, e.g., +=   +=...   +=abc.def

  const string_view_t seed_tokenizer_str = R"(
    - tokenizer = Default [
      - ignore WHITESPACE
      - ignore COMMENT
      - indent = INDENT
      - dedent = DEDENT
      - newline = NEWLINE
      - number = NUMBER
      - string = STRING
      - language = LANGUAGE
    ]
    - tokenizer = FreeForm [
      - ignore WHITESPACE
      - ignore COMMENT
      - ignore INDENT
      - ignore DEDENT
      - ignore NEWLINE
      - number = NUMBER
      - string = STRING
      - language = LANGUAGE
    ]
    - tokenizer = Seed [
      - tokenizer FreeFrom
      - operators = ::: PARENTHESES OPERATOR 'concat' but_then' 'x' 'p' '_'
      - rule_name = IDENTIFIER_PASCAL_CASE
      - var_name = IDENTIFIER_SNAKE_CASE\'_v'
      - func_name = IDENTIFIER_SNAKE_CASE\'_f'
      - token_category_name = IDENTIFIER_SNAKE_CASE
      - frag_name = IDENTIFIER_MACRO_CASE
    ]
  )";

  const string_view_t tokenizer_seed_str = R"'(
    - Seed.Tokenzier = [
      - x = p.Nonterminal.Base '[' ( '-' Rule ) * ']'
      - Rule =  IncludeRule | IgnoreRule | TokenRule
      - IncludeRule = 'include' 'tokenizer' p.Nonterminal.Base
      - IgnoreRule = 'ignore' Rule
      - TokenRule = '=' Rule
      - Rule = Atom * ( ':::' Atom + ) ?
      - Atom = Matcher | string | List
      - Matcher = FragName ( '/' string ) ? ( '\\' string ) ? ( '|' string )?
      - List = '[' Atom * ']'
      - FragName = identifier / '^[A-Z_]+$'
    ]
  )'";

  struct tokenizer_t {
    syntax_ward_ptr_t swp;
    array_t<impl::rule_t> rules;

    expected_t<tokenization_ptr_t> apply(syntax_ward_ptr_t, const fragmentization_t&) const;
  };

  expected_t<tokenizer_t>
  tokenizer_create(syntax_ward_ptr_t, name_id_t tokenizer_name, parse_tree_span_t);
}

// IMPLEMENTATION

namespace silva::seed {
}
