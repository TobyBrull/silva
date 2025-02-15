#include "parse_tree.hpp"

#include "canopy/convert.hpp"

namespace silva {
  constexpr index_t max_shown_tokens = 3;

  expected_t<string_t> parse_tree_to_string(const parse_tree_t& pt, const index_t token_offset)
  {
    string_t curr_line;
    const auto curr_line_space_to = [&curr_line](const index_t n) {
      while (curr_line.size() < n) {
        curr_line.push_back(' ');
      }
    };
    token_context_ptr_t tcp = pt.tokenization->context;
    string_t retval;
    auto result = pt.visit_subtree([&](const span_t<const tree_branch_t> path,
                                       const tree_event_t event) -> expected_t<bool> {
      if (!is_on_entry(event)) {
        return true;
      }
      SILVA_EXPECT(!path.empty(), ASSERT, "Empty path at " SILVA_CPP_LOCATION);
      const parse_tree_t::node_t& node = pt.nodes[path.back().node_index];
      curr_line.clear();
      curr_line_space_to(2 * (path.size() - 1));
      curr_line += fmt::format("[{}]{}",
                               path.back().child_index,
                               tcp->full_name_to_string(node.rule_name, "."));
      curr_line_space_to(token_offset);
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
      retval += curr_line;
      retval += '\n';
      return true;
    });
    SILVA_EXPECT_FWD(std::move(result));
    return retval;
  }

  expected_t<string_t> parse_tree_to_graphviz(const parse_tree_t& pt)
  {
    token_context_ptr_t tcp = pt.tokenization->context;
    string_t retval;
    retval += "digraph parse_tree {\n";
    auto result = pt.visit_subtree([&](const span_t<const tree_branch_t> path,
                                       const tree_event_t event) -> expected_t<bool> {
      if (!is_on_entry(event)) {
        return true;
      }
      SILVA_EXPECT(!path.empty(), ASSERT, "Empty path at " SILVA_CPP_LOCATION);
      const parse_tree_t::node_t& node = pt.nodes[path.back().node_index];

      string_t node_name = "/";
      if (path.size() >= 2) {
        string_t parent_node_name = "/";
        for (index_t i = 1; i < path.size() - 1; ++i) {
          parent_node_name += fmt::format("{}/", path[i].child_index);
        }
        node_name = fmt::format("{}{}/", parent_node_name, path.back().child_index);
        retval += fmt::format("  \"{}\" -> \"{}\"\n", parent_node_name, node_name);
      }
      retval += fmt::format("  \"{}\" [label=\"[{}]{}\\n{}\"]\n",
                            node_name,
                            path.back().child_index,
                            tcp->full_name_to_string(node.rule_name, "."),
                            string_escaped(pt.tokenization->token_info_get(node.token_begin)->str));
      return true;
    });
    SILVA_EXPECT_FWD(std::move(result));
    retval += "}";
    return retval;
  }
}
