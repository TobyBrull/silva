#include "tree_reversed.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

using event_t = tree_event_t;
struct result_t {
  size_t stack_size  = 0;
  index_t node_index = 0;
  event_t event      = event_t::INVALID;

  friend auto operator<=>(const result_t&, const result_t&) = default;
  friend std::ostream& operator<<(std::ostream& os, const result_t& self)
  {
    return os << rfl::json::write(self) << '\n';
  }
};

TEST_CASE("tree", "[tree_t]")
{
  tree_t<std::monostate> tree;
  using node_t = tree_t<std::monostate>::node_t;
  tree.nodes.push_back(node_t{.num_children = 4, .children_end = 15}); // [0]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 3});  // [1]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 3});  // [2]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 5});  // [3]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 5});  // [4]
  tree.nodes.push_back(node_t{.num_children = 2, .children_end = 8});  // [5]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 7});  // [6]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 8});  // [7]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 15}); // [8]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 15}); // [9]
  tree.nodes.push_back(node_t{.num_children = 2, .children_end = 15}); // [10]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 13}); // [11]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 13}); // [12]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 15}); // [13]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 15}); // [14]

  vector_t<result_t> result;
  REQUIRE(tree.visit_subtree(
      [&](const span_t<const tree_branch_t> path, const event_t event) -> expected_t<bool> {
        result.push_back(result_t{
            .stack_size = path.size(),
            .node_index = path.back().node_index,
            .event      = event,
        });
        return true;
      }));
  CHECK(result ==
        vector_t<result_t>{{
            result_t{.stack_size = 1, .node_index = 0, .event = event_t::ON_ENTRY},

            result_t{.stack_size = 2, .node_index = 1, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 2, .event = event_t::ON_LEAF},
            result_t{.stack_size = 2, .node_index = 1, .event = event_t::ON_EXIT},

            result_t{.stack_size = 2, .node_index = 3, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 4, .event = event_t::ON_LEAF},
            result_t{.stack_size = 2, .node_index = 3, .event = event_t::ON_EXIT},

            result_t{.stack_size = 2, .node_index = 5, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 6, .event = event_t::ON_LEAF},
            result_t{.stack_size = 3, .node_index = 7, .event = event_t::ON_LEAF},
            result_t{.stack_size = 2, .node_index = 5, .event = event_t::ON_EXIT},

            result_t{.stack_size = 2, .node_index = 8, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 9, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 4, .node_index = 10, .event = event_t::ON_ENTRY},

            result_t{.stack_size = 5, .node_index = 11, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 6, .node_index = 12, .event = event_t::ON_LEAF},
            result_t{.stack_size = 5, .node_index = 11, .event = event_t::ON_EXIT},

            result_t{.stack_size = 5, .node_index = 13, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 6, .node_index = 14, .event = event_t::ON_LEAF},
            result_t{.stack_size = 5, .node_index = 13, .event = event_t::ON_EXIT},

            result_t{.stack_size = 4, .node_index = 10, .event = event_t::ON_EXIT},
            result_t{.stack_size = 3, .node_index = 9, .event = event_t::ON_EXIT},
            result_t{.stack_size = 2, .node_index = 8, .event = event_t::ON_EXIT},

            result_t{.stack_size = 1, .node_index = 0, .event = event_t::ON_EXIT},
        }});
}

TEST_CASE("tree-reversed", "[tree_reversed_t]")
{
  tree_reversed_t<std::monostate> tree;
  using node_t = tree_reversed_t<std::monostate>::node_t;
  tree.nodes.push_back(node_t{.num_children = 0, .children_begin = 0});  // [0]
  tree.nodes.push_back(node_t{.num_children = 1, .children_begin = 0});  // [1]
  tree.nodes.push_back(node_t{.num_children = 0, .children_begin = 2});  // [2]
  tree.nodes.push_back(node_t{.num_children = 1, .children_begin = 2});  // [3]
  tree.nodes.push_back(node_t{.num_children = 2, .children_begin = 0});  // [4]
  tree.nodes.push_back(node_t{.num_children = 1, .children_begin = 0});  // [5]
  tree.nodes.push_back(node_t{.num_children = 1, .children_begin = 0});  // [6]
  tree.nodes.push_back(node_t{.num_children = 0, .children_begin = 7});  // [7]
  tree.nodes.push_back(node_t{.num_children = 0, .children_begin = 8});  // [8]
  tree.nodes.push_back(node_t{.num_children = 2, .children_begin = 7});  // [9]
  tree.nodes.push_back(node_t{.num_children = 0, .children_begin = 10}); // [10]
  tree.nodes.push_back(node_t{.num_children = 1, .children_begin = 10}); // [11]
  tree.nodes.push_back(node_t{.num_children = 0, .children_begin = 12}); // [12]
  tree.nodes.push_back(node_t{.num_children = 1, .children_begin = 12}); // [13]
  tree.nodes.push_back(node_t{.num_children = 4, .children_begin = 0});  // [14]

  vector_t<result_t> result;
  SILVA_EXPECT_REQUIRE(tree.visit_subtree(
      [&](const span_t<const tree_branch_t> path, const event_t event) -> expected_t<bool> {
        result.push_back(result_t{
            .stack_size = path.size(),
            .node_index = path.back().node_index,
            .event      = event,
        });
        return true;
      },
      tree.nodes.size() - 1));
  CHECK(result ==
        vector_t<result_t>{{
            result_t{.stack_size = 1, .node_index = 14, .event = event_t::ON_ENTRY},

            result_t{.stack_size = 2, .node_index = 13, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 12, .event = event_t::ON_LEAF},
            result_t{.stack_size = 2, .node_index = 13, .event = event_t::ON_EXIT},

            result_t{.stack_size = 2, .node_index = 11, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 10, .event = event_t::ON_LEAF},
            result_t{.stack_size = 2, .node_index = 11, .event = event_t::ON_EXIT},

            result_t{.stack_size = 2, .node_index = 9, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 8, .event = event_t::ON_LEAF},
            result_t{.stack_size = 3, .node_index = 7, .event = event_t::ON_LEAF},
            result_t{.stack_size = 2, .node_index = 9, .event = event_t::ON_EXIT},

            result_t{.stack_size = 2, .node_index = 6, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 5, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 4, .node_index = 4, .event = event_t::ON_ENTRY},

            result_t{.stack_size = 5, .node_index = 3, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 6, .node_index = 2, .event = event_t::ON_LEAF},
            result_t{.stack_size = 5, .node_index = 3, .event = event_t::ON_EXIT},

            result_t{.stack_size = 5, .node_index = 1, .event = event_t::ON_ENTRY},
            result_t{.stack_size = 6, .node_index = 0, .event = event_t::ON_LEAF},
            result_t{.stack_size = 5, .node_index = 1, .event = event_t::ON_EXIT},

            result_t{.stack_size = 4, .node_index = 4, .event = event_t::ON_EXIT},
            result_t{.stack_size = 3, .node_index = 5, .event = event_t::ON_EXIT},
            result_t{.stack_size = 2, .node_index = 6, .event = event_t::ON_EXIT},

            result_t{.stack_size = 1, .node_index = 14, .event = event_t::ON_EXIT},
        }});
}
