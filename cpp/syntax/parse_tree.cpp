#include "parse_tree.hpp"

#include "canopy/convert.hpp"
#include "canopy/tree.hpp"
#include "name_id_style.hpp"

namespace silva {
  constexpr index_t max_num_tokens = 5;

  void pretty_write_impl(const parse_tree_span_t& pts, byte_sink_t* stream)
  {
    if (pts.tp.is_nullptr()) {
      stream->write_str("unknown parse_tree_span");
    }
    else {
      stream->format("[{}] parse_tree_span[ {} ]",
                     pretty_string(pts.token_position()),
                     pretty_string(pts.token_range()));
    }
  }

  expected_t<string_t> parse_tree_span_t::to_string(const index_t token_offset,
                                                    const parse_tree_printing_t printing) const
  {
    const name_id_style_t& nis = tp->swp->default_name_id_style();
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
      string_pad(curr_line, token_offset);
      curr_line += silva::pretty_string(pts.token_range());
    });
  }

  expected_t<string_t> parse_tree_span_t::to_graphviz() const
  {
    const name_id_style_t& nis = tp->swp->default_name_id_style();
    return tree_span_t::to_graphviz([&](auto& node) {
      return fmt::format("{}\\n{}",
                         nis.absolute(node.rule_name),
                         string_escaped(tp->token_info_get(node.token_begin)->str));
    });
  }

  parse_tree_span_t::parse_tree_span_t(const parse_tree_t& other)
    : tree_span_t(other.nodes), tp(other.tp)
  {
  }

  parse_tree_span_t::parse_tree_span_t(const parse_tree_node_t* root,
                                       index_t stride,
                                       tokenization_ptr_t tp)
    : tree_span_t(root, stride), tp(std::move(tp))
  {
  }

  parse_tree_span_t parse_tree_span_t::sub_tree_span_at(const index_t pos) const
  {
    return parse_tree_span_t{&((*this)[pos]), stride, tp};
  }

  token_id_t parse_tree_span_t::first_token_id() const
  {
    return tp->tokens[(*this)[0].token_begin];
  }

  token_range_t parse_tree_span_t::token_range() const
  {
    return token_range_t{
        .tp          = tp,
        .token_begin = (*this)[0].token_begin,
        .token_end   = (*this)[0].token_end,
    };
  }

  token_position_t parse_tree_span_t::token_position() const
  {
    return token_position_t{
        .tp          = tp,
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
        .tp    = tp,
        .nodes = std::move(nodes),
    };
  }
}
