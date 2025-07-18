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
    compiler_t compiler(sw.ptr(), pool);
    byte_sink_memory_t print_buffer;
    vm_t vm{.print_stream = &print_buffer};

    const auto make_chunk =
        [&](const string_view_t lox_code) -> tuple_t<parse_tree_ptr_t, unique_ptr_t<chunk_t>> {
      const auto tp  = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.lox", lox_code));
      const auto ptp = SILVA_EXPECT_REQUIRE(si->apply(tp, sw.name_id_of("Lox")));
      auto chunk     = std::make_unique<chunk_t>(sw.ptr());
      SILVA_EXPECT_REQUIRE(compiler.compile(ptp->span(), *chunk));
      return {ptp, std::move(chunk)};
    };

    {
      const auto [ptp, chunk]      = make_chunk("var hello = 'world' ; 1 + 2 * 3 ;");
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

    const auto test = [&](const string_view_t lox_code, const string_view_t expected) {
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

    test(" print 42.0 ; print 12.0 ; ", "42\n12\n");
    test(" print - 42.0 ; ", "-42\n");
    test(" print 1 + 2 * 3 + 4 ; ", "11\n");
    test(" print ! true ; ", "false\n");
    test(" print ! ( 1 + 2 == 3 ) ; ", "false\n");
    test(" print 1 + 2 != 4 ; ", "true\n");
    test(" print 1 + 2 <= 2 ; ", "false\n");
    test(" print 1 + 2 <= 3 ; ", "true\n");
    test(" print 1 + 2 <= 4 ; ", "true\n");
    test(" print 'hello' + ' world' ; ", "hello world\n");
    test(" print ! ( 5 - 4 > 3 * 2 == ! none ) ; ", "true\n");
    test(" var abc = 42 ; print 100 + abc ;", "142\n");
    test(" var a = 3 ; var b = 5 ; a = 10 + b * a ; print a ;", "25\n");
    test(" var a = 10 ; { print a ; var a = 20 ; var b = 30 ; print a ; print b ; } print a ; ",
         "10\n20\n30\n10\n");
    test(" var a; if ( 1 + 2 < 4 ) { a = 'true' ; } else { a = 'false' ; } print a ; ", "true\n");
    test(" var a; if ( 1 + 2 > 4 ) { a = 'true' ; } else { a = 'false' ; } print a ; ", "false\n");
    test(" var a; if ( 1 + 2 < 4 ) { a = 'true' ; } print a ; ", "true\n");
    test(" var a; if ( 1 + 2 > 4 ) { a = 'true' ; } print a ; ", "none\n");
    test(" var a = ( 1 + 2 > 4 ) or 'test' ; print a ; ", "test\n");
    test(" var a = ( 1 + 2 < 4 ) or 'test' ; print a ; ", "true\n");
    test(" var a = ( 1 + 2 > 4 ) and 'test' ; print a ; ", "false\n");
    test(" var a = ( 1 + 2 < 4 ) and 'test' ; print a ; ", "test\n");
    test(R"(
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
    // TODO_CURR
    //
    // test(R"(
    //     fun foo(y) {
    //       return y * 10;
    //     }
    //     fun bar(x) {
    //       x = x + 10;
    //       x = x + foo(x);
    //       return x;
    //     }
    //     print bar(10);
    //   )",
    //      "220\n");
    // test(R"(
    //     fun fib(n) {
    //       if (n < 2) return n;
    //       return fib(n - 1) + fib(n - 2);
    //     }
    //     for ( var i = 1; i <= 4 ; i = i + 1 ) {
    //       print fib(i);
    //     }
    //   )",
    //      "1\n1\n");

    const auto test_runtime_error = [&](const string_view_t lox_code,
                                        const vector_t<string_t> expected_err_msgs) {
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
    test_runtime_error(" return 1 + ( 42 + 'world' ) ; ",
                       {
                           "runtime type error",
                           "while executing instruction",
                           "42 + 'world'",
                           "[test.lox:1:15]",
                       });
  }
}
