#include "error.hpp"
#include "tree.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using enum error_level_t;
  using enum tree_event_t;

  struct result_t {
    size_t stack_size  = 0;
    index_t node_index = 0;
    tree_event_t event = INVALID;

    friend auto operator<=>(const result_t&, const result_t&) = default;
    friend void pretty_write_impl(const result_t& self, byte_sink_t* byte_sink)
    {
      byte_sink->format("{}\n", rfl::json::write(self));
    }
  };

  TEST_CASE("error-tree", "[error_tree_t]")
  {
    error_tree_t tree;
    using node_t = error_tree_t::node_t;
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [0]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [1]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [2]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [3]
    tree.nodes.push_back(node_t{{.num_children = 2, .subtree_size = 5}});  // [4]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 6}});  // [5]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 7}});  // [6]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [7]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [8]
    tree.nodes.push_back(node_t{{.num_children = 2, .subtree_size = 3}});  // [9]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [10]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [11]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [12]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [13]
    tree.nodes.push_back(node_t{{.num_children = 4, .subtree_size = 15}}); // [14]

    array_t<result_t> result;
    const index_t root_index = tree.nodes.size() - 1;
    const tree_span_t<const node_t> tspan{&tree.nodes[root_index], -1};
    SILVA_REQUIRE(tspan.visit_subtree(
        [&](const span_t<const tree_branch_t> path, const tree_event_t event) -> expected_t<bool> {
          result.push_back(result_t{
              .stack_size = path.size(),
              .node_index = root_index - path.back().node_index,
              .event      = event,
          });
          return true;
        }));
    CHECK(result ==
          array_t<result_t>{{
              result_t{.stack_size = 1, .node_index = 14, .event = ON_ENTRY},

              result_t{.stack_size = 2, .node_index = 13, .event = ON_ENTRY},
              result_t{.stack_size = 3, .node_index = 12, .event = ON_LEAF},
              result_t{.stack_size = 2, .node_index = 13, .event = ON_EXIT},

              result_t{.stack_size = 2, .node_index = 11, .event = ON_ENTRY},
              result_t{.stack_size = 3, .node_index = 10, .event = ON_LEAF},
              result_t{.stack_size = 2, .node_index = 11, .event = ON_EXIT},

              result_t{.stack_size = 2, .node_index = 9, .event = ON_ENTRY},
              result_t{.stack_size = 3, .node_index = 8, .event = ON_LEAF},
              result_t{.stack_size = 3, .node_index = 7, .event = ON_LEAF},
              result_t{.stack_size = 2, .node_index = 9, .event = ON_EXIT},

              result_t{.stack_size = 2, .node_index = 6, .event = ON_ENTRY},
              result_t{.stack_size = 3, .node_index = 5, .event = ON_ENTRY},
              result_t{.stack_size = 4, .node_index = 4, .event = ON_ENTRY},

              result_t{.stack_size = 5, .node_index = 3, .event = ON_ENTRY},
              result_t{.stack_size = 6, .node_index = 2, .event = ON_LEAF},
              result_t{.stack_size = 5, .node_index = 3, .event = ON_EXIT},

              result_t{.stack_size = 5, .node_index = 1, .event = ON_ENTRY},
              result_t{.stack_size = 6, .node_index = 0, .event = ON_LEAF},
              result_t{.stack_size = 5, .node_index = 1, .event = ON_EXIT},

              result_t{.stack_size = 4, .node_index = 4, .event = ON_EXIT},
              result_t{.stack_size = 3, .node_index = 5, .event = ON_EXIT},
              result_t{.stack_size = 2, .node_index = 6, .event = ON_EXIT},

              result_t{.stack_size = 1, .node_index = 14, .event = ON_EXIT},
          }});
  }

  TEST_CASE("error", "[error_t]")
  {
    error_context_t error_context;

    array_t<silva::error_t> errors;

    silva::error_t final_error;
    {
      auto a_1 = make_error(MINOR, {}, "scope a 1");
      auto a_2 = make_error(MINOR, {}, "scope a 2");

      silva::error_t a_3;
      {
        auto b_1 = make_error(MINOR, {}, "scope b 1 i");

        errors.clear();
        errors.push_back(std::move(b_1));
        b_1 = make_error(MINOR, errors, "scope b 1 ii");

        errors.clear();
        errors.push_back(std::move(b_1));
        b_1 = make_error(MINOR, errors, "scope b 1 iii");

        auto b_2 = make_error(MINOR, {}, "scope b 2");
        errors.clear();
        errors.push_back(std::move(b_1));
        errors.push_back(std::move(b_2));
        a_3 = make_error(MAJOR, errors, "combined 1");

        errors.clear();
        errors.push_back(std::move(a_3));
        a_3 = make_error(MINOR, errors, "combined 2");
      }

      auto a_4 = make_error(MINOR, {}, "scope a 4");

      errors.clear();
      errors.push_back(std::move(a_1));
      errors.push_back(std::move(a_2));
      errors.push_back(std::move(a_3));
      errors.push_back(std::move(a_4));
      final_error = make_error(MINOR, errors, "scope final 1");

      errors.clear();
      errors.push_back(std::move(final_error));
      final_error = make_error(MINOR, errors, "scope final 2");

      errors.clear();
      errors.push_back(std::move(final_error));
      final_error = make_error(MINOR, errors, "scope final 3");
    }

    CHECK(error_context.tree.nodes.size() == 12);
    {
      const string_view_t expected = R"(
┌─scope a 1
├─scope a 2
│   scope b 1 i
│   scope b 1 ii
│ ┌─scope b 1 iii
│ ├─scope b 2
│ combined 1
├─combined 2
├─scope a 4
scope final 1
scope final 2
scope final 3
)";
      const auto result            = final_error.to_string_flat();
      CHECK(result.as_string_view() == expected.substr(1));
    }
  }
}
