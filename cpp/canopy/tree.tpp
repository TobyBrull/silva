#include "tree.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  struct test_tree_node_t : public tree_node_t {
    string_t name;
  };

  static_assert(std::input_or_output_iterator<tree_span_child_iter_t<test_tree_node_t>>);

  TEST_CASE("tree")
  {
    array_t<test_tree_node_t> test_tree;
    test_tree.push_back({{3, 8}, "A"});
    test_tree.push_back({{2, 4}, "B"});
    test_tree.push_back({{0, 1}, "E"});
    test_tree.push_back({{1, 2}, "F"});
    test_tree.push_back({{0, 1}, "G"});
    test_tree.push_back({{0, 1}, "C"});
    test_tree.push_back({{1, 2}, "D"});
    test_tree.push_back({{0, 1}, "H"});

    tree_span_t tspan{test_tree};
    SECTION("normal")
    {
      tspan = tree_span_t{test_tree};
    }
    SECTION("reverse")
    {
      std::ranges::reverse(test_tree);
      tspan = tree_span_t{&test_tree.back(), -1};
    }

    {
      const string_t result_str =
          SILVA_EXPECT_REQUIRE(tspan.to_string([&](string_t& curr_line, auto& path) {
            curr_line += fmt::format(" {}", tspan[path.back().node_index].name);
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
      CHECK(result_str == expected.substr(1));
    }

    CHECK(tspan.get_children_dyn() == array_t<index_t>{1, 5, 6});
    CHECK(tspan.size() == 8);
    CHECK(tspan.sub_tree_span_at(1).get_children_dyn() == array_t<index_t>{1, 2});
    CHECK(tspan.sub_tree_span_at(1).size() == 4);
    CHECK(tspan.sub_tree_span_at(2).get_children_dyn() == array_t<index_t>{});
    CHECK(tspan.sub_tree_span_at(2).size() == 1);
    CHECK(tspan.sub_tree_span_at(3).get_children_dyn() == array_t<index_t>{1});
    CHECK(tspan.sub_tree_span_at(3).size() == 2);

    {
      array_t<string_t> results;
      for (const auto& [node_idx, child_idx]: tspan.children_range()) {
        results.push_back(tspan[node_idx].name);
      }
      CHECK(results == array_t<string_t>{"B", "C", "D"});
    }
  }
}
