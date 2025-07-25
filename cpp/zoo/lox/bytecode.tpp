#include "bytecode.hpp"
#include "bytecode_compiler.hpp"
#include "bytecode_vm.hpp"
#include "canopy/time.hpp"

#include "lox.hpp"

#include <catch2/catch_all.hpp>

namespace silva::lox::bytecode::test {
  struct test_harness_t {
    syntax_ward_t sw;
    unique_ptr_t<seed::interpreter_t> si = seed_interpreter(sw.ptr());
    object_pool_t object_pool;
    compiler_t compiler = {sw.ptr(), &object_pool};
    byte_sink_memory_t print_buffer;
    vm_t vm{sw.ptr(), &object_pool, &print_buffer};

    tuple_t<parse_tree_ptr_t, unique_ptr_t<chunk_t>> make_chunk(const string_view_t lox_code)
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
      INFO(result);
      INFO(expected);
      CHECK(vm.stack.empty());
      CHECK(result == expected);
    };

    void test_runtime_error(const string_view_t lox_code,
                            const vector_t<string_t> expected_err_msgs)
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

  TEST_CASE("lox::bytecode::to_string", "[lox][bytecode]")
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

  TEST_CASE("lox-bytecode-full", "[lox][bytecode]")
  {
    test_harness_t th;
    th.test_success(" print 42.0 ; print 12.0 ; ", "42\n12\n");
    th.test_success(" print - 42.0 ; ", "-42\n");
    th.test_success(" print 1 + 2 * 3 + 4 ; ", "11\n");
    th.test_success(" print ! true ; ", "false\n");
    th.test_success(" print ! ( 1 + 2 == 3 ) ; ", "false\n");
    th.test_success(" print 1 + 2 != 4 ; ", "true\n");
    th.test_success(" print 1 + 2 <= 2 ; ", "false\n");
    th.test_success(" print 1 + 2 <= 3 ; ", "true\n");
    th.test_success(" print 1 + 2 <= 4 ; ", "true\n");
    th.test_success(" print 'hello' + ' world' ; ", "hello world\n");
    th.test_success(" print ! ( 5 - 4 > 3 * 2 == ! none ) ; ", "true\n");
    th.test_success(" var abc = 42 ; print 100 + abc ;", "142\n");
    th.test_success(" var a = 3 ; var b = 5 ; a = 10 + b * a ; print a ;", "25\n");
    th.test_success(
        " var a = 10 ; { print a ; var a = 20 ; var b = 30 ; print a ; print b ; } print a ; ",
        "10\n20\n30\n10\n");
    th.test_success(" var a; if ( 1 + 2 < 4 ) { a = 'true' ; } else { a = 'false' ; } print a ; ",
                    "true\n");
    th.test_success(" var a; if ( 1 + 2 > 4 ) { a = 'true' ; } else { a = 'false' ; } print a ; ",
                    "false\n");
    th.test_success(" var a; if ( 1 + 2 < 4 ) { a = 'true' ; } print a ; ", "true\n");
    th.test_success(" var a; if ( 1 + 2 > 4 ) { a = 'true' ; } print a ; ", "none\n");
    th.test_success(" var a = ( 1 + 2 > 4 ) or 'test' ; print a ; ", "test\n");
    th.test_success(" var a = ( 1 + 2 < 4 ) or 'test' ; print a ; ", "true\n");
    th.test_success(" var a = ( 1 + 2 > 4 ) and 'test' ; print a ; ", "false\n");
    th.test_success(" var a = ( 1 + 2 < 4 ) and 'test' ; print a ; ", "test\n");
    th.test_success(R"(
        var sum = 0 ;
        var i = 0 ;
        while ( i <= 3 ) {
          sum = sum + i ;
          i = i + 1 ;
        }
        print sum ;
        for ( ; i < 10 ; i = i + 1 ) {
          sum = sum + i ;
        }
        print sum ;
      )",
                    "6\n45\n");
    th.test_success(R"(
        fun foo1(y) {
          fun printer(x) {
            print x;
          }
          printer(y * 10);
        }
        fun foo2(y) {
          print y * 100;
          return;
        }
        var i = 42;
        fun bar(x) {
          x = x + 10;
          foo1(x);
          foo2(x);
          return x;
        }
        print bar(i);
      )",
                    "520\n5200\n52\n");
    th.test_success(R"(
        fun fib(n) {
          if (n < 2) return n;
          return fib(n - 1) + fib(n - 2);
        }
        for ( var i = 1; i <= 6 ; i = i + 1 ) {
          print fib(i);
        }
      )",
                    "1\n1\n2\n3\n5\n8\n");
    th.test_success(R"(
        var x = 'global';
        fun outer() {
          var x = 'outer';
          fun inner() {
            print x;
          }
          inner();
        }
        outer();
      )",
                    "global\n");
  }

  TEST_CASE("lox-bytecode-error", "[lox][bytecode]")
  {
    test_harness_t th;
    th.test_runtime_error(" return 1 + ( 42 + 'world' ) ; ",
                          {
                              "type error evaluating expression",
                              "while executing instruction",
                              "42 + 'world'",
                              "[test.lox:1:15]",
                          });
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
