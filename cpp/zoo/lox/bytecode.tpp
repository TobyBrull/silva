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
      INFO("result");
      INFO(result);
      INFO("expected");
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
  }

  TEST_CASE("lox-bytecode-closure", "[lox][bytecode]")
  {
    test_harness_t th;
    th.test_success(R"(
        fun aaa() {
          var temp;
          {
            var x = 11;
            var y = 22;
            fun bbb() {
              print y;
              print x;
            }
            temp = bbb;
          }
          return temp;
        }
        var rv = aaa();
        rv();
      )",
                    "22\n11\n");
    th.test_success(R"(
        fun aaa() {
          var y = 123;
          fun bbb() {
            var temp;
            {
              var x = 42;
              fun ccc() {
                print x;
                print y;
              }
              temp = ccc;
            }
            temp();
          }
          return bbb;
        }
        var rv = aaa();
        rv();
      )",
                    "42\n123\n");
    th.test_success(R"(
        fun aaa() {
          var xxx = 123;
          fun bbb() {
            fun ccc() {
              print xxx;
            }
            var xxx = 42;
            fun ddd() {
              print xxx;
            }
            ccc();
            ddd();
            xxx = 43;
            ccc();
            ddd();
            return ccc;
          }
          var inner = bbb();
          inner();
          return inner;
        }
        var rv = aaa();
        rv();
      )",
                    "123\n42\n123\n43\n123\n123\n");
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
                    "outer\n");
    th.test_success(R"(
        fun outer() {
          var x = 'value';
          fun middle() {
            fun inner() {
              print x;
            }
            print 'create inner closure';
            return inner;
          }
          print 'return from outer';
          return middle;
        }
        var mid = outer();
        var in = mid();
        in();
    )",
                    "return from outer\ncreate inner closure\nvalue\n");
    th.test_success(R"(
        var globalSet;
        var globalGet;
        fun main() {
          var a = 'initial';
          fun set() { a = 'updated'; }
          fun get() { print a; }
          globalSet = set;
          globalGet = get;
        }
        main();
        globalGet();
        globalSet();
        globalGet();
    )",
                    "initial\nupdated\n");
    th.test_success(R"(
        fun make_counter() {
          var x = 0;
          fun retval() {
            x = x + 1;
            print x;
          }
          return retval;
        }
        var c1 = make_counter();
        var c2 = make_counter();
        c1();
        c1();
        c1();
        c2();
        c2();
        c1();
    )",
                    "1\n2\n3\n1\n2\n4\n");
  }

  TEST_CASE("lox-bytecode-classes", "[lox][bytecode]")
  {
    test_harness_t th;
    th.test_success(R"(
        {
          class Toast {}
          var toast = Toast();
          print toast.jam = 'grape';
        }

        class Pair {}
        var pair = Pair();
        pair.first = 1;
        pair.second = 2;
        print pair.first + pair.second;
    )",
                    "grape\n3\n");
    th.test_success(R"(
        class Foo {
          bar(first, second) {
            print 'bar with ' + first + ' and ' + second;
          }
          baz(x) {
            print 'baz with ' + x;
          }
        }
        var foo = Foo();
        foo.bar('aa', 'bb');
        var bound_method = foo.baz;
        bound_method('cc');
    )",
                    "bar with aa and bb\nbaz with cc\n");
    th.test_success(R"(
        class Person {
          sayName() {
            print this.name;
          }
        }
        var jane = Person();
        jane.name = 'Jane';
        var method = jane.sayName;
        method();
    )",
                    "Jane\n");
    th.test_success(R"(
        class Person {
          init(name_param) {
            this.name = name_param;
          }
          sayName() {
            print 'I am ' + this.name;
          }
        }
        Person('Jane').sayName();
    )",
                    "I am Jane\n");
    th.test_success(R"(
        class A {
          foo() { print 'A foo'; }
          bar() { print 'A bar'; }
        }
        class B < A {
          foo() { print 'B foo'; }
        }
        class C < B {}
        var c = C();
        c.foo();
        c.bar();
    )",
                    "B foo\nA bar\n");
    // th.test_success(R"(
    //     class A {
    //       method() { print 'A method'; }
    //     }
    //     class B < A {
    //       method() { print 'B method'; }
    //       test() { super.method(); }
    //     }
    //     class C < B {}
    //     var c = C();
    //     c.method();
    //     c.test();
    // )",
    //                 "B method\nA method\n");
    // th.test_success(R"(
    //     class D {
    //       foo() {
    //         print 'foo D';
    //       }
    //     }
    //     class E < D {
    //       method() {
    //         var closure = super.foo;
    //         closure();
    //       }
    //       foo() {
    //         print 'foo E';
    //       }
    //     }
    //     class F < E {
    //     }
    //     var f = F();
    //     f.method();
    // )",
    //                 "foo D\n");
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
