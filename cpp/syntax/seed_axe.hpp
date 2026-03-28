#pragma once

#include "seed_axe.impl.hpp"

#include "parse_tree_nursery.hpp"

#include "canopy/delegate.hpp"

namespace silva::seed {

  // An mechanism for parsing [a]rithmetic e[x]pr[e]ssions. This is a version of the Shunting Yard
  // algorithm or precedence climbing.
  //
  // * https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
  //
  // You basically specify a number of operators, in the order of their desired precedence (from
  // high precedence to low precedence), what type of operator they are (prefix, infx,
  // left-to-right, right-to-left, nested, ...), and what an atom looks like. From this description
  // an expression parser is then generated. For example,
  //
  // « number [
  //   - Parens   = nest  atom_nest '(' ')'
  //   - Prefix   = ltr   prefix '-'
  //   - Product  = ltr   infix '*' '/'
  //   - Addition = ltr   infix '+' '-'
  // ] »
  //
  // can be used to parse
  //
  // « 1 + ( 2 + 3 ) * - 4 »
  //
  // in the normal, mathematical way.

  const string_view_t axe_str = R"'(
    - Seed.Axe = [
      - x = p.Nonterminal '[' ( '-' Level ) * ']'
      - Level = p.Nonterminal.Base '=' Assoc Ops *
      - Assoc = 'nest' | 'ltr' | 'rtl'
      - Ops = OpType ( '->' p.Nonterminal ) ? Op *
      - OpType = 'atom_nest' | 'atom_nest_transparent'
               | 'prefix' | 'prefix_nest'
               | 'infix' | 'infix_flat' | 'ternary'
               | 'postfix' | 'postfix_nest'
      - Op = string | 'concat'
    ]
)'";

  struct axe_t {
    syntax_farm_ptr_t sfp;
    name_id_t name      = name_id_root;
    name_id_t atom_rule = name_id_root;
    hash_map_t<token_id_t, impl::axe_result_t> results;
    optional_t<impl::result_oper_t<impl::oper_regular_t>> concat_result;

    using parse_delegate_t = delegate_t<expected_t<parse_tree_node_t>(name_id_t)>;
    expected_t<parse_tree_node_t> apply(parse_tree_nursery_t&, parse_delegate_t) const;
  };

  expected_t<axe_t> axe_create(syntax_farm_ptr_t, name_id_t axe_name, parse_tree_span_t);
}
