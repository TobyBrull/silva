#include "parse_tree_nursery.hpp"

namespace silva {

  // parse_tree_sub_t

  void parse_tree_sub_t::operator+=(parse_tree_sub_t&& other)
  {
    num_children += other.num_children;
    num_children_total += other.num_children_total;
  }

  // parse_tree_guard_t

  parse_tree_guard_t::parse_tree_guard_t(parse_tree_t* pt, index_t* token_index)
    : pt(pt)
    , token_index(token_index)
    , orig_node_size(pt->nodes.size())
    , orig_token_index(*token_index)
  {
  }

  parse_tree_sub_t parse_tree_guard_t::release()
  {
    pt = nullptr;
    return std::move(sub);
  }

  bool parse_tree_guard_t::is_released() const
  {
    return !pt;
  }

  parse_tree_guard_t::~parse_tree_guard_t()
  {
    if (!is_released()) {
      pt->nodes.resize(orig_node_size);
      *token_index = orig_token_index;
    }
  }

  // parse_tree_guard_for_rule_t

  parse_tree_guard_for_rule_t::parse_tree_guard_for_rule_t(parse_tree_t* pt, index_t* token_index)
    : parse_tree_guard_t(pt, token_index)
  {
    node_index = pt->nodes.size();
    pt->nodes.push_back(parse_tree_t::node_t{.token_index = *token_index});
  }

  void parse_tree_guard_for_rule_t::set_rule_index(const index_t rule_index)
  {
    pt->nodes[node_index].rule_index = rule_index;
  }

  parse_tree_sub_t parse_tree_guard_for_rule_t::release()
  {
    pt->nodes[node_index].num_children = sub.num_children;
    pt->nodes[node_index].children_end = node_index + sub.num_children_total + 1;
    parse_tree_sub_t retval{
        .num_children       = 1,
        .num_children_total = sub.num_children_total + 1,
    };
    parse_tree_guard_t::release();
    return retval;
  }

  parse_tree_guard_for_rule_t::~parse_tree_guard_for_rule_t() = default;

  // parse_tree_nursery_t

  parse_tree_nursery_t::parse_tree_nursery_t(const tokenization_t* tokenization,
                                             const_ptr_t<parse_root_t> parse_root)
  {
    retval.tokenization = tokenization;
    retval.root         = std::move(parse_root);
  }

  const index_t parse_tree_nursery_t::num_tokens_left() const
  {
    return retval.tokenization->tokens.size() - token_index;
  }

  const token_info_index_t parse_tree_nursery_t::token_id_by(const index_t token_index_offset) const
  {
    return retval.tokenization->tokens[token_index + token_index_offset];
  }

  const token_info_t* parse_tree_nursery_t::token_data_by(const index_t token_index_offset) const
  {
    const token_info_index_t token_id_ = token_id_by(token_index_offset);
    return &(retval.tokenization->context->token_infos[token_id_]);
  }

  token_position_t parse_tree_nursery_t::token_position_by(const index_t token_index_offset) const
  {
    return token_position_t{
        .tokenization = retval.tokenization,
        .token_index  = token_index + token_index_offset,
    };
  }
  token_position_t parse_tree_nursery_t::token_position_at(const index_t arg_token_index) const
  {
    return token_position_t{
        .tokenization = retval.tokenization,
        .token_index  = arg_token_index,
    };
  }
}
