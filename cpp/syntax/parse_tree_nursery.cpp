#include "parse_tree_nursery.hpp"

namespace silva {

  void parse_tree_nursery_t::stake_t::add_proto_node(const parse_tree_node_t& other)
  {
    proto_node.num_children += other.num_children;
    proto_node.subtree_size += other.subtree_size;
    proto_node.token_begin = std::min(proto_node.token_begin, other.token_begin);
    proto_node.token_end   = std::max(proto_node.token_end, other.token_end);
  }

  parse_tree_nursery_t::stake_t::stake_t(parse_tree_nursery_t* nursery)
    : nursery(nursery), orig_state(nursery->state()), proto_node{}
  {
    proto_node.subtree_size = 0;
  }

  parse_tree_nursery_t::stake_t::stake_t(stake_t&& other)
    : nursery(std::exchange(other.nursery, nullptr))
    , orig_state(std::exchange(other.orig_state, state_t{}))
    , proto_node(std::exchange(other.proto_node, parse_tree_node_t{}))
  {
  }

  parse_tree_nursery_t::stake_t& parse_tree_nursery_t::stake_t::operator=(stake_t&& other)
  {
    if (this != &other) {
      clear();
      nursery    = std::exchange(other.nursery, nullptr);
      orig_state = std::exchange(other.orig_state, state_t{});
      proto_node = std::exchange(other.proto_node, parse_tree_node_t{});
    }
    return *this;
  }

  void parse_tree_nursery_t::stake_t::create_node(const name_id_t rule_name)
  {
    SILVA_ASSERT(!owns_node);
    SILVA_ASSERT(proto_node.subtree_size == 0);
    SILVA_ASSERT(nursery->tree.size() == orig_state.tree_size);
    owns_node               = true;
    proto_node.subtree_size = 1;
    proto_node.rule_name    = rule_name;
    proto_node.token_begin  = nursery->token_index;
    proto_node.token_end    = nursery->token_index + 1;
    nursery->tree.emplace_back();
  }

  parse_tree_node_t parse_tree_nursery_t::stake_t::commit()
  {
    proto_node.token_end = nursery->token_index;
    if (owns_node) {
      const index_t node_index{orig_state.tree_size};
      nursery->tree[node_index] = proto_node;
      proto_node.num_children   = 1;
      proto_node.rule_name      = name_id_root;
    }
    nursery = nullptr;
    return proto_node;
  }

  void parse_tree_nursery_t::stake_t::clear()
  {
    if (nursery != nullptr) {
      nursery->set_state(orig_state);
    }
    nursery = nullptr;
  }

  parse_tree_nursery_t::stake_t::~stake_t()
  {
    clear();
  }

  parse_tree_nursery_t::parse_tree_nursery_t(shared_ptr_t<const tokenization_t> tokenization)
    : tokenization(tokenization), tcp(tokenization->context)
  {
  }

  parse_tree_nursery_t::state_t::state_t(const parse_tree_nursery_t* nursery)
    : tree_size(nursery->tree.size()), token_index(nursery->token_index)
  {
  }

  void parse_tree_nursery_t::set_state(const state_t& s)
  {
    tree.resize(s.tree_size);
    token_index = s.token_index;
  }

  parse_tree_t parse_tree_nursery_t::finish() &&
  {
    return parse_tree_t{
        .nodes        = std::move(tree),
        .tokenization = std::move(tokenization),
    };
  }

  // Token helper functions.

  const index_t parse_tree_nursery_t::num_tokens_left() const
  {
    return tokenization->tokens.size() - token_index;
  }

  const token_id_t parse_tree_nursery_t::token_id_by(const index_t token_index_offset) const
  {
    return tokenization->tokens[token_index + token_index_offset];
  }

  const token_info_t* parse_tree_nursery_t::token_data_by(const index_t token_index_offset) const
  {
    const token_id_t token_id_ = token_id_by(token_index_offset);
    return &(tokenization->context->token_infos[token_id_]);
  }

  token_position_t parse_tree_nursery_t::token_position_by(const index_t token_index_offset) const
  {
    return token_position_t{
        .tokenization = tokenization.get(),
        .token_index  = token_index + token_index_offset,
    };
  }
  token_position_t parse_tree_nursery_t::token_position_at(const index_t arg_token_index) const
  {
    return token_position_t{
        .tokenization = tokenization.get(),
        .token_index  = arg_token_index,
    };
  }
}
