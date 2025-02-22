#pragma once

#include "expected.hpp"
#include "preprocessor.hpp"
#include "small_vector.hpp"
#include "types.hpp"

namespace silva {

  template<typename NodeData>
  struct tree_t {
    struct node_t : public NodeData {
      index_t num_children = 0;
      index_t children_end = 0;

      friend auto operator<=>(const node_t&, const node_t&) = default;
    };
    vector_t<node_t> nodes;

    bool is_consistent() const;

    // Invokes the Visitor once (ON_LEAF for leaves) or twice (ON_ENTRY and ON_EXIT for non-leaves)
    // for each node. The visitation order is such that ON_ENTRY for a node happens immediately
    // before the first child invocations and ON_EXIT happens immediately after the last child
    // invokation. The "span" always contains a range that describes the path from the root node to
    // the current node.
    //
    // The Visitor is expected to have return type "expected_t<bool>". Any error returned from
    // Visitor stops the traversal and returns the same error from this function. Otherwise, the
    // returned bool is interpreted as whether the visitation should continue; "false" stops the
    // visitation and makes this function return without an error.
    template<typename Visitor>
      requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
    expected_t<void> visit_subtree(Visitor, index_t start_node_index = 0) const;

    // Calls Visitor once for each child of "parent_node_index". The arguments are (1) the
    // node-index of the child node, "child_node_index", and (2) the "child_index", meaning that
    // "child_node_index" is child number "child_index" of "parent_node_index".
    //
    // Regarding the return type of Visitor, see comment from "visit_subtree" applies.
    template<typename Visitor>
      requires std::invocable<Visitor, index_t, index_t>
    expected_t<void> visit_children(Visitor, index_t parent_node_index) const;

    template<typename Visitor>
      requires std::invocable<Visitor, index_t, index_t>
    void visit_children_reversed(Visitor, index_t parent_node_index) const;

    // Get the indexes of the children of "parent_node_index" but only if the number of children
    // matches "N".
    template<index_t N>
    expected_t<array_t<index_t, N>> get_children(index_t parent_node_index) const;

    // Get the indexes of the children of "parent_node_index" but only if the number of children
    // matches "N".
    template<index_t N>
    expected_t<small_vector_t<index_t, N>> get_children_up_to(index_t parent_node_index) const;
  };

  template<typename NodeData, typename NodeDataFunc>
  expected_t<string_t> tree_to_string(const tree_t<NodeData>&, NodeDataFunc);

  template<typename NodeData, typename NodeDataFunc>
  expected_t<string_t> tree_to_graphviz(const tree_t<NodeData>&, NodeDataFunc);

  template<typename NodeData>
  struct tree_inv_t {
    struct node_t : public NodeData {
      index_t num_children   = 0;
      index_t children_begin = 0;

      friend auto operator<=>(const node_t&, const node_t&) = default;
    };
    vector_t<node_t> nodes;

    bool is_consistent() const;

    template<typename Visitor>
      requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
    expected_t<void> visit_subtree(Visitor, index_t start_node_index = 0) const;

    template<typename Visitor>
      requires std::invocable<Visitor, index_t, index_t>
    expected_t<void> visit_children(Visitor, index_t parent_node_index) const;

    template<typename Visitor>
      requires std::invocable<Visitor, index_t, index_t>
    void visit_children_reversed(Visitor, index_t parent_node_index) const;

    template<index_t N>
    expected_t<array_t<index_t, N>> get_children(index_t parent_node_index) const;

    template<index_t N>
    expected_t<small_vector_t<index_t, N>> get_children_up_to(index_t parent_node_index) const;
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename NodeData>
  template<typename Visitor>
    requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
  expected_t<void> tree_t<NodeData>::visit_subtree(Visitor visitor,
                                                   const index_t start_node_index) const
  {
    vector_t<tree_branch_t> path;
    const auto clean_stack_till =
        [&](const index_t new_node_index) -> expected_t<optional_t<index_t>> {
      index_t next_child_index = 0;
      while (!path.empty() && nodes[path.back().node_index].children_end <= new_node_index) {
        const bool is_leaf = (nodes[path.back().node_index].num_children == 0);
        next_child_index   = path.back().child_index + 1;
        if (!is_leaf) {
          const bool cont =
              SILVA_EXPECT_FWD(visitor(span_t<const tree_branch_t>{path}, tree_event_t::ON_EXIT));
          if (!cont) {
            return {none};
          }
        }
        path.pop_back();
      }
      return {next_child_index};
    };

    const index_t end_node_index = nodes[start_node_index].children_end;
    for (index_t node_index = start_node_index; node_index < end_node_index; ++node_index) {
      const optional_t<index_t> maybe_new_child_index =
          SILVA_EXPECT_FWD(clean_stack_till(node_index));
      if (!maybe_new_child_index) {
        return {};
      }
      path.push_back({.node_index = node_index, .child_index = maybe_new_child_index.value()});
      const bool is_leaf = (nodes[node_index].num_children == 0);
      if (is_leaf) {
        const bool cont =
            SILVA_EXPECT_FWD(visitor(span_t<const tree_branch_t>{path}, tree_event_t::ON_LEAF));
        if (!cont) {
          return {};
        }
      }
      else {
        const bool cont =
            SILVA_EXPECT_FWD(visitor(span_t<const tree_branch_t>{path}, tree_event_t::ON_ENTRY));
        if (!cont) {
          return {};
        }
      }
    }

    const optional_t<index_t> maybe_new_child_index =
        SILVA_EXPECT_FWD(clean_stack_till(end_node_index));
    if (!maybe_new_child_index) {
      return {};
    }
    SILVA_EXPECT(maybe_new_child_index.value() == 1,
                 ASSERT,
                 "Invalid tree at ",
                 SILVA_CPP_LOCATION);
    SILVA_EXPECT(path.empty(), ASSERT, "Path not empty at " SILVA_CPP_LOCATION);
    return {};
  }

  template<typename NodeData>
  template<typename Visitor>
    requires std::invocable<Visitor, index_t, index_t>
  expected_t<void> tree_t<NodeData>::visit_children(Visitor visitor,
                                                    const index_t parent_node_index) const
  {
    const node_t& node       = nodes[parent_node_index];
    index_t child_node_index = parent_node_index + 1;
    for (index_t child_index = 0; child_index < node.num_children; ++child_index) {
      const bool cont = SILVA_EXPECT_FWD(visitor(child_node_index, child_index));
      if (!cont) {
        break;
      }
      child_node_index = nodes[child_node_index].children_end;
    }
    return {};
  }

  template<typename NodeData>
  template<index_t N>
  expected_t<array_t<index_t, N>>
  tree_t<NodeData>::get_children(const index_t parent_node_index) const
  {
    const node_t& node = nodes[parent_node_index];
    SILVA_EXPECT(node.num_children == N, MAJOR);
    array_t<index_t, N> retval;
    SILVA_EXPECT_FWD(visit_children(
        [&](const index_t child_node_index, const index_t child_num) -> expected_t<bool> {
          retval[child_num] = child_node_index;
          return true;
        },
        parent_node_index));
    return {std::move(retval)};
  }

  template<typename NodeData>
  template<index_t N>
  expected_t<small_vector_t<index_t, N>>
  tree_t<NodeData>::get_children_up_to(const index_t parent_node_index) const
  {
    const node_t& node = nodes[parent_node_index];
    SILVA_EXPECT(node.num_children <= N, MAJOR);
    small_vector_t<index_t, N> retval;
    SILVA_EXPECT_FWD(visit_children(
        [&](const index_t child_node_index, const index_t child_num) -> expected_t<bool> {
          retval.emplace_back(child_node_index);
          return true;
        },
        parent_node_index));
    return {std::move(retval)};
  }

  template<typename NodeData, typename NodeDataFunc>
  expected_t<string_t> tree_to_string(const tree_t<NodeData>& tree, NodeDataFunc node_data_func)
  {
    string_t curr_line;
    string_t retval;
    auto result = tree.visit_subtree(
        [&](const span_t<const tree_branch_t> path, const tree_event_t event) -> expected_t<bool> {
          if (!is_on_entry(event)) {
            return true;
          }
          SILVA_EXPECT(!path.empty(), ASSERT, "Empty path at " SILVA_CPP_LOCATION);
          const typename tree_t<NodeData>::node_t& node = tree.nodes[path.back().node_index];
          curr_line.assign(2 * (path.size() - 1), ' ');
          curr_line += fmt::format("[{}]", path.back().child_index);
          node_data_func(curr_line, node);
          retval += curr_line;
          retval += '\n';
          return true;
        });
    SILVA_EXPECT_FWD(std::move(result));
    return retval;
  }

  template<typename NodeData, typename NodeDataFunc>
  expected_t<string_t> tree_to_graphviz(const tree_t<NodeData>& tree, NodeDataFunc node_data_func)
  {
    string_t retval;
    retval += "digraph parse_tree {\n";
    auto result = tree.visit_subtree(
        [&](const span_t<const tree_branch_t> path, const tree_event_t event) -> expected_t<bool> {
          if (!is_on_entry(event)) {
            return true;
          }
          SILVA_EXPECT(!path.empty(), ASSERT, "Empty path at " SILVA_CPP_LOCATION);
          const typename tree_t<NodeData>::node_t& node = tree.nodes[path.back().node_index];
          string_t node_name{"/"};
          if (path.size() >= 2) {
            string_t parent_node_name = "/";
            for (index_t i = 1; i < path.size() - 1; ++i) {
              parent_node_name += fmt::format("{}/", path[i].child_index);
            }
            node_name = fmt::format("{}{}/", parent_node_name, path.back().child_index);
            retval += fmt::format("  \"{}\" -> \"{}\"\n", parent_node_name, node_name);
          }
          retval += fmt::format("  \"{}\" [label=\"[{}]{}\"]\n",
                                node_name,
                                path.back().child_index,
                                node_data_func(node));
          return true;
        });
    SILVA_EXPECT_FWD(std::move(result));
    retval += "}";
    return retval;
  }
}
