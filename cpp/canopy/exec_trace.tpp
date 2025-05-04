#include "exec_trace.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  namespace {
    struct widget_t {
      struct item_t {
        string_t name;
        bool success = false;
      };
      exec_trace_t<item_t> et;

      void func_1()
      {
        auto ets     = SILVA_EXEC_TRACE_SCOPE(et, "func_1");
        ets->success = true;
      }

      void func_2()
      {
        auto ets = SILVA_EXEC_TRACE_SCOPE(et, "func_2");
        func_1();
        func_1();
      }
    };
  }

  TEST_CASE("exec-trace", "[exec_trace_t]")
  {
    widget_t widget;
    widget.func_2();
    widget.func_1();
    widget.func_2();
    auto etr = SILVA_EXPECT_REQUIRE(widget.et.as_tree("ROOT"));
    tree_span_t ets{etr};
    const string_t estr = SILVA_EXPECT_REQUIRE(ets.to_string([&](string_t& curr_line, auto& path) {
      const auto& dd = ets.sub_tree_span_at(path.back().node_index)[0].item.data;
      curr_line += fmt::format("{} / {}", dd.name, dd.success);
    }));
    const string_view_t expected = R"(
[0]ROOT / false
  [0]func_2 / false
    [0]func_1 / true
    [1]func_1 / true
  [1]func_1 / true
  [2]func_2 / false
    [0]func_1 / true
    [1]func_1 / true
)";
    CHECK(estr == expected.substr(1));
  }
}
