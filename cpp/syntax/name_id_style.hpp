#pragma once

#include "parse_tree.hpp"

namespace silva {
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
