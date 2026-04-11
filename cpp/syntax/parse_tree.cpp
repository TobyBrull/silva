#include "parse_tree.hpp"

#include "canopy/string_convert.hpp"
#include "canopy/tree.hpp"

#include "seed.lexicon.hpp"

namespace silva {
  constexpr index_t max_num_tokens = 5;

  void pretty_write_impl(const parse_tree_span_t& pts, byte_sink_t* stream)
  {
    if (pts.ptp.is_nullptr()) {
      stream->write_str("unknown parse_tree_span");
    }
    else {
      stream->format("[{}] parse_tree_span[ {} ]",
                     pretty_string(pts.token_location()),
                     pretty_string(pts.token_span()));
    }
  }

  void pretty_write_impl(const parse_tree_node_t& ptn, byte_sink_t* byte_sink)
  {
    byte_sink->format("{}@{}:{}", ptn.rule_name, ptn.token_begin, ptn.token_end);
  }

  expected_t<string_t> parse_tree_span_t::to_string(const index_t token_offset) const
  {
    const seed::lexicon_t& lexicon = ptp->tp->sfp->get_lexicon<seed::lexicon_t>();
    return tree_span_t::to_string([&](string_t& curr_line, auto& path) {
      const auto pts = this->sub_tree_span_at(path.back().node_index);
      curr_line += lexicon.name_id_str(pts[0].rule_name);
      string_pad(curr_line, token_offset);
      curr_line += silva::pretty_string(pts.token_span());
    });
  }

  expected_t<string_t> parse_tree_span_t::to_graphviz() const
  {
    const seed::lexicon_t& lexicon = ptp->tp->sfp->get_lexicon<seed::lexicon_t>();
    return tree_span_t::to_graphviz([&](auto& node) {
      return fmt::format("{}\\n{}",
                         lexicon.name_id_str(node.rule_name),
                         string_escaped(ptp->tp->token_info_get(node.token_begin)->str));
    });
  }

  parse_tree_span_t::parse_tree_span_t(const parse_tree_t& other)
    : tree_span_t(other.nodes), ptp(other.ptr())
  {
  }

  parse_tree_span_t::parse_tree_span_t(const parse_tree_node_t* root,
                                       index_t stride,
                                       parse_tree_ptr_t ptp)
    : tree_span_t(root, stride), ptp(std::move(ptp))
  {
  }

  parse_tree_span_t parse_tree_span_t::sub_tree_span_at(const index_t pos) const
  {
    return parse_tree_span_t{&((*this)[pos]), stride, ptp};
  }

  index_t parse_tree_span_t::count_children_with(const name_id_t name_id) const
  {
    auto [it, end] = children_range();
    index_t retval = 0;
    while (it != end) {
      const auto rn = (*this)[it.pos].rule_name;
      if (rn == name_id) {
        retval += 1;
      }
      ++it;
    }
    return retval;
  }

  expected_t<token_id_t> parse_tree_span_t::front_token_id() const
  {
    const auto& root = (*this)[0];
    SILVA_EXPECT(root.token_begin < root.token_end, MINOR);
    return ptp->tp->tokens[root.token_begin];
  }
  expected_t<token_id_t> parse_tree_span_t::front_token_category() const
  {
    const auto& root = (*this)[0];
    SILVA_EXPECT(root.token_begin < root.token_end, MINOR);
    return ptp->tp->categories[root.token_begin];
  }

  expected_t<token_id_t> parse_tree_span_t::back_token_id() const
  {
    const auto& root = (*this)[0];
    SILVA_EXPECT(root.token_begin < root.token_end, MINOR);
    return ptp->tp->tokens[root.token_end - 1];
  }
  expected_t<token_id_t> parse_tree_span_t::back_token_category() const
  {
    const auto& root = (*this)[0];
    SILVA_EXPECT(root.token_begin < root.token_end, MINOR);
    return ptp->tp->categories[root.token_end - 1];
  }

  token_span_t parse_tree_span_t::token_span() const
  {
    return token_span_t{
        .tp    = ptp->tp,
        .begin = (*this)[0].token_begin,
        .end   = (*this)[0].token_end,
    };
  }

  token_location_t parse_tree_span_t::token_location() const
  {
    return token_location_t{
        .tp          = ptp->tp,
        .token_index = (*this)[0].token_begin,
    };
  }

  parse_tree_t parse_tree_span_t::copy() const
  {
    array_t<parse_tree_node_t> nodes;
    nodes.reserve(size());
    for (index_t i = 0; i < size(); ++i) {
      nodes.push_back((*this)[i]);
    }
    return parse_tree_t{
        .tp    = ptp->tp,
        .nodes = std::move(nodes),
    };
  }
}
