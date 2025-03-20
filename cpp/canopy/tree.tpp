#include "tree.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

struct test_tree_node_t : public tree_node_t {
  string_t name;
};

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

  using ts_t = tree_span_t<test_tree_node_t>;
  ts_t ts{test_tree};

  {
    vector_t<string_t> results;
    for (const auto& child: ts.children_range()) {
      results.push_back(child.front().name);
    }
    CHECK(results == vector_t<string_t>{"B", "C", "D"});
  }
}
