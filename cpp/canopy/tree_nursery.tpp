#include "tree_nursery.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

struct test_tree_node_t : public tree_node_t {
  string_t name;
};

struct test_tree_nursery_t
  : public tree_nursery_t<test_tree_node_t, tree_nursery_state_t, test_tree_nursery_t> {};

TEST_CASE("tree_nursery")
{
  test_tree_nursery_t nursery;
  auto A{nursery.stake()};
  A.create_node();
  A.proto_node.name = "A";
  {
    auto B{nursery.stake()};
    B.create_node();
    B.proto_node.name = "B";
    {
      auto E{nursery.stake()};
      E.create_node();
      E.proto_node.name = "E";
      B.add_proto_node(E.commit());
    }
    {
      auto F{nursery.stake()};
      F.create_node();
      F.proto_node.name = "F";
      {
        auto G{nursery.stake()};
        G.create_node();
        G.proto_node.name = "G";
        F.add_proto_node(G.commit());
      }
      B.add_proto_node(F.commit());
    }
    {
      auto Z{nursery.stake()};
      Z.create_node();
      Z.proto_node.name = "Z";
    }
    A.add_proto_node(B.commit());
  }
  {
    auto C{nursery.stake()};
    C.create_node();
    C.proto_node.name = "C";
    A.add_proto_node(C.commit());
  }
  {
    auto D{nursery.stake()};
    D.create_node();
    D.proto_node.name = "D";
    {
      auto H{nursery.stake()};
      H.create_node();
      H.proto_node.name = "H";
      D.add_proto_node(H.commit());
    }
    A.add_proto_node(D.commit());
  }
  const test_tree_node_t root_node = A.commit();
  CHECK(root_node.num_children == 1);
  CHECK(root_node.subtree_size == 8);
  auto result = std::move(nursery).finish();
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
