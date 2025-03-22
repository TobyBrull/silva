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

  tree_span_t<test_tree_node_t> ts;
  SECTION("normal")
  {
    ts = tree_span_t<test_tree_node_t>{test_tree};
  }
  SECTION("reverse")
  {
    std::ranges::reverse(test_tree);
    ts = tree_span_t<test_tree_node_t>{&test_tree.back(), -1};
  }

  {
    const string_t result = SILVA_EXPECT_REQUIRE(ts.to_string([&](string_t& curr_line, auto& path) {
      curr_line.push_back(' ');
      curr_line += ts[path.back().node_index].name;
    }));
    const string_view_t expected = R"(
[0] A
  [0] B
    [0] E
    [1] F
      [0] G
  [1] C
  [2] D
    [0] H
)";
    CHECK(result == expected.substr(1));
  }

  CHECK(ts.get_children_dyn() == vector_t<index_t>{1, 5, 6});
  CHECK(ts.size() == 8);
  CHECK(ts.sub_tree_span_at(1).get_children_dyn() == vector_t<index_t>{1, 2});
  CHECK(ts.sub_tree_span_at(1).size() == 4);
  CHECK(ts.sub_tree_span_at(2).get_children_dyn() == vector_t<index_t>{});
  CHECK(ts.sub_tree_span_at(2).size() == 1);
  CHECK(ts.sub_tree_span_at(3).get_children_dyn() == vector_t<index_t>{1});
  CHECK(ts.sub_tree_span_at(3).size() == 2);

  {
    vector_t<string_t> results;
    for (const auto& [node_idx, child_idx]: ts.children_range()) {
      results.push_back(ts[node_idx].name);
    }
    CHECK(results == vector_t<string_t>{"B", "C", "D"});
  }
}
