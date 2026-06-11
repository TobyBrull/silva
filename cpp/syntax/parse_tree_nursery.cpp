#include "parse_tree_nursery.hpp"

#include "syntax_farm.hpp"

namespace silva {

  void parse_tree_nursery_t::on_get_state(parse_tree_nursery_state_t& s) const
  {
    s.fragment_index = fragment_index;
    s.token_index    = tokenization.tokens.size();
  }

  void parse_tree_nursery_t::on_set_state(const parse_tree_nursery_state_t& s)
  {
    fragment_index = s.fragment_index;
    tokenization.tokens.resize(s.token_index);
  }

  void parse_tree_nursery_t::on_stake_ctor(parse_tree_node_t& proto_node) const
  {
    proto_node.token_begin = tokenization.size();
    proto_node.token_end   = tokenization.size();
  }

  void parse_tree_nursery_t::on_stake_create_node(parse_tree_node_t& proto_node,
                                                  const name_id_t rule_name) const
  {
    proto_node.rule_name = rule_name;
  }

  void parse_tree_nursery_t::on_stake_add_proto_node(parse_tree_node_t& proto_node,
                                                     const parse_tree_node_t& other) const
  {
    proto_node.token_begin = std::min(proto_node.token_begin, other.token_begin);
    proto_node.token_end   = std::max(proto_node.token_end, other.token_end);
  }

  void parse_tree_nursery_t::on_stake_commit_pre(parse_tree_node_t& proto_node) const
  {
    proto_node.token_end = tokenization.size();
  }

  void parse_tree_nursery_t::on_stake_commit_owning_to_proto(parse_tree_node_t& proto_node) const
  {
    proto_node.rule_name = name_id_none;
  }

  // parse_tree_nursery_t

  parse_tree_ptr_t parse_tree_nursery_t::finish() &&
  {
    auto tp = sfp->add(std::make_unique<tokenization_t>(std::move(tokenization)));
    auto pt = sfp->add(std::make_unique<parse_tree_t>(parse_tree_t{
        .tp    = std::move(tp),
        .nodes = std::move(tree),
    }));
    return pt;
  }

  parse_tree_nursery_t::parse_tree_nursery_t(fragmentization_ptr_t fp) : sfp(fp->sfp), fp(fp) {}

  // Token helper functions.

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
  parse_tree_nursery_t::fragment_location_at(const index_t arg_token_index) const
  {
    return fragment_location_t{
        .fp             = fp,
        .fragment_index = arg_token_index,
    };
  }
}
