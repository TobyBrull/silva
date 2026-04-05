#include "name_id_style.hpp"

namespace silva {
  string_t name_id_style_t::absolute(const name_id_t tgt) const
  {
    if (tgt == name_id_root) {
      return "";
    }
    const name_info_t& fni = sfp->name_infos[tgt];
    return absolute(fni.parent_name) + sfp->token_infos[separator].str +
        sfp->token_infos[fni.base_name].str;
  }

  expected_t<name_id_t> name_id_style_t::derive_name(const name_id_t scope_name,
                                                     const parse_tree_span_t pts_nt) const
  {
    const auto& tokens = pts_nt.tp->tokens;
    name_id_t retval   = scope_name;
    SILVA_EXPECT(pts_nt[0].rule_name == ni_nonterminal, MINOR, "expected Nonterminal");
    if (tokens[pts_nt[0].token_begin] == separator) {
      retval = name_id_root;
    }
    for (const auto [child_node_index, child_index]: pts_nt.children_range()) {
      const auto& s_node = pts_nt[child_node_index];
      SILVA_EXPECT(s_node.rule_name == ni_nonterminal_base, MINOR, "expected Nonterminal.Base");
      const token_id_t base = tokens[s_node.token_begin];
      if (base == current) {
        ;
      }
      else {
        retval = sfp->name_id(retval, base);
      }
    }
    return retval;
  }

}
