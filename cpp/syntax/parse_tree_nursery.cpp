#include "parse_tree_nursery.hpp"

#include "syntax_farm.hpp"

namespace silva {
  expected_t<parse_tree_node_t> parse_tree_nursery_t::parse_literal(const fragmented_token_t& ft)
  {
    auto ss_rule                = stake();
    const index_t n             = ft.items.size();
    const index_t orig_frag_idx = fragment_index;
    SILVA_EXPECT(num_fragments_left() >= n,
                 MINOR,
                 "[{}] not enough fragments left when expecting {}",
                 fragment_location_at(orig_frag_idx),
                 sfp->token_id_wrap(ft.token_id));
    SILVA_EXPECT(n > 0, ASSERT);
    for (index_t i = 0; i < n; ++i) {
      auto maybe_curr_cp = fp->get_unique_codepoint(fragment_index);
      if (!maybe_curr_cp.has_value()) {
        maybe_curr_cp.error().clear();
        SILVA_EXPECT(false,
                     MINOR,
                     "[{}] expected {}",
                     fragment_location_at(orig_frag_idx),
                     sfp->token_id_wrap(ft.token_id));
      }
      const unicode::codepoint_t curr_cp = *maybe_curr_cp;
      SILVA_EXPECT(ft.items[i].codepoint == curr_cp,
                   MINOR,
                   "[{}] expected {}",
                   fragment_location_at(orig_frag_idx),
                   sfp->token_id_wrap(ft.token_id));
      fragment_index += 1;
    }
    if (ft.as_identifier) {
      SILVA_EXPECT(num_fragments_left() == 0 ||
                       !is_fragment_category_id_continue(fragment_category_by()),
                   MINOR,
                   "[{}] expected {}",
                   fragment_location_at(orig_frag_idx),
                   sfp->token_id_wrap(ft.token_id));
    }
    return ss_rule.commit();
  }

  void parse_tree_nursery_t::on_get_state(parse_tree_nursery_state_t& s) const
  {
    s.fragment_index = fragment_index;
  }

  void parse_tree_nursery_t::on_set_state(const parse_tree_nursery_state_t& s)
  {
    fragment_index = s.fragment_index;
  }

  void parse_tree_nursery_t::on_stake_ctor(parse_tree_node_t& proto_node) const
  {
    proto_node.fragment_begin = fragment_index;
    proto_node.fragment_end   = fragment_index;
  }

  void parse_tree_nursery_t::on_stake_create_node(parse_tree_node_t& proto_node,
                                                  const name_id_t rule_name,
                                                  const bool allow_token) const
  {
    proto_node.rule_name   = rule_name;
    proto_node.allow_token = allow_token;
  }

  void parse_tree_nursery_t::on_stake_add_proto_node(parse_tree_node_t& proto_node,
                                                     const parse_tree_node_t& other) const
  {
    proto_node.fragment_begin = std::min(proto_node.fragment_begin, other.fragment_begin);
    proto_node.fragment_end   = std::max(proto_node.fragment_end, other.fragment_end);
  }

  void parse_tree_nursery_t::on_stake_commit_pre(parse_tree_node_t& proto_node) const
  {
    proto_node.fragment_end = fragment_index;
  }

  void parse_tree_nursery_t::on_stake_commit_owning_to_proto(parse_tree_node_t& proto_node) const
  {
    proto_node.rule_name = name_id_t{};
  }

  // parse_tree_nursery_t

  expected_t<void> parse_tree_nursery_t::init(const name_id_t language_name,
                                              const lexicon_t& lexicon)
  {
    fragment_index = fs.begin;
    SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(language_name, LANG_BEGIN);
    SILVA_EXPECT_PARSE(language_name,
                       fp->fragments[fs.end - 1].category == fragment_category_t::LANG_END,
                       "fragment_span_t doesn't properly point to language");
    return {};
  }

  expected_t<parse_tree_ptr_t> parse_tree_nursery_t::finish() &&
  {
    SILVA_EXPECT(fragment_index + 1 == fs.end, MINOR, "[{}] vs [{}]", fragment_index, fs.end);
    auto pt = sfp->add(std::make_unique<parse_tree_t>(parse_tree_t{
        .fp    = fp,
        .nodes = std::move(tree),
    }));
    return pt;
  }

  parse_tree_nursery_t::parse_tree_nursery_t(fragment_span_t fs)
    : sfp(fs.fp->sfp), fp(fs.fp), fs(fs)
  {
  }

  // Fragment helper functions.

  index_t parse_tree_nursery_t::num_fragments_left() const
  {
    return fp->size() - fragment_index;
  }
  const fragment_t* parse_tree_nursery_t::fragment_by(const index_t idx_offset) const
  {
    return &(fp->fragments[fragment_index + idx_offset]);
  }
  unicode::codepoint_t
  parse_tree_nursery_t::fragment_unique_codepoint_or_zero_by(const index_t idx_offset) const
  {
    auto res = fp->get_unique_codepoint(fragment_index + idx_offset);
    if (res.has_value()) {
      return *res;
    }
    else {
      return U'\0';
    }
  }
  fragment_category_t parse_tree_nursery_t::fragment_category_by(const index_t idx_offset) const
  {
    return fragment_by(idx_offset)->category;
  }
  fragment_location_t parse_tree_nursery_t::fragment_location_by(const index_t idx_offset) const
  {
    return fragment_location_t{
        .fp             = fp,
        .fragment_index = fragment_index + idx_offset,
    };
  }
  fragment_location_t
  parse_tree_nursery_t::fragment_location_at(const index_t arg_fragment_index) const
  {
    return fragment_location_t{
        .fp             = fp,
        .fragment_index = arg_fragment_index,
    };
  }
}
