#include "parse_tree_nursery.hpp"

namespace silva {

  // parse_tree_sub_t

  void parse_tree_sub_t::operator+=(const parse_tree_sub_t& other)
  {
    num_children += other.num_children;
    num_children_total += other.num_children_total;
    token_begin = std::min(token_begin, other.token_begin);
    token_end   = std::max(token_end, other.token_end);
  }

  // parse_tree_guard_t

  parse_tree_guard_t::parse_tree_guard_t(parse_tree_t* pt, index_t* token_index)
    : pt(pt)
    , token_index(token_index)
    , orig_node_size(pt->nodes.size())
    , orig_token_index(*token_index)
    , sub{
          .token_begin = *token_index,
          .token_end   = *token_index + 1,
      }
  {
  }

  parse_tree_guard_t::parse_tree_guard_t(parse_tree_guard_t&& other)
    : pt(std::exchange(other.pt, nullptr))
    , token_index(std::exchange(other.token_index, nullptr))
    , orig_node_size(std::exchange(other.orig_node_size, 0))
    , orig_token_index(std::exchange(other.orig_token_index, 0))
    , sub(std::exchange(other.sub, parse_tree_sub_t{}))
  {
  }

  parse_tree_guard_t& parse_tree_guard_t::operator=(parse_tree_guard_t&& other)
  {
    if (this != &other) {
      pt               = std::exchange(other.pt, nullptr);
      token_index      = std::exchange(other.token_index, nullptr);
      orig_node_size   = std::exchange(other.orig_node_size, 0);
      orig_token_index = std::exchange(other.orig_token_index, 0);
      sub              = std::exchange(other.sub, parse_tree_sub_t{});
    }
    return *this;
  }

  void parse_tree_guard_t::swap(parse_tree_guard_t& other)
  {
    std::swap(pt, other.pt);
    std::swap(token_index, other.token_index);
    std::swap(orig_node_size, other.orig_node_size);
    std::swap(orig_token_index, other.orig_token_index);
    std::swap(sub, other.sub);
  }

  parse_tree_sub_t parse_tree_guard_t::release()
  {
    sub.token_end = *token_index;
    auto retval   = sub;
    (*this)       = parse_tree_guard_t{};
    return retval;
  }

  bool parse_tree_guard_t::is_empty() const
  {
    return !pt;
  }

  void parse_tree_guard_t::reset()
  {
    if (!is_empty()) {
      pt->nodes.resize(orig_node_size);
      *token_index = orig_token_index;
    }
    pt               = nullptr;
    token_index      = nullptr;
    orig_node_size   = 0;
    orig_token_index = 0;
    sub              = parse_tree_sub_t{};
  }

  parse_tree_guard_t::~parse_tree_guard_t()
  {
    reset();
  }

  // parse_tree_guard_for_rule_t

  parse_tree_guard_for_rule_t::parse_tree_guard_for_rule_t(parse_tree_t* pt, index_t* token_index)
    : parse_tree_guard_t(pt, token_index)
  {
    node_index = pt->nodes.size();
    pt->nodes.push_back(parse_tree_t::node_t{{
        .token_begin = *token_index,
        .token_end   = *token_index + 1,
    }});
  }

  void parse_tree_guard_for_rule_t::set_rule_name(const full_name_id_t rule_name)
  {
    pt->nodes[node_index].rule_name = rule_name;
  }

  parse_tree_sub_t parse_tree_guard_for_rule_t::release()
  {
    sub.token_end                      = *token_index;
    pt->nodes[node_index].num_children = sub.num_children;
    pt->nodes[node_index].children_end = node_index + sub.num_children_total + 1;
    pt->nodes[node_index].token_begin  = sub.token_begin;
    pt->nodes[node_index].token_end    = sub.token_end;
    parse_tree_sub_t retval{
        .num_children       = 1,
        .num_children_total = sub.num_children_total + 1,
        .token_begin        = sub.token_begin,
        .token_end          = sub.token_end,
    };
    parse_tree_guard_t::release();
    return retval;
  }

  void parse_tree_guard_for_rule_t::implant(const parse_tree_t& other_pt,
                                            const index_t other_node_index)
  {
    const auto& other_node = other_pt.nodes[other_node_index];
    const index_t len      = other_node.children_end - other_node_index;
    const index_t diff     = node_index - other_node_index;
    pt->nodes[node_index]  = other_node;
    pt->nodes.insert(pt->nodes.end(),
                     other_pt.nodes.begin() + other_node_index + 1,
                     other_pt.nodes.begin() + other_node.children_end);
    for (index_t ni = other_node_index; ni < other_node.children_end; ++ni) {
      pt->nodes[ni].children_end += diff;
    }
    sub.num_children += other_node.num_children;
    sub.num_children_total += len - 1;
    sub.token_begin = std::min(sub.token_begin, other_node.token_begin);
    sub.token_end   = std::max(sub.token_end, other_node.token_end);
  }

  parse_tree_guard_for_rule_t::~parse_tree_guard_for_rule_t() = default;

  // parse_tree_nursery_t

  parse_tree_nursery_t::parse_tree_nursery_t(shared_ptr_t<const tokenization_t> tokenization)
  {
    tcp                 = tokenization->context;
    retval.tokenization = std::move(tokenization);
  }

  const index_t parse_tree_nursery_t::num_tokens_left() const
  {
    return retval.tokenization->tokens.size() - token_index;
  }

  const token_id_t parse_tree_nursery_t::token_id_by(const index_t token_index_offset) const
  {
    return retval.tokenization->tokens[token_index + token_index_offset];
  }

  const token_info_t* parse_tree_nursery_t::token_data_by(const index_t token_index_offset) const
  {
    const token_id_t token_id_ = token_id_by(token_index_offset);
    return &(retval.tokenization->context->token_infos[token_id_]);
  }

  token_position_t parse_tree_nursery_t::token_position_by(const index_t token_index_offset) const
  {
    return token_position_t{
        .tokenization = retval.tokenization.get(),
        .token_index  = token_index + token_index_offset,
    };
  }
  token_position_t parse_tree_nursery_t::token_position_at(const index_t arg_token_index) const
  {
    return token_position_t{
        .tokenization = retval.tokenization.get(),
        .token_index  = arg_token_index,
    };
  }
}
