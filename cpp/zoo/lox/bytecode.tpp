#include "bytecode.hpp"
#include "bytecode_compiler.hpp"
#include "bytecode_vm.hpp"

#include "lox.hpp"

#include <catch2/catch_all.hpp>

namespace silva::lox::bytecode::test {
  TEST_CASE("lox::bytecode", "[lox][bytecode]")
  {
    syntax_ward_t sw;
    auto si = seed_interpreter(sw.ptr());
    object_pool_t pool;
    compiler_t compiler(sw.ptr());
    stream_memory_t print_buffer;
    vm_t vm{.print_stream = &print_buffer};

    const auto make_chunk =
        [&](const string_view_t lox_code) -> tuple_t<parse_tree_ptr_t, chunk_t> {
      const auto tp       = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.lox", lox_code));
      const auto ptp      = SILVA_EXPECT_REQUIRE(si->apply(tp, sw.name_id_of("Lox")));
      const chunk_t chunk = SILVA_EXPECT_REQUIRE(compiler.compile(ptp->span(), pool));
      return {ptp, chunk};
    };

    {
      const auto [ptp, chunk]      = make_chunk("1 + 2 * 3 ;");
      const string_view_t expected = R"(
   0 [1:1]               CONSTANT 0 1
   2 [1:5]               CONSTANT 1 2
   4 [1:9]               CONSTANT 2 3
   6 [1:5]               MULTIPLY
   7 [1:1]               ADD
   8                     POP
)";
      CHECK(SILVA_EXPECT_REQUIRE(chunk.to_string()) == expected.substr(1));
    }

    const auto test = [&](const string_view_t lox_code, const string_view_t expected) {
      const auto [ptp, chunk] = make_chunk(lox_code);
      INFO(SILVA_EXPECT_REQUIRE(ptp->span().to_string()));
      INFO(SILVA_EXPECT_REQUIRE(chunk.to_string()));
      SILVA_EXPECT_REQUIRE(vm.run(chunk));
      const auto result = print_buffer.content_str_fetch();
      INFO(result);
      INFO(expected);
      vm.stack.clear();
      CHECK(result == expected);
    };

    test(" print 42.0 ; ", "42");
    test(" print - 42.0 ; ", "-42");
    test(" print 1 + 2 * 3 + 4 ; ", "11");
    test(" print ! true ; ", "false");
    test(" print ! ( 1 + 2 == 3 ) ; ", "false");
    test(" print 1 + 2 != 4 ; ", "true");
    test(" print 1 + 2 <= 2 ; ", "false");
    test(" print 1 + 2 <= 3 ; ", "true");
    test(" print 1 + 2 <= 4 ; ", "true");
    test(" print 'hello' + ' world' ; ", "hello world");
    test(" print ! ( 5 - 4 > 3 * 2 == ! none ) ; ", "true");

    const auto test_runtime_error = [&](const string_view_t lox_code,
                                        const vector_t<string_t> expected_err_msgs) {
      const auto [ptp, chunk] = make_chunk(lox_code);
      INFO(SILVA_EXPECT_REQUIRE(ptp->span().to_string()));
      INFO(SILVA_EXPECT_REQUIRE(chunk.to_string()));
      const auto result = vm.run(chunk);
      REQUIRE(!result.has_value());
      const string_t err_msg = to_string(std::move(result).error()).as_string();
      INFO(err_msg);
      for (const auto expected_err_msg: expected_err_msgs) {
        INFO(expected_err_msg);
        CHECK(err_msg.contains(expected_err_msg));
      }
    };
    test_runtime_error(" return 1 + ( 42 + 'world' ) ; ",
                       {
                           "runtime type error",
                           "while executing instruction",
                           "42 + 'world'",
                           "[test.lox:1:15]",
                       });
  }
}
