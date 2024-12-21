#pragma once

#include "assert.hpp"
#include "tree_base.hpp"

namespace silva {
  // This is
  template<typename T>
  struct error_tree_t {
    struct node_t {
      index_t num_children   = 0;
      index_t children_begin = 0;
      T message{};
    };
    vector_t<node_t> nodes;

    template<typename Visitor>
      requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
    void visit_subtree(Visitor, index_t start_node_index = 0) const;
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  template<typename Visitor>
    requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
  void error_tree_t<T>::visit_subtree(Visitor visitor, const index_t start_node_index) const
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
}
