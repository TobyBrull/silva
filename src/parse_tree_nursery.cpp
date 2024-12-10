#include "parse_tree_nursery.hpp"

namespace silva {

  // parse_tree_sub_t

  void parse_tree_sub_t::operator+=(const parse_tree_sub_t& other)
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
    return sub;
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
    pt->nodes.push_back(parse_tree_t::node_t{
        .token_index = *token_index,
    });
  }

  void parse_tree_guard_for_rule_t::set_rule_index(const index_t rule_index)
  {
    pt->nodes[node_index].rule_index = rule_index;
  }

  parse_tree_sub_t parse_tree_guard_for_rule_t::release()
  {
    pt->nodes[node_index].num_children = sub.num_children;
    pt->nodes[node_index].children_end = node_index + sub.num_children_total + 1;
    const parse_tree_sub_t retval{
        .num_children       = 1,
        .num_children_total = sub.num_children_total + 1,
    };
    parse_tree_guard_t::release();
    return retval;
  }

  parse_tree_guard_for_rule_t::~parse_tree_guard_for_rule_t() = default;

  // parse_tree_nursery_t

  parse_tree_nursery_t::parse_tree_nursery_t(const_ptr_t<tokenization_t> tokenization,
                                             const parse_root_t* parse_root)
  {
    retval.tokenization = std::move(tokenization);
    retval.root         = parse_root;
  }

  optional_t<token_id_t> parse_tree_nursery_t::lookup_token(const string_view_t str)
  {
    return retval.tokenization->lookup_token(str);
  }

  const index_t parse_tree_nursery_t::num_tokens_left() const
  {
    return retval.tokenization->tokens.size() - token_index;
  }

  const token_id_t parse_tree_nursery_t::token_id(const index_t token_index_offset) const
  {
    return retval.tokenization->tokens[token_index + token_index_offset];
  }

  const tokenization_t::token_data_t*
  parse_tree_nursery_t::token_data(const index_t token_index_offset) const
  {
    const token_id_t token_id_ = token_id(token_index_offset);
    return &(retval.tokenization->token_datas[token_id_]);
  }
}
