#pragma once

#include "seed_axe.impl.hpp"

// * https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing

// An mechanism for parsing [a]rithmetic e[x]pr[e]ssions. This is a version of the Shunting Yard
// algorithm.

namespace silva {

  const string_view_t seed_axe_seed = R"'(
    - Seed.Axe = [
      - x = '[' ( '-' Level ) * ']'
      - Level = p.Nonterminal.Base '=' Assoc Ops *
      - Assoc = 'nest' | 'ltr' | 'rtl'
      - Ops = OpType Op *
      - OpType = 'atom_nest' | 'prefix' | 'prefix_nest'
               | 'infix' | 'infix_flat' | 'ternary'
               | 'postfix' | 'postfix_nest'
      - Op = string | 'concat'
    ]
  )'";

  struct seed_axe_t {
    syntax_ward_ptr_t swp;
    name_id_t name = name_id_root;
    hashmap_t<token_id_t, impl::seed_axe_result_t> results;
    optional_t<impl::result_oper_t<impl::oper_regular_t>> concat_result;

    expected_t<parse_tree_node_t> apply(parse_tree_nursery_t&,
                                        name_id_t atom_name_id,
                                        delegate_t<expected_t<parse_tree_node_t>()> atom) const;
  };

  expected_t<seed_axe_t>
  seed_axe_create(syntax_ward_ptr_t, name_id_t seed_axe_name, parse_tree_span_t);
}

// IMPLEMENTATION

namespace silva::impl {
  expected_t<seed_axe_t> seed_axe_create(syntax_ward_ptr_t,
                                         name_id_t seed_axe_name,
                                         const vector_t<impl::seed_axe_level_desc_t>&);
}
