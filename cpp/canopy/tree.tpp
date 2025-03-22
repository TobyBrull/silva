#include "tree.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

struct test_tree_node_t : public tree_node_t {
  string_t name;
};

static_assert(std::input_or_output_iterator<tree_span_child_iter_t<test_tree_node_t>>);

TEST_CASE("tree")
{
  vector_t<test_tree_node_t> test_tree;
  test_tree.push_back({{3, 8}, "A"});
  test_tree.push_back({{2, 4}, "B"});
  test_tree.push_back({{0, 1}, "E"});
  test_tree.push_back({{1, 2}, "F"});
  test_tree.push_back({{0, 1}, "G"});
  test_tree.push_back({{0, 1}, "C"});
  test_tree.push_back({{1, 2}, "D"});
  test_tree.push_back({{0, 1}, "H"});

  {
    const tree_span_t<test_tree_node_t> tree_span{test_tree};

    {
      vector_t<string_t> results;
      for (const auto& [node_idx, child_idx]: tree_span.children_range()) {
        results.push_back(tree_span[node_idx].name);
      }
      CHECK(results == vector_t<string_t>{"B", "C", "D"});
    }
  }

  std::ranges::reverse(test_tree);
  {
    tree_span_t<test_tree_node_t> tree_span_2{&test_tree.back(), -1};

    {
      vector_t<string_t> results;
      for (const auto& [node_idx, child_idx]: tree_span_2.children_range()) {
        results.push_back(tree_span_2[node_idx].name);
      }
      CHECK(results == vector_t<string_t>{"B", "C", "D"});
    }
  }
}
