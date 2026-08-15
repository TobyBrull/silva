#include "parse_tree.hpp"

#include "canopy/string_convert.hpp"
#include "canopy/tree.hpp"

#include "seed.lexicon.hpp"

namespace silva {
  void pretty_write_impl(const parse_tree_span_t& pts, byte_sink_t* stream)
  {
    if (pts.ptp.is_nullptr()) {
      stream->write_str("unknown parse_tree_span");
    }
    else {
      stream->format("[{}] parse_tree_span[ {} ]",
                     pretty_string(pts.location()),
                     pretty_string(pts.fragment_span()));
    }
  }

  void pretty_write_impl(const parse_tree_node_t& ptn, byte_sink_t* byte_sink)
  {
    byte_sink->format("{}@{}:{}", ptn.rule_name.val, ptn.fragment_begin, ptn.fragment_end);
  }

  expected_t<string_t> parse_tree_span_t::to_string(const index_t fragment_indent) const
  {
    string_t retval;
    const seed::lexicon_t& lexicon = ptp->fp->sfp->get_lexicon<seed::lexicon_t>();
    retval += SILVA_EXPECT_FWD(tree_span_t::to_string([&](string_t& curr_line, auto& path) {
      const auto pts = this->subspan_at(path.back().node_index);
      curr_line += lexicon.name_id_str(pts[0].rule_name);
      string_pad(curr_line, fragment_indent);
      curr_line += silva::pretty_string(pts.fragment_span());
    }));
    return retval;
  }

  expected_t<string_t> parse_tree_span_t::to_graphviz() const
  {
    const seed::lexicon_t& lexicon = ptp->fp->sfp->get_lexicon<seed::lexicon_t>();
    return tree_span_t::to_graphviz([&](string_t& curr_line, auto& path) {
      const auto pts = this->subspan_at(path.back().node_index);
      curr_line += fmt::format("{}\\n{}",
                               lexicon.name_id_str(pts[0].rule_name),
                               escape_string(silva::pretty_string(pts.fragment_span())));
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

  parse_tree_span_t parse_tree_span_t::subspan_at(const index_t pos) const
  {
    return parse_tree_span_t{&((*this)[pos]), stride, ptp};
  }

  index_t parse_tree_span_t::count_children_with(const name_id_t name_id) const
  {
    index_t retval = 0;
    for (const auto pts_child: children_range()) {
      if (pts_child.rule_name() == name_id) {
        retval += 1;
      }
    }
    return retval;
  }

  expected_t<token_id_t> parse_tree_span_t::token() const
  {
    return fragment_span().derive_token_id();
  }
  expected_t<fragment_span_t> parse_tree_span_t::language() const
  {
    SILVA_EXPECT((*this)[0].rule_name == name_id_language, MINOR);
    return fragment_span();
  }

  fragment_span_t parse_tree_span_t::fragment_span() const
  {
    return fragment_span_t{
        ptp->fp,
        (*this)[0].fragment_begin,
        (*this)[0].fragment_end,
    };
  }

  fragment_location_t parse_tree_span_t::location() const
  {
    return fragment_location_t{
        ptp->fp,
        (*this)[0].fragment_begin,
    };
  }

  parse_tree_t parse_tree_span_t::copy() const
  {
    array_t<parse_tree_node_t> nodes;
    nodes.reserve(subtree_size());
    for (index_t i = 0; i < subtree_size(); ++i) {
      nodes.push_back((*this)[i]);
    }
    return parse_tree_t{
        .fp    = ptp->fp,
        .nodes = std::move(nodes),
    };
  }

  expected_t<name_id_t> name_id_definition(const lexicon_t& lexicon,
                                           const name_id_t scope_name,
                                           const parse_tree_span_t& pts)
  {
    auto [it, end]   = pts.children_range();
    name_id_t retval = scope_name;
    SILVA_EXPECT(it != end, MINOR);
    if (SILVA_EXPECT_FWD((*it).token()) == lexicon.name_sep) {
      retval = name_id_t{};
      ++it;
    }
    while (it != end) {
      const token_id_t base = SILVA_EXPECT_FWD((*it).token());
      SILVA_EXPECT(base != lexicon.name_sep, MINOR);
      retval = lexicon.sfp->name_id(retval, base);
      ++it;
      if (it != end) {
        const token_id_t expected_sep = SILVA_EXPECT_FWD((*it).token());
        SILVA_EXPECT(expected_sep == lexicon.name_sep, MINOR);
        ++it;
      }
    }
    return retval;
  }

  name_id_ref_t::name_id_ref_t(parse_tree_span_t pts) : pts(std::move(pts)) {}

  void name_id_ref_t::resolve_clear() const
  {
    resolved_name = name_id_t{};
  }

  bool operator==(const name_id_ref_t& lhs, const name_id_ref_t& rhs)
  {
    return lhs.pts == rhs.pts;
  }

  hash_value_t hash_impl(const name_id_ref_t& x)
  {
    return hash_impl(x.pts);
  }
}
