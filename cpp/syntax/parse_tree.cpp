#include "parse_tree.hpp"

#include "canopy/convert.hpp"
#include "canopy/tree.hpp"

namespace silva {
  constexpr index_t max_num_tokens = 5;

  string_or_view_t to_string_impl(const parse_tree_span_t& pts)
  {
    if (!pts.tokenization) {
      return string_or_view_t{string_view_t{"unknown parse_tree_span"}};
    }
    return string_or_view_t{fmt::format("[{}] parse_tree_span[ {} ]",
                                        to_string(pts.token_position()),
                                        to_string(pts.token_range()))};
  }

  expected_t<string_t> parse_tree_span_t::to_string(const index_t token_offset,
                                                    const parse_tree_printing_t printing)
  {
    token_catalog_ptr_t tcp    = tokenization->context;
    const name_id_style_t& nis = tcp->default_name_id_style();
    return tree_span_t::to_string([&](string_t& curr_line, auto& path) {
      const auto pts = this->sub_tree_span_at(path.back().node_index);
      using enum parse_tree_printing_t;
      if (printing == ABSOLUTE) {
        curr_line += nis.absolute(pts[0].rule_name);
      }
      else {
        if (path.size() >= 2) {
          name_id_t from = (*this)[path[path.size() - 2].node_index].rule_name;
          curr_line += nis.relative(from, pts[0].rule_name);
        }
        else {
          curr_line += nis.absolute(pts[0].rule_name);
        }
      }
      do {
        curr_line.push_back(' ');
      } while (curr_line.size() < token_offset);
      curr_line += silva::to_string(pts.token_range()).as_string_view();
    });
  }

  expected_t<string_t> parse_tree_span_t::to_graphviz()
  {
    token_catalog_ptr_t tcp    = tokenization->context;
    const name_id_style_t& nis = tcp->default_name_id_style();
    return tree_span_t::to_graphviz([&](auto& node) {
      return fmt::format("{}\\n{}",
                         nis.absolute(node.rule_name),
                         string_escaped(tokenization->token_info_get(node.token_begin)->str));
    });
  }

  parse_tree_span_t::parse_tree_span_t(const parse_tree_t& other)
    : tree_span_t(other.nodes), tokenization(other.tokenization)
  {
  }

  parse_tree_span_t::parse_tree_span_t(const parse_tree_node_t* root,
                                       index_t stride,
                                       shared_ptr_t<const tokenization_t> tokenization)
    : tree_span_t(root, stride), tokenization(std::move(tokenization))
  {
  }

  parse_tree_span_t parse_tree_span_t::sub_tree_span_at(const index_t pos) const
  {
    return parse_tree_span_t{&((*this)[pos]), stride, tokenization};
  }

  token_range_t parse_tree_span_t::token_range() const
  {
    return token_range_t{
        .tp          = tokenization->ptr(),
        .token_begin = (*this)[0].token_begin,
        .token_end   = (*this)[0].token_end,
    };
  }

  token_position_t parse_tree_span_t::token_position() const
  {
    return token_position_t{
        .tp          = tokenization->ptr(),
        .token_index = (*this)[0].token_begin,
    };
  }

  parse_tree_t parse_tree_span_t::copy() const
  {
    vector_t<parse_tree_node_t> nodes;
    nodes.reserve(size());
    for (index_t i = 0; i < size(); ++i) {
      nodes.push_back((*this)[i]);
    }
    return parse_tree_t{
        .tokenization = tokenization,
        .nodes        = std::move(nodes),
    };
  }
}
