#pragma once

#include "tree.hpp"

namespace silva {
  struct tree_nursery_state_t {
    index_t tree_size = 0;
  };

  template<typename NodeType, typename StateType, typename Derived>
  struct tree_nursery_t {
    static_assert(std::derived_from<NodeType, tree_node_t>);
    static_assert(std::derived_from<StateType, tree_nursery_state_t>);

    vector_t<NodeType> tree;

    tree_nursery_t();

    StateType get_state(this const auto&);
    void set_state(this auto&, const StateType&);

    struct stake_t {
      Derived* nursery = nullptr;
      StateType orig_state;
      NodeType proto_node;

      bool owns_node = false;

      stake_t(Derived*);

      // Move operations and default ctor could be implemented.
      stake_t(stake_t&&)            = delete;
      stake_t& operator=(stake_t&&) = delete;

      stake_t(const stake_t&)            = delete;
      stake_t& operator=(const stake_t&) = delete;

      template<typename... Args>
      void create_node(Args&&...);

      void add_proto_node(const NodeType&);

      NodeType commit();
      void clear();
      ~stake_t();
    };
    [[nodiscard]] stake_t stake(this auto& self) { return stake_t{&self}; }

    vector_t<NodeType> finish() &&;

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

  template<typename NodeType, typename StateType, typename Derived>
  tree_nursery_t<NodeType, StateType, Derived>::tree_nursery_t()
  {
    static_assert(std::derived_from<Derived, tree_nursery_t>);
  }

  // state_t

  template<typename NodeType, typename StateType, typename Derived>
  StateType tree_nursery_t<NodeType, StateType, Derived>::get_state(this const auto& self)
  {
    tree_nursery_state_t tns{.tree_size = index_t(self.tree.size())};
    StateType retval{tns};

    if constexpr (requires(Derived d) { d.on_get_state(retval); }) {
      self.on_get_state(retval);
    }

    return retval;
  }

  template<typename NodeType, typename StateType, typename Derived>
  void tree_nursery_t<NodeType, StateType, Derived>::set_state(this auto& self,
                                                               const StateType& state)
  {
    self.tree.resize(state.tree_size);

    if constexpr (requires(Derived d) { d.on_set_state(state); }) {
      self.on_set_state(state);
    }
  }

  // stake_t

  template<typename NodeType, typename StateType, typename Derived>
  tree_nursery_t<NodeType, StateType, Derived>::stake_t::stake_t(Derived* nursery)
    : nursery(nursery), orig_state(nursery->get_state())
  {
    proto_node.subtree_size = 0;
  }

  template<typename NodeType, typename StateType, typename Derived>
  template<typename... Args>
  void tree_nursery_t<NodeType, StateType, Derived>::stake_t::create_node(Args&&... args)
  {
    SILVA_ASSERT(!owns_node);
    SILVA_ASSERT(proto_node.subtree_size == 0);
    SILVA_ASSERT(nursery->tree.size() == orig_state.tree_size);
    owns_node               = true;
    proto_node.subtree_size = 1;
    nursery->tree.emplace_back();

    if constexpr (requires(Derived d) {
                    d.on_stake_create_node(proto_node, std::forward<Args>(args)...);
                  }) {
      nursery->on_stake_create_node(proto_node, std::forward<Args>(args)...);
    }
  }

  template<typename NodeType, typename StateType, typename Derived>
  void tree_nursery_t<NodeType, StateType, Derived>::stake_t::add_proto_node(const NodeType& other)
  {
    proto_node.num_children += other.num_children;
    proto_node.subtree_size += other.subtree_size;

    if constexpr (requires(Derived d) { d.on_stake_add_proto_node(proto_node, other); }) {
      nursery->on_stake_add_proto_node(proto_node, other);
    }
  }

  template<typename NodeType, typename StateType, typename Derived>
  NodeType tree_nursery_t<NodeType, StateType, Derived>::stake_t::commit()
  {
    SILVA_ASSERT(nursery != nullptr);

    if constexpr (requires(Derived d) { d.on_stake_commit_pre(proto_node); }) {
      nursery->on_stake_commit_pre(proto_node);
    }

    if (owns_node) {
      nursery->tree[orig_state.tree_size] = proto_node;
      proto_node.num_children             = 1;

      if constexpr (requires(Derived d) { d.on_stake_commit_owning_to_proto(proto_node); }) {
        nursery->on_stake_commit_owning_to_proto(proto_node);
      }
    }
    nursery = nullptr;
    return proto_node;
  }

  template<typename NodeType, typename StateType, typename Derived>
  void tree_nursery_t<NodeType, StateType, Derived>::stake_t::clear()
  {
    if (nursery != nullptr) {
      nursery->set_state(orig_state);
    }
    nursery = nullptr;
  }

  template<typename NodeType, typename StateType, typename Derived>
  tree_nursery_t<NodeType, StateType, Derived>::stake_t::~stake_t()
  {
    clear();
  }

  // tree_nursery_t

  template<typename NodeType, typename StateType, typename Derived>
  vector_t<NodeType> tree_nursery_t<NodeType, StateType, Derived>::finish() &&
  {
    return std::move(tree);
  }
}
