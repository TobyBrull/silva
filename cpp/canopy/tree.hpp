#pragma once

#include "array_small.hpp"
#include "canopy/iterator_facade.hpp"
#include "expected.hpp"
#include "preprocessor.hpp"
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
  struct tree_span_t {
    static_assert(std::derived_from<NodeData, tree_node_t>);

    NodeData* root = nullptr;
    index_t stride = 0;

    tree_span_t() = default;
    tree_span_t(NodeData* root, index_t stride);
    explicit tree_span_t(span_t<NodeData>);
    explicit tree_span_t(array_t<NodeData>&);

    index_t subtree_size() const;
    index_t num_children() const;

    auto& node_at(this auto&&, index_t);
    tree_span_t subspan_at(index_t) const;

    auto children_range(this auto&&);
    auto children_range_idx(this auto&&);

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

    template<typename Self>
    expected_t<Self> iterate_to_child(this const Self&, index_t);

    template<typename Self>
    array_t<Self> get_children_array(this const Self&);

    // Get the direct child tree-spans if the number of children is exactly equal to "N".
    template<index_t N, typename Self>
    expected_t<array_fixed_t<Self, N>> get_children(this const Self&);

    // Get the direct child tree-spans if the number of children is less-or-equal to "N".
    template<index_t N, typename Self>
    expected_t<array_small_t<Self, N>> get_children_up_to(this const Self&);

    friend bool operator==(const tree_span_t&, const tree_span_t&) = default;
    // friend hash_value_t hash_impl(const tree_span_t& x);

    template<typename NodeDataFunc>
    expected_t<string_t> to_string(NodeDataFunc) const;

    template<typename NodeDataFunc>
    expected_t<string_t> to_graphviz(NodeDataFunc) const;
  };

  template<typename Span, bool WithIdx>
  struct tree_span_child_pts_iter_t : public iterator_facade_t {
    Span tree_span;
    index_t pos         = 1;
    index_t child_index = 0;

    auto dereference() const;
    void increment();
    friend auto operator<=>(const tree_span_child_pts_iter_t&,
                            const tree_span_child_pts_iter_t&) = default;
  };

  template<typename NodeData>
    requires std::derived_from<NodeData, tree_node_t>
  using tree_t = array_t<NodeData>;
}

// IMPLEMENTATION

namespace silva {

  template<typename Span, bool WithIdx>
  auto tree_span_child_pts_iter_t<Span, WithIdx>::dereference() const
  {
    if constexpr (WithIdx) {
      return pair_t<Span, index_t>{tree_span.subspan_at(pos), child_index};
    }
    else {
      return tree_span.subspan_at(pos);
    }
  }

  template<typename Span, bool WithIdx>
  void tree_span_child_pts_iter_t<Span, WithIdx>::increment()
  {
    pos += tree_span.node_at(pos).subtree_size;
    child_index += 1;
  }

  template<typename NodeData>
  tree_span_t<NodeData>::tree_span_t(NodeData* root, index_t stride) : root{root}, stride{stride}
  {
  }

  template<typename NodeData>
  tree_span_t<NodeData>::tree_span_t(span_t<NodeData> vec) : root{vec.data()}, stride{1}
  {
  }

  template<typename NodeData>
  tree_span_t<NodeData>::tree_span_t(array_t<NodeData>& vec) : root{vec.data()}, stride{1}
  {
  }

  template<typename NodeData>
  index_t tree_span_t<NodeData>::subtree_size() const
  {
    return root->subtree_size;
  }
  template<typename NodeData>
  index_t tree_span_t<NodeData>::num_children() const
  {
    return root->num_children;
  }

  template<typename NodeData>
  auto& tree_span_t<NodeData>::node_at(this auto&& self, const index_t i)
  {
    return *(self.root + self.stride * i);
  }

  template<typename NodeData>
  tree_span_t<NodeData> tree_span_t<NodeData>::subspan_at(const index_t pos) const
  {
    return {&((*this).node_at(pos)), stride};
  }

  namespace impl {
    template<bool WithIdx, typename Span>
    auto children_range_pts(const Span& self)
    {
      using iter_t = tree_span_child_pts_iter_t<Span, WithIdx>;
      static_assert(std::input_or_output_iterator<iter_t>);
      iter_t begin{
          .tree_span   = self,
          .pos         = 1,
          .child_index = 0,
      };
      iter_t end{
          .tree_span   = self,
          .pos         = self.root->subtree_size,
          .child_index = self.root->num_children,
      };
      return std::ranges::subrange<iter_t, iter_t>(begin, end);
    }
  }

  template<typename NodeData>
  auto tree_span_t<NodeData>::children_range(this auto&& self)
  {
    return impl::children_range_pts<false, std::remove_cvref_t<decltype(self)>>(self);
  }

  template<typename NodeData>
  auto tree_span_t<NodeData>::children_range_idx(this auto&& self)
  {
    return impl::children_range_pts<true, std::remove_cvref_t<decltype(self)>>(self);
  }

  template<typename NodeData>
  template<typename Visitor>
    requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
  expected_t<void> tree_span_t<NodeData>::visit_subtree(Visitor visitor) const
  {
    array_t<tree_branch_t> path;
    const auto clean_stack_till =
        [&](const index_t new_node_index) -> expected_t<optional_t<index_t>> {
      index_t next_child_index = 0;
      while (!path.empty() &&
             path.back().node_index + (*this).node_at(path.back().node_index).subtree_size <=
                 new_node_index) {
        const bool is_leaf = ((*this).node_at(path.back().node_index).num_children == 0);
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

    const index_t end_node_index = (*this).subtree_size();
    for (index_t node_index = 0; node_index < end_node_index; ++node_index) {
      const optional_t<index_t> maybe_new_child_index =
          SILVA_EXPECT_FWD(clean_stack_till(node_index));
      if (!maybe_new_child_index) {
        return {};
      }
      path.push_back({.node_index = node_index, .child_index = maybe_new_child_index.value()});
      const bool is_leaf = ((*this).node_at(node_index).num_children == 0);
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
  template<typename Self>
  expected_t<Self> tree_span_t<NodeData>::iterate_to_child(this const Self& self, index_t idx)
  {
    if (0 <= idx) {
      SILVA_EXPECT(idx < self.num_children(), MINOR);
      index_t pos = 1;
      while (idx > 0) {
        pos += self.node_at(pos).subtree_size;
        idx -= 1;
      }
      return self.subspan_at(pos);
    }
    else {
      SILVA_EXPECT(-self.num_children() <= idx && idx <= -1, MINOR);
      return self.iterate_to_child(self.num_children() + idx);
    }
  }

  template<typename NodeData>
  template<typename Self>
  array_t<Self> tree_span_t<NodeData>::get_children_array(this const Self& self)
  {
    const auto& node = self.node_at(0);
    array_t<Self> retval;
    retval.reserve(node.num_children);
    for (const auto pts_child: self.children_range()) {
      retval.emplace_back(pts_child);
    }
    return retval;
  }

  template<typename NodeData>
  template<index_t N, typename Self>
  expected_t<array_fixed_t<Self, N>> tree_span_t<NodeData>::get_children(this const Self& self)
  {
    const auto& node = self.node_at(0);
    SILVA_EXPECT(node.num_children == N,
                 MAJOR,
                 "expected {} children, got {}",
                 N,
                 node.num_children);
    static_assert(std::derived_from<Self, tree_span_t<NodeData>>);
    array_fixed_t<Self, N> retval;
    for (const auto [pts_child, child_index]: self.children_range_idx()) {
      retval[child_index] = pts_child;
    }
    return retval;
  }

  template<typename NodeData>
  template<index_t N, typename Self>
  expected_t<array_small_t<Self, N>>
  tree_span_t<NodeData>::get_children_up_to(this const Self& self)
  {
    const auto& node = self.node_at(0);
    SILVA_EXPECT(node.num_children <= N, MAJOR);
    static_assert(std::derived_from<Self, tree_span_t<NodeData>>);
    array_small_t<Self, N> retval;
    for (const auto pts_child: self.children_range()) {
      retval.emplace_back(pts_child);
    }
    return retval;
  }

  template<typename NodeData>
  hash_value_t hash_impl(const tree_span_t<NodeData>& x)
  {
    return hash(tuple_t<NodeData*, index_t>{x.root, x.stride});
  }

  template<typename NodeData>
  template<typename NodeDataFunc>
  expected_t<string_t> tree_span_t<NodeData>::to_string(NodeDataFunc node_data_func) const
  {
    string_t retval;
    auto result = visit_subtree(
        [&](const span_t<const tree_branch_t> path, const tree_event_t event) -> expected_t<bool> {
          if (!is_on_entry(event)) {
            return true;
          }
          SILVA_EXPECT(!path.empty(), ASSERT, "Empty path at " SILVA_CPP_LOCATION);
          string_t curr_line;
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
          const auto& node = (*this).node_at(path.back().node_index);
          string_t node_name{"/"};
          string_t curr_line;
          if (path.size() >= 2) {
            string_t parent_node_name = "/";
            for (index_t i = 1; i < path.size() - 1; ++i) {
              parent_node_name += fmt::format("{}/", path[i].child_index);
            }
            node_name = fmt::format("{}{}/", parent_node_name, path.back().child_index);
            curr_line += fmt::format("  \"{}\" -> \"{}\"\n", parent_node_name, node_name);
          }
          curr_line += fmt::format("  \"{}\" [label=\"[{}]", node_name, path.back().child_index);
          node_data_func(curr_line, path);
          curr_line += "\"]\n";
          retval += curr_line;
          return true;
        });
    SILVA_EXPECT_FWD(std::move(result));
    retval += "}";
    return retval;
  }
}
