#include "bytecode.hpp"
#include "bytecode_compiler.hpp"
#include "bytecode_vm.hpp"
#include "canopy/time.hpp"

#include "lox.hpp"
#include "test_suite.hpp"

#include <catch2/catch_all.hpp>

namespace silva::lox::test {
  struct test_harness_t {
    syntax_ward_t sw;
    unique_ptr_t<seed::interpreter_t> si = seed_interpreter(sw.ptr());
    object_pool_t object_pool;
    bytecode_compiler_t compiler = {sw.ptr(), &object_pool};
    byte_sink_memory_t print_buffer;
    bytecode_vm_t vm{sw.ptr(), &object_pool, &print_buffer};

    tuple_t<parse_tree_ptr_t, unique_ptr_t<bytecode_chunk_t>>
    make_chunk(const string_view_t lox_code)
    {
      const auto tp  = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.lox", lox_code));
      const auto ptp = SILVA_EXPECT_REQUIRE(si->apply(tp, sw.name_id_of("Lox")));
      auto chunk     = SILVA_EXPECT_REQUIRE(compiler.compile(ptp->span()));
      return {ptp, std::move(chunk)};
    };

    void test_success(const string_view_t lox_code, const string_view_t expected)
    {
      const auto [ptp, chunk] = make_chunk(lox_code);
      INFO(SILVA_EXPECT_REQUIRE(ptp->span().to_string()));
      INFO(SILVA_EXPECT_REQUIRE(chunk->to_string()));
      SILVA_EXPECT_REQUIRE(vm.run(*chunk));
      const auto result = print_buffer.content_str_fetch();
      INFO("result");
      INFO(result);
      INFO("expected");
      INFO(expected);
      CHECK(vm.stack.empty());
      CHECK(result == expected);
    };

    void test_runtime_error(const string_view_t lox_code,
                            const vector_t<string_view_t>& expected_err_msgs)
    {
      const auto [ptp, chunk] = make_chunk(lox_code);
      INFO(SILVA_EXPECT_REQUIRE(ptp->span().to_string()));
      INFO(SILVA_EXPECT_REQUIRE(chunk->to_string()));
      const auto result = vm.run(*chunk);
      REQUIRE(!result.has_value());
      const string_t err_msg = pretty_string(std::move(result).error());
      INFO(err_msg);
      for (const auto expected_err_msg: expected_err_msgs) {
        INFO(expected_err_msg);
        CHECK(err_msg.contains(expected_err_msg));
      }
    };
  };

  TEST_CASE("lox::to_string", "[lox][bytecode]")
  {
    test_harness_t th;
    const auto [ptp, chunk]      = th.make_chunk("var hello = 'world' ; 1 + 2 * 3 ;");
    const string_view_t expected = R"(
   0 [1:13]              CONSTANT 0
   5 [1:1]               DEFINE_GLOBAL 209 hello
  10 [1:23]              CONSTANT 1
  15 [1:27]              CONSTANT 2
  20 [1:31]              CONSTANT 3
  25 [1:27]              MULTIPLY
  26 [1:23]              ADD
  27  --                 POP

CONSTANT-TABLE
CONSTANT 0 world
CONSTANT 1 1
CONSTANT 2 2
CONSTANT 3 3
)";
    CHECK(SILVA_EXPECT_REQUIRE(chunk->to_string()) == expected.substr(1));
  }

  TEST_CASE("lox-bytecode-vm", "[lox][bytecode]")
  {
    test_harness_t th;
    const auto ts = test_suite();
    for (const auto& chapter: ts) {
      for (const auto& test_case: chapter.test_cases) {
        if (test_case.is_success_expected()) {
          th.test_success(test_case.lox_code, std::get<string_view_t>(test_case.expected));
        }
        else {
          th.test_runtime_error(test_case.lox_code,
                                std::get<test_error_t>(test_case.expected).error_parts);
        }
      }
    }
  }

  TEST_CASE("lox-bytecode-performance", "[lox][bytecode][.]")
  {
    test_harness_t th;
    const auto start        = time_point_t::now();
    const auto [ptp, chunk] = th.make_chunk(R"(
        fun fib(n) {
          if (n < 2) return n;
          return fib(n - 1) + fib(n - 2);
        }
        print fib(30);
      )");
    SILVA_EXPECT_REQUIRE(th.vm.run(*chunk));
    const auto end = time_point_t::now();
    fmt::println("FIBS TOOK {}\n", end - start);
  }
}
