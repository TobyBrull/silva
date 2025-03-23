#include "tree_nursery.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

struct test_tree_node_t : public tree_node_t {
  string_t name;
};

TEST_CASE("tree_nursery")
{
  tree_nursery_t<test_tree_node_t> nursery;
  nursery.root.node().name = "A";
  {
    auto B{nursery.create_node()};
    B.node().name = "B";
    {
      auto E{nursery.create_node()};
      E.node().name = "E";
      E.commit();
    }
    {
      auto F{nursery.create_node()};
      F.node().name = "F";
      {
        auto G{nursery.create_node()};
        G.node().name = "G";
        G.commit();
      }
      F.commit();
    }
    {
      auto Z{nursery.create_node()};
      Z.node().name = "Z";
    }
    B.commit();
  }
  {
    auto C{nursery.create_node()};
    C.node().name = "C";
    C.commit();
  }
  {
    auto D{nursery.create_node()};
    D.node().name = "D";
    {
      auto H{nursery.create_node()};
      H.node().name = "H";
      H.commit();
    }
    D.commit();
  }
  auto result = std::move(nursery).commit_root();
  {
    auto tspan = tree_span_t<test_tree_node_t>{result};
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
}
