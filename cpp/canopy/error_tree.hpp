#pragma once

#include "assert.hpp"
#include "memento.hpp"

namespace silva {
  enum class tree_event_t {
    INVALID  = 0,
    ON_ENTRY = 0b01,
    ON_EXIT  = 0b10,
    ON_LEAF  = 0b11,
  };
  constexpr bool is_on_entry(tree_event_t);
  constexpr bool is_on_exit(tree_event_t);

  struct tree_branch_t {
    index_t node_index = 0;

    // This node ("node_index") is child number "child_index" of its parent. Zero for the root
    // node.
    index_t child_index = 0;
  };

  struct error_tree_t {
    struct node_t {
      index_t num_children   = 0;
      index_t children_begin = 0;

      memento_buffer_offset_t memento_buffer_offset = 0;
      memento_buffer_offset_t memento_buffer_begin  = 0;
    };
    vector_t<node_t> nodes;

    template<typename Visitor>
      requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
    void visit_subtree(Visitor, index_t start_node_index = 0) const;

    template<typename Visitor>
      requires std::invocable<Visitor, index_t, index_t>
    void visit_children(Visitor, index_t parent_node_index) const;

    template<typename Visitor>
      requires std::invocable<Visitor, index_t, index_t>
    void visit_children_reversed(Visitor, index_t parent_node_index) const;
  };
}

// IMPLEMENTATION

namespace silva {
  constexpr bool is_on_entry(const tree_event_t event)
  {
    const auto retval = (to_int(event) & to_int(tree_event_t::ON_ENTRY));
    return retval != 0;
  }

  constexpr bool is_on_exit(const tree_event_t event)
  {
    const auto retval = (to_int(event) & to_int(tree_event_t::ON_EXIT));
    return retval != 0;
  }

  template<typename Visitor>
    requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
  void error_tree_t::visit_subtree(Visitor visitor, const index_t start_node_index) const
  {
    vector_t<tree_branch_t> path;
    const auto clean_stack_till = [&](const index_t prev_node_index) -> optional_t<index_t> {
      index_t next_child_index = 0;
      while (!path.empty() && prev_node_index <= nodes[path.back().node_index].children_begin) {
        const bool is_leaf = (nodes[path.back().node_index].num_children == 0);
        next_child_index   = path.back().child_index + 1;
        if (!is_leaf) {
          const bool cont = visitor(span_t<const tree_branch_t>{path}, tree_event_t::ON_EXIT);
          if (!cont) {
            return {none};
          }
        }
        path.pop_back();
      }
      return {next_child_index};
    };

    const index_t begin_node_index = nodes[start_node_index].children_begin;
    index_t node_index             = start_node_index;
    while (true) {
      const optional_t<index_t> maybe_new_child_index = clean_stack_till(node_index + 1);
      if (!maybe_new_child_index) {
        return;
      }
      path.push_back({.node_index = node_index, .child_index = maybe_new_child_index.value()});
      const bool is_leaf = (nodes[node_index].num_children == 0);
      if (is_leaf) {
        const bool cont = visitor(span_t<const tree_branch_t>{path}, tree_event_t::ON_LEAF);
        if (!cont) {
          return;
        }
      }
      else {
        const bool cont = visitor(span_t<const tree_branch_t>{path}, tree_event_t::ON_ENTRY);
        if (!cont) {
          return;
        }
      }
      if (node_index <= begin_node_index) {
        break;
      }
      node_index -= 1;
    }

    const optional_t<index_t> maybe_new_child_index = clean_stack_till(begin_node_index);
    if (!maybe_new_child_index) {
      return;
    }
    SILVA_ASSERT(maybe_new_child_index.value() == 1);
    SILVA_ASSERT(path.empty());
    return;
  }

  template<typename Visitor>
    requires std::invocable<Visitor, index_t, index_t>
  void error_tree_t::visit_children(Visitor visitor, index_t parent_node_index) const
  {
    const node_t& parent_node = nodes[parent_node_index];
    index_t curr_node_index   = parent_node_index;
    for (index_t child_index = 0; child_index < parent_node.num_children; ++child_index) {
      curr_node_index -= 1;
      visitor(curr_node_index, child_index);
      curr_node_index = nodes[curr_node_index].children_begin;
    }
  }

  namespace impl {
    template<typename Visitor>
    void error_tree_visit_children_reversed(const error_tree_t& et,
                                            Visitor& visitor,
                                            const index_t parent_node_index,
                                            const index_t prev_child_node_begin,
                                            const index_t child_index)
    {
      const auto& parent_node = et.nodes[parent_node_index];
      if (child_index < parent_node.num_children) {
        const index_t child_node_index      = prev_child_node_begin - 1;
        const index_t next_child_node_begin = et.nodes[child_node_index].children_begin;
        error_tree_visit_children_reversed(et,
                                           visitor,
                                           parent_node_index,
                                           next_child_node_begin,
                                           child_index + 1);
        visitor(child_node_index, child_index);
      }
    }
  }

  template<typename Visitor>
    requires std::invocable<Visitor, index_t, index_t>
  void error_tree_t::visit_children_reversed(Visitor visitor, index_t parent_node_index) const
  {
    impl::error_tree_visit_children_reversed(*this,
                                             visitor,
                                             parent_node_index,
                                             parent_node_index,
                                             0);
  }
}
