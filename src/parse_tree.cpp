#include "parse_tree.hpp"

#include "parse_root.hpp"

#include "canopy/convert.hpp"

#include <utility>

namespace silva {
  bool is_on_entry(const parse_tree_event_t event)
  {
    const auto retval =
        (std::to_underlying(event) & std::to_underlying(parse_tree_event_t::ON_ENTRY));
    return retval != 0;
  }

  bool is_on_exit(const parse_tree_event_t event)
  {
    const auto retval =
        (std::to_underlying(event) & std::to_underlying(parse_tree_event_t::ON_EXIT));
    return retval != 0;
  }

  string_t parse_tree_to_string(const parse_tree_t& pt, const index_t token_offset)
  {
    string_t curr_line;
    const auto curr_line_space_to = [&curr_line](const index_t n) {
      while (curr_line.size() < n) {
        curr_line.push_back(' ');
      }
    };

    string_t retval;
    const auto result = pt.visit_subtree([&](const span_t<const parse_tree_t::visit_state_t> stack,
                                             const parse_tree_event_t event) -> expected_t<bool> {
      if (!is_on_entry(event)) {
        return true;
      }
      SILVA_ASSERT(!stack.empty());
      const parse_tree_t::node_t& node = pt.nodes[stack.back().node_index];
      curr_line.clear();
      if (stack.size() == 1) {
        curr_line = fmt::format("[.]{},{}",
                                pt.root->rules[node.rule_index].name,
                                pt.root->rules[node.rule_index].precedence);
      }
      else {
        curr_line.clear();
        curr_line_space_to(2 * (stack.size() - 1));
        curr_line += fmt::format("[{}]{},{}",
                                 stack.back().child_index,
                                 pt.root->rules[node.rule_index].name,
                                 pt.root->rules[node.rule_index].precedence);
      }
      curr_line_space_to(token_offset);
      curr_line += pt.tokenization->token_data(node.token_index)->str;

      retval += curr_line;
      retval += '\n';
      return true;
    });
    SILVA_ASSERT(result);
    return retval;
  }

  string_t parse_tree_to_graphviz(const parse_tree_t& pt)
  {
    string_t retval;
    retval += "digraph parse_tree {\n";
    const auto result = pt.visit_subtree([&](const span_t<const parse_tree_t::visit_state_t> stack,
                                             const parse_tree_event_t event) -> expected_t<bool> {
      if (!is_on_entry(event)) {
        return true;
      }
      SILVA_ASSERT(!stack.empty());
      const parse_tree_t::node_t& node = pt.nodes[stack.back().node_index];

      string_t node_name = "/";
      if (stack.size() >= 2) {
        string_t parent_node_name = "/";
        for (index_t i = 1; i < stack.size() - 1; ++i) {
          parent_node_name += fmt::format("{}/", stack[i].child_index);
        }
        node_name = fmt::format("{}{}/", parent_node_name, stack.back().child_index);
        retval += fmt::format("  \"{}\" -> \"{}\"\n", parent_node_name, node_name);
      }
      retval += fmt::format("  \"{}\" [label=\"[{}]{},{}\\n{}\"]\n",
                            node_name,
                            stack.back().child_index,
                            pt.root->rules[node.rule_index].name,
                            pt.root->rules[node.rule_index].precedence,
                            string_escaped(pt.tokenization->token_data(node.token_index)->str));
      return true;
    });
    SILVA_ASSERT(result);
    retval += "}";
    return retval;
  }
}
