#pragma once

#include "tree.hpp"

namespace silva {
  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  struct tree_nursery_t {
    vector_t<NodeData> tree;

    vector_t<index_t> create_node_stack;

    struct stake_t {
      tree_nursery_t* nursery         = nullptr;
      index_t create_node_stack_index = 0;

      stake_t() = default;
      stake_t(tree_nursery_t*, index_t);

      stake_t(stake_t&&);
      stake_t& operator=(stake_t&&);
      stake_t(const stake_t&)            = delete;
      stake_t& operator=(const stake_t&) = delete;

      NodeData& node() const;

      void commit();
      void clear();
      ~stake_t();
    };
    stake_t root;

    stake_t stake();

    vector_t<NodeData> commit_root() &&;

    tree_nursery_t();

    tree_nursery_t(tree_nursery_t&&)            = delete;
    tree_nursery_t& operator=(tree_nursery_t&&) = delete;

    tree_nursery_t(const tree_nursery_t&)            = delete;
    tree_nursery_t& operator=(const tree_nursery_t&) = delete;
  };

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  struct tree_nursery_inverted_t {
    struct extended_node_data_t : public NodeData {
      index_t last_child   = 0;
      index_t prev_sibling = 0;
    };
    vector_t<extended_node_data_t> tree;
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t::stake_t(stake_t&& other)
    : nursery(std::exchange(other.nursery, nullptr))
    , create_node_stack_index(std::exchange(other.create_node_stack_index, 0))
  {
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t& tree_nursery_t<NodeData>::stake_t::operator=(stake_t&& other)
  {
    if (this != &other) {
      clear();
      nursery                 = std::exchange(other.nursery, nullptr);
      create_node_stack_index = std::exchange(other.create_node_stack_index, 0);
    }
    return *this;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t::~stake_t()
  {
    clear();
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  NodeData& tree_nursery_t<NodeData>::stake_t::node() const
  {
    const index_t node_index = nursery->create_node_stack[create_node_stack_index];
    NodeData& retval         = nursery->tree[node_index];
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  void tree_nursery_t<NodeData>::stake_t::commit()
  {
    SILVA_ASSERT(nursery != nullptr);
    SILVA_ASSERT(create_node_stack_index > 0);
    const index_t prev_node_index = nursery->create_node_stack[create_node_stack_index - 1];
    const index_t curr_node_index = nursery->create_node_stack[create_node_stack_index];
    NodeData& prev_node           = nursery->tree[prev_node_index];
    NodeData& curr_node           = nursery->tree[curr_node_index];
    prev_node.num_children += 1;
    prev_node.subtree_size += curr_node.subtree_size;
    nursery->create_node_stack.resize(create_node_stack_index);
    nursery = nullptr;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  void tree_nursery_t<NodeData>::stake_t::clear()
  {
    if (nursery != nullptr) {
      const index_t node_index = nursery->create_node_stack[create_node_stack_index];
      nursery->tree.resize(node_index);
      nursery->create_node_stack.resize(create_node_stack_index);
      nursery                 = nullptr;
      create_node_stack_index = 0;
    }
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t::stake_t(tree_nursery_t* nursery,
                                             const index_t create_node_stack_index)
    : nursery(nursery), create_node_stack_index(create_node_stack_index)
  {
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t tree_nursery_t<NodeData>::stake()
  {
    tree_nursery_t<NodeData>::stake_t retval(this, create_node_stack.size());
    create_node_stack.push_back(tree.size());
    tree.emplace_back();
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  vector_t<NodeData> tree_nursery_t<NodeData>::commit_root() &&
  {
    root.nursery = nullptr;
    return std::move(tree);
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::tree_nursery_t() : root(stake())
  {
  }
}
