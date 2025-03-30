#pragma once

#include "tree.hpp"

namespace silva {
  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  struct tree_nursery_t {
    vector_t<NodeData> tree;

    tree_nursery_t() = default;

    struct state_t {
      state_t() = default;
      state_t(const tree_nursery_t*);

      index_t tree_size = 0;
    };
    state_t state() const { return state_t{this}; }
    void set_state(const state_t&);

    struct stake_t {
      tree_nursery_t* nursery = nullptr;
      state_t orig_state;
      NodeData proto_node;

      bool owns_node = false;

      stake_t() = default;
      stake_t(tree_nursery_t*);

      stake_t(stake_t&&);
      stake_t& operator=(stake_t&&);
      stake_t(const stake_t&)            = delete;
      stake_t& operator=(const stake_t&) = delete;

      void create_node();
      NodeData& node() const;

      void add_proto_node(const NodeData&);

      NodeData commit();
      void clear();
      ~stake_t();
    };
    [[nodiscard]] stake_t stake() { return stake_t{this}; }

    vector_t<NodeData> finish() &&;

    tree_nursery_t(tree_nursery_t&&)                 = delete;
    tree_nursery_t& operator=(tree_nursery_t&&)      = delete;
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
  tree_nursery_t<NodeData>::stake_t::stake_t(tree_nursery_t* nursery)
    : nursery(nursery), orig_state(nursery->state())
  {
    proto_node.subtree_size = 0;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::state_t::state_t(const tree_nursery_t* nursery)
    : tree_size(nursery->tree.size())
  {
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  void tree_nursery_t<NodeData>::set_state(const state_t& s)
  {
    tree.resize(s.tree_size);
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t::stake_t(stake_t&& other)
    : nursery(std::exchange(other.nursery, nullptr))
  {
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t& tree_nursery_t<NodeData>::stake_t::operator=(stake_t&& other)
  {
    if (this != &other) {
      clear();
      nursery = std::exchange(other.nursery, nullptr);
    }
    return *this;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  void tree_nursery_t<NodeData>::stake_t::create_node()
  {
    SILVA_ASSERT(!owns_node);
    owns_node               = true;
    proto_node.subtree_size = 1;
    nursery->tree.emplace_back();
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  NodeData& tree_nursery_t<NodeData>::stake_t::node() const
  {
    SILVA_ASSERT(owns_node);
    NodeData& retval = nursery->tree[orig_state.tree_size];
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  void tree_nursery_t<NodeData>::stake_t::add_proto_node(const NodeData& other)
  {
    SILVA_ASSERT(owns_node);
    proto_node.num_children += other.num_children;
    proto_node.subtree_size += other.subtree_size;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  NodeData tree_nursery_t<NodeData>::stake_t::commit()
  {
    SILVA_ASSERT(nursery != nullptr);
    NodeData retval;
    if (owns_node) {
      const index_t node_index{orig_state.tree_size};
      nursery->tree[node_index].num_children = proto_node.num_children;
      nursery->tree[node_index].subtree_size = proto_node.subtree_size;

      retval.num_children = 1;
      retval.subtree_size = proto_node.subtree_size;
    }
    else {
      retval = std::move(proto_node);
    }
    nursery = nullptr;
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  void tree_nursery_t<NodeData>::stake_t::clear()
  {
    if (nursery != nullptr) {
      nursery->set_state(orig_state);
    }
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_nursery_t<NodeData>::stake_t::~stake_t()
  {
    clear();
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  vector_t<NodeData> tree_nursery_t<NodeData>::finish() &&
  {
    return std::move(tree);
  }
}
