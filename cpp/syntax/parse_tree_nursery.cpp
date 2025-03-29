#include "parse_tree_nursery.hpp"

namespace silva {

  // parse_tree_sub_t

  void parse_tree_sub_t::operator+=(parse_tree_sub_t&& other)
  {
    num_children += other.num_children;
    subtree_size += other.subtree_size;
    token_begin = std::min(token_begin, other.token_begin);
    token_end   = std::max(token_end, other.token_end);
  }

  // parse_tree_guard_t

  parse_tree_stake_t::parse_tree_stake_t(parse_tree_nursery_t* nursery)
    : nursery(nursery)
    , orig_node_size(nursery->tree.size())
    , orig_token_index(nursery->token_index)
    , sub{}
  {
  }

  parse_tree_stake_t::parse_tree_stake_t(parse_tree_stake_t&& other)
    : nursery(std::exchange(other.nursery, nullptr))
    , orig_node_size(std::exchange(other.orig_node_size, 0))
    , orig_token_index(std::exchange(other.orig_token_index, 0))
    , sub(std::exchange(other.sub, parse_tree_sub_t{}))
  {
  }

  parse_tree_stake_t& parse_tree_stake_t::operator=(parse_tree_stake_t&& other)
  {
    if (this != &other) {
      nursery          = std::exchange(other.nursery, nullptr);
      orig_node_size   = std::exchange(other.orig_node_size, 0);
      orig_token_index = std::exchange(other.orig_token_index, 0);
      sub              = std::exchange(other.sub, parse_tree_sub_t{});
    }
    return *this;
  }

  void parse_tree_stake_t::create_node(const name_id_t rule_name)
  {
    SILVA_ASSERT(!has_node);
    has_node         = true;
    sub.subtree_size = 1;
    sub.token_begin  = nursery->token_index;
    sub.token_end    = nursery->token_index + 1;
    nursery->tree.emplace_back(parse_tree_node_t{.rule_name = rule_name});
  }

  parse_tree_sub_t parse_tree_stake_t::commit()
  {
    sub.token_end = nursery->token_index;
    parse_tree_sub_t retval;
    if (has_node) {
      sub.token_end                              = nursery->token_index;
      nursery->tree[orig_node_size].num_children = sub.num_children;
      nursery->tree[orig_node_size].subtree_size = sub.subtree_size;
      nursery->tree[orig_node_size].token_begin  = sub.token_begin;
      nursery->tree[orig_node_size].token_end    = sub.token_end;

      retval = parse_tree_sub_t{
          .num_children = 1,
          .subtree_size = sub.subtree_size,
          .token_begin  = sub.token_begin,
          .token_end    = sub.token_end,
      };
    }
    else {
      retval = std::move(sub);
    }
    (*this) = parse_tree_stake_t{};
    return retval;
  }

  void parse_tree_stake_t::clear()
  {
    if (nursery != nullptr) {
      nursery->tree.resize(orig_node_size);
      nursery->token_index = orig_token_index;
    }
    nursery          = nullptr;
    orig_node_size   = 0;
    orig_token_index = 0;
    sub              = parse_tree_sub_t{};
  }

  parse_tree_stake_t::~parse_tree_stake_t()
  {
    clear();
  }

  // parse_tree_nursery_t

  parse_tree_nursery_t::parse_tree_nursery_t(shared_ptr_t<const tokenization_t> tokenization)
    : tokenization(tokenization), tcp(tokenization->context)
  {
  }

  parse_tree_t parse_tree_nursery_t::commit_root() &&
  {
    return parse_tree_t{
        .nodes        = std::move(tree),
        .tokenization = std::move(tokenization),
    };
  }

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
