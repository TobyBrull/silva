#pragma once

#include "canopy/iterator_facade.hpp"
#include "expected.hpp"
#include "preprocessor.hpp"
#include "small_vector.hpp"
#include "types.hpp"

namespace silva {

  struct tree_node_t {
    // Number of direct children of this node.
    index_t num_children = 0;

    // Size of the subtree rooted in this node, including this node.
    index_t subtree_size = 1;

    friend auto operator<=>(const tree_node_t&, const tree_node_t&) = default;
  };

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  struct tree_span_t {
    NodeData* root = nullptr;
    index_t stride = 0;

    tree_span_t() = default;
    tree_span_t(NodeData* root, index_t stride);
    explicit tree_span_t(span_t<NodeData>);
    explicit tree_span_t(vector_t<NodeData>&);

    index_t size() const;

    auto& operator[](this auto&&, index_t);

    tree_span_t sub_tree_span_at(index_t) const;

    auto children_range(this auto&&);

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
    expected_t<void> visit_subtree(Visitor) const;

    // Get the indexes of the children of "parent_node_index" but only if the number of children
    // matches "N".
    vector_t<index_t> get_children_dyn() const;

    // Get the indexes of the children of "parent_node_index" but only if the number of children
    // matches "N".
    template<index_t N>
    expected_t<array_t<index_t, N>> get_children() const;

    // Get the indexes of the children of "parent_node_index" but only if the number of children
    // matches "N".
    template<index_t N>
    expected_t<small_vector_t<index_t, N>> get_children_up_to() const;

    friend bool operator==(const tree_span_t&, const tree_span_t&) = default;
    // friend hash_value_t hash_impl(const tree_span_t& x);

    template<typename NodeDataFunc>
    expected_t<string_t> to_string(NodeDataFunc) const;

    template<typename NodeDataFunc>
    expected_t<string_t> to_graphviz(NodeDataFunc) const;
  };

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  struct tree_span_child_iter_t : public iterator_facade_t {
    tree_span_t<NodeData> tree_span;
    index_t pos         = 1;
    index_t child_index = 0;

    pair_t<index_t, index_t> dereference() const;
    void increment();
    friend auto operator<=>(const tree_span_child_iter_t&, const tree_span_child_iter_t&) = default;
  };

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  using tree_t = vector_t<NodeData>;
}

// IMPLEMENTATION

namespace silva {

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  pair_t<index_t, index_t> tree_span_child_iter_t<NodeData>::dereference() const
  {
    return {pos, child_index};
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  void tree_span_child_iter_t<NodeData>::increment()
  {
    pos += tree_span[pos].subtree_size;
    child_index += 1;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_span_t<NodeData>::tree_span_t(NodeData* root, index_t stride) : root{root}, stride{stride}
  {
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_span_t<NodeData>::tree_span_t(span_t<NodeData> vec) : root{vec.data()}, stride{1}
  {
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_span_t<NodeData>::tree_span_t(vector_t<NodeData>& vec) : root{vec.data()}, stride{1}
  {
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  index_t tree_span_t<NodeData>::size() const
  {
    return root->subtree_size;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  auto& tree_span_t<NodeData>::operator[](this auto&& self, const index_t i)
  {
    return *(self.root + self.stride * i);
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  tree_span_t<NodeData> tree_span_t<NodeData>::sub_tree_span_at(const index_t pos) const
  {
    return {&((*this)[pos]), stride};
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  auto tree_span_t<NodeData>::children_range(this auto&& self)
  {
    static_assert(std::input_or_output_iterator<tree_span_child_iter_t<NodeData>>);
    tree_span_child_iter_t<NodeData> begin{
        .tree_span   = self,
        .pos         = 1,
        .child_index = 0,
    };
    tree_span_child_iter_t<NodeData> end{
        .tree_span   = self,
        .pos         = self.root->subtree_size,
        .child_index = self.root->num_children,
    };
    return std::ranges::subrange<tree_span_child_iter_t<NodeData>,
                                 tree_span_child_iter_t<NodeData>>(begin, end);
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  template<typename Visitor>
    requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
  expected_t<void> tree_span_t<NodeData>::visit_subtree(Visitor visitor) const
  {
    vector_t<tree_branch_t> path;
    const auto clean_stack_till =
        [&](const index_t new_node_index) -> expected_t<optional_t<index_t>> {
      index_t next_child_index = 0;
      while (!path.empty() &&
             path.back().node_index + (*this)[path.back().node_index].subtree_size <=
                 new_node_index) {
        const bool is_leaf = ((*this)[path.back().node_index].num_children == 0);
        next_child_index   = path.back().child_index + 1;
        if (!is_leaf) {
          const bool cont =
              SILVA_EXPECT_FWD(visitor(span_t<const tree_branch_t>{path}, tree_event_t::ON_EXIT));
          if (!cont) {
            return {{none}};
          }
        }
        path.pop_back();
      }
      return {next_child_index};
    };

    const index_t end_node_index = (*this)[0].subtree_size;
    for (index_t node_index = 0; node_index < end_node_index; ++node_index) {
      const optional_t<index_t> maybe_new_child_index =
          SILVA_EXPECT_FWD(clean_stack_till(node_index));
      if (!maybe_new_child_index) {
        return {};
      }
      path.push_back({.node_index = node_index, .child_index = maybe_new_child_index.value()});
      const bool is_leaf = ((*this)[node_index].num_children == 0);
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
    requires std::derived_from<NodeData, tree_node_t>
  vector_t<index_t> tree_span_t<NodeData>::get_children_dyn() const
  {
    const auto& node = (*this)[0];
    vector_t<index_t> retval;
    retval.reserve(node.num_children);
    for (const auto [child_node_index, child_index]: children_range()) {
      retval.emplace_back(child_node_index);
    }
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  template<index_t N>
  expected_t<array_t<index_t, N>> tree_span_t<NodeData>::get_children() const
  {
    const auto& node = (*this)[0];
    SILVA_EXPECT(node.num_children == N,
                 MAJOR,
                 "expected {} children, got {}",
                 N,
                 node.num_children);
    array_t<index_t, N> retval;
    for (const auto [child_node_index, child_index]: children_range()) {
      retval[child_index] = child_node_index;
    }
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  template<index_t N>
  expected_t<small_vector_t<index_t, N>> tree_span_t<NodeData>::get_children_up_to() const
  {
    const auto& node = (*this)[0];
    SILVA_EXPECT(node.num_children <= N, MAJOR);
    small_vector_t<index_t, N> retval;
    for (const auto [child_node_index, child_index]: children_range()) {
      retval.emplace_back(child_node_index);
    }
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  hash_value_t hash_impl(const tree_span_t<NodeData>& x)
  {
    return hash(tuple_t<NodeData*, index_t>{x.root, x.stride});
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  template<typename NodeDataFunc>
  expected_t<string_t> tree_span_t<NodeData>::to_string(NodeDataFunc node_data_func) const
  {
    string_t curr_line;
    string_t retval;
    auto result = visit_subtree(
        [&](const span_t<const tree_branch_t> path, const tree_event_t event) -> expected_t<bool> {
          if (!is_on_entry(event)) {
            return true;
          }
          SILVA_EXPECT(!path.empty(), ASSERT, "Empty path at " SILVA_CPP_LOCATION);
          curr_line.assign(2 * (path.size() - 1), ' ');
          curr_line += fmt::format("[{}]", path.back().child_index);
          node_data_func(curr_line, path);
          retval += curr_line;
          retval += '\n';
          return true;
        });
    SILVA_EXPECT_FWD(std::move(result));
    return retval;
  }

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  template<typename NodeDataFunc>
  expected_t<string_t> tree_span_t<NodeData>::to_graphviz(NodeDataFunc node_data_func) const
  {
    string_t retval;
    retval += "digraph parse_tree {\n";
    auto result = visit_subtree(
        [&](const span_t<const tree_branch_t> path, const tree_event_t event) -> expected_t<bool> {
          if (!is_on_entry(event)) {
            return true;
          }
          SILVA_EXPECT(!path.empty(), ASSERT, "Empty path at " SILVA_CPP_LOCATION);
          const auto& node = (*this)[path.back().node_index];
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
