#include "parse_tree.hpp"

#include "canopy/convert.hpp"
#include "canopy/tree.hpp"

namespace silva {

  parse_tree_t parse_tree_t::subtree(const index_t node_index) const
  {
    parse_tree_t retval{tree_t<parse_tree_node_data_t>::subtree(node_index)};
    retval.tokenization = tokenization;
    return retval;
  }

  constexpr index_t max_shown_tokens = 3;

  expected_t<string_t> parse_tree_to_string(const parse_tree_t& pt, const index_t token_offset)
  {
    token_context_ptr_t tcp = pt.tokenization->context;
    return tree_to_string(pt, [&](string_t& curr_line, auto& node) {
      curr_line += tcp->full_name_to_string(node.rule_name, ".");
      while (curr_line.size() < token_offset) {
        curr_line.push_back(' ');
      }
      const index_t used_token_index_end =
          std::min(node.token_begin + max_shown_tokens, node.token_end);
      for (index_t token_idx = node.token_begin; token_idx < used_token_index_end; ++token_idx) {
        curr_line += pt.tokenization->token_info_get(token_idx)->str;
        if (token_idx + 1 < used_token_index_end) {
          curr_line += " ";
        }
      }
      if (used_token_index_end < node.token_end) {
        curr_line += " ...";
      }
    });
  }

  expected_t<string_t> parse_tree_to_graphviz(const parse_tree_t& pt)
  {
    token_context_ptr_t tcp = pt.tokenization->context;
    return tree_to_graphviz(pt, [&](auto& node) {
      return fmt::format("{}\\n{}",
                         tcp->full_name_to_string(node.rule_name, "."),
                         string_escaped(pt.tokenization->token_info_get(node.token_begin)->str));
    });
  }
}
