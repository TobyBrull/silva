#include "parse_tree_nursery.hpp"

#include "syntax_ward.hpp"

namespace silva {

  void parse_tree_nursery_t::on_get_state(parse_tree_nursery_state_t& s) const
  {
    s.token_index = token_index;
  }

  void parse_tree_nursery_t::on_set_state(const parse_tree_nursery_state_t& s)
  {
    token_index = s.token_index;
  }

  void parse_tree_nursery_t::on_stake_create_node(parse_tree_node_t& proto_node,
                                                  const name_id_t rule_name) const
  {
    proto_node.rule_name   = rule_name;
    proto_node.token_begin = token_index;
    proto_node.token_end   = token_index + 1;
  }

  void parse_tree_nursery_t::on_stake_add_proto_node(parse_tree_node_t& proto_node,
                                                     const parse_tree_node_t& other) const
  {
    proto_node.token_begin = std::min(proto_node.token_begin, other.token_begin);
    proto_node.token_end   = std::max(proto_node.token_end, other.token_end);
  }

  void parse_tree_nursery_t::on_stake_commit_pre(parse_tree_node_t& proto_node) const
  {
    proto_node.token_end = token_index;
  }

  void parse_tree_nursery_t::on_stake_commit_owning_to_proto(parse_tree_node_t& proto_node) const
  {
    proto_node.rule_name = name_id_root;
  }

  // parse_tree_nursery_t

  unique_ptr_t<parse_tree_t> parse_tree_nursery_t::finish() &&
  {
    return std::make_unique<parse_tree_t>(parse_tree_t{
        .tp    = tp,
        .nodes = std::move(tree),
    });
  }

  parse_tree_nursery_t::parse_tree_nursery_t(syntax_ward_t& sc, tokenization_ptr_t tp)
    : sc(sc), tp(tp), tcp(sc.token_ward().ptr())
  {
  }

  // Token helper functions.

  const index_t parse_tree_nursery_t::num_tokens_left() const
  {
    return tp->tokens.size() - token_index;
  }

  const token_id_t parse_tree_nursery_t::token_id_by(const index_t token_index_offset) const
  {
    return tp->tokens[token_index + token_index_offset];
  }

  const token_info_t* parse_tree_nursery_t::token_data_by(const index_t token_index_offset) const
  {
    const token_id_t token_id_ = token_id_by(token_index_offset);
    return &(tp->context->token_infos[token_id_]);
  }

  token_position_t parse_tree_nursery_t::token_position_by(const index_t token_index_offset) const
  {
    return token_position_t{
        .tp          = tp,
        .token_index = token_index + token_index_offset,
    };
  }
  token_position_t parse_tree_nursery_t::token_position_at(const index_t arg_token_index) const
  {
    return token_position_t{
        .tp          = tp,
        .token_index = arg_token_index,
    };
  }
}
