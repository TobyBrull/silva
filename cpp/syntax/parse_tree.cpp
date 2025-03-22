#include "parse_tree.hpp"

#include "seed.hpp"

#include "canopy/convert.hpp"
#include "canopy/tree.hpp"

namespace silva {

  parse_tree_t parse_tree_t::subtree(const index_t node_index) const
  {
    parse_tree_t retval{tree_t<parse_tree_node_t>::subtree(node_index)};
    retval.tokenization = tokenization;
    return retval;
  }

  constexpr index_t max_num_tokens = 5;

  expected_t<string_t> parse_tree_span_t::to_string(const index_t token_offset,
                                                    const parse_tree_printing_t printing)
  {
    token_context_ptr_t tcp = tokenization->context;
    const auto style        = seed_name_style(tcp);
    return tree_span_t::to_string([&](string_t& curr_line, auto& path) {
      const auto& node = (*this)[path.back().node_index];
      using enum parse_tree_printing_t;
      if (printing == ABSOLUTE) {
        curr_line += style.absolute(node.rule_name);
      }
      else {
        if (path.size() >= 2) {
          name_id_t from = (*this)[path[path.size() - 2].node_index].rule_name;
          curr_line += style.relative(from, node.rule_name);
        }
        else {
          curr_line += style.absolute(node.rule_name);
        }
      }
      do {
        curr_line.push_back(' ');
      } while (curr_line.size() < token_offset);
      const auto print_tokens = [&curr_line, this](const index_t begin, const index_t end) {
        for (index_t token_idx = begin; token_idx < end; ++token_idx) {
          curr_line += tokenization->token_info_get(token_idx)->str;
          if (token_idx + 1 < end) {
            curr_line += " ";
          }
        }
      };
      const index_t num_tokens = node.token_end - node.token_begin;
      if (num_tokens <= max_num_tokens) {
        print_tokens(node.token_begin, node.token_end);
      }
      else {
        print_tokens(node.token_begin, node.token_begin + max_num_tokens / 2);
        curr_line += " ... ";
        print_tokens(node.token_end - max_num_tokens / 2, node.token_end);
      }
    });
  }

  expected_t<string_t> parse_tree_span_t::to_graphviz()
  {
    token_context_ptr_t tcp = tokenization->context;
    const auto style        = seed_name_style(tcp);
    return tree_span_t::to_graphviz([&](auto& node) {
      return fmt::format("{}\\n{}",
                         style.absolute(node.rule_name),
                         string_escaped(tokenization->token_info_get(node.token_begin)->str));
    });
  }
}
