#include "name_id_style.hpp"

namespace silva {
  string_t name_id_style_t::absolute(const name_id_t target_fni) const
  {
    if (target_fni == name_id_root) {
      return swp->token_infos[root].str;
    }
    const name_info_t& fni = swp->name_infos[target_fni];
    return absolute(fni.parent_name) + swp->token_infos[separator].str +
        swp->token_infos[fni.base_name].str;
  }

  string_t name_id_style_t::relative(const name_id_t current_fni, const name_id_t target_fni) const
  {
    const name_id_t lca = swp->name_id_lca(current_fni, target_fni);

    string_t first_part;
    {
      name_id_t curr = current_fni;
      while (curr != lca) {
        if (!first_part.empty()) {
          first_part += swp->token_infos[separator].str;
        }
        first_part += swp->token_infos[parent].str;
        curr = swp->name_infos[curr].parent_name;
      }
    }

    string_t second_part;
    {
      name_id_t curr = target_fni;
      while (curr != lca) {
        if (!second_part.empty()) {
          second_part = swp->token_infos[separator].str + second_part;
        }
        const name_info_t* fni = &swp->name_infos[curr];
        second_part            = swp->token_infos[fni->base_name].str + second_part;
        curr                   = swp->name_infos[curr].parent_name;
      }
    }
    if (!first_part.empty() && !second_part.empty()) {
      return first_part + swp->token_infos[separator].str + second_part;
    }
    else if (first_part.empty() && second_part.empty()) {
      return swp->token_infos[current].str;
    }
    else {
      return first_part + second_part;
    }
  }

  expected_t<token_id_t> name_id_style_t::derive_base_name(const name_id_t scope_name,
                                                           const parse_tree_span_t pts_nt) const
  {
    const auto& s_node = pts_nt[0];
    SILVA_EXPECT(s_node.rule_name == ni_nonterminal_base && s_node.num_children == 0,
                 MINOR,
                 "expected Nonterminal.Base");
    const token_id_t retval = pts_nt.tp->tokens[s_node.token_begin];
    return retval;
  }

  expected_t<name_id_t>
  name_id_style_t::derive_relative_name(const name_id_t scope_name,
                                        const parse_tree_span_t pts_nt_base) const
  {
    name_id_t retval     = scope_name;
    const auto base_name = SILVA_EXPECT_FWD(derive_base_name(scope_name, pts_nt_base));
    if (base_name == current) {
      return scope_name;
    }
    else {
      return swp->name_id(scope_name, base_name);
    }
    return retval;
  }

  expected_t<name_id_t> name_id_style_t::derive_name(const name_id_t scope_name,
                                                     const parse_tree_span_t pts_nt) const
  {
    name_id_t retval = scope_name;
    SILVA_EXPECT(pts_nt[0].rule_name == ni_nonterminal, MINOR, "expected Nonterminal");
    for (const auto [child_node_index, child_index]: pts_nt.children_range()) {
      const auto& s_node = pts_nt[child_node_index];
      SILVA_EXPECT(s_node.rule_name == ni_nonterminal_base, MINOR, "expected Nonterminal.Base");
      const token_id_t base = pts_nt.tp->tokens[s_node.token_begin];
      if (base == root) {
        SILVA_EXPECT(child_index == 0, MINOR, "Root node may only appear as first element");
        retval = name_id_root;
      }
      else if (base == current) {
        ;
      }
      else if (base == parent) {
        retval = swp->name_infos[retval].parent_name;
      }
      else {
        retval = swp->name_id(retval, base);
      }
    }
    return retval;
  }
}
