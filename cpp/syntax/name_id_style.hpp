#pragma once

#include "parse_tree.hpp"

namespace silva {
  struct name_id_style_t {
    syntax_farm_ptr_t sfp;
    token_id_t current   = sfp->token_id("⊙");
    token_id_t separator = sfp->token_id(".");

    name_id_t ni_nonterminal      = sfp->name_id_of("Seed", "Nonterminal");
    name_id_t ni_nonterminal_base = sfp->name_id_of("Seed", "Nonterminal", "Base");

    name_id_t from_token_span(name_id_t current, span_t<const token_id_t>) const;

    string_t absolute(name_id_t) const;

    expected_t<name_id_t> derive_name(const name_id_t scope_name,
                                      const parse_tree_span_t pts_nt) const;

    template<typename Lookup, typename F>
    expected_t<void> for_each_possible_name(const name_id_t scope_name,
                                            const parse_tree_span_t pts_nonterminal,
                                            Lookup,
                                            F) const;
  };
}

// IMPLEMENTATION

namespace silva {
}
