#include "tree.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

struct test_tree_node_t : public tree_node_t {
  string_t name;
};

static_assert(std::input_or_output_iterator<tree_span_child_iter_t<test_tree_node_t>>);

TEST_CASE("tree")
{
  vector_t<test_tree_node_t> test_tree_nodes;
  test_tree_nodes.push_back({{3, 8}, "A"});
  test_tree_nodes.push_back({{2, 4}, "B"});
  test_tree_nodes.push_back({{0, 1}, "E"});
  test_tree_nodes.push_back({{1, 2}, "F"});
  test_tree_nodes.push_back({{0, 1}, "G"});
  test_tree_nodes.push_back({{0, 1}, "C"});
  test_tree_nodes.push_back({{1, 2}, "D"});
  test_tree_nodes.push_back({{0, 1}, "H"});

  {
    tree_t<test_tree_node_t> test_tree{.nodes = test_tree_nodes};

    const tree_span_t<test_tree_node_t> tree_span = test_tree.span();

    {
      vector_t<string_t> results;
      for (const auto& child: tree_span.children_range()) {
        results.push_back(child.root->name);
      }
      CHECK(results == vector_t<string_t>{"B", "C", "D"});
    }
  }

  std::ranges::reverse(test_tree_nodes);
  {
    tree_span_t<test_tree_node_t> tree_span_2{.root = &test_tree_nodes.back(), .stride = -1};

    {
      vector_t<string_t> results;
      for (const auto& child: tree_span_2.children_range()) {
        results.push_back(child.root->name);
      }
      CHECK(results == vector_t<string_t>{"B", "C", "D"});
    }
  }
}
