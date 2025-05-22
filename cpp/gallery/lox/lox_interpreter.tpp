#include "lox_interpreter.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("lox", "[lox]")
  {
    constexpr string_view_t lox_example = R"(
      fun sayHi(first, last) {
        print 'Hi, ' + first + ' ' + last + '!';
      }
      sayHi('Dear', 'Reader');
      print sayHi;

      fun fib(n) {
          if (n <= 1) return n;
          return fib(n - 2) + fib(n - 1);
      }
      for (var i = 0; i < 20; i = i + 1) {
          print fib(i);
      }

      fun makeCounter() {
        var i = 0;
        fun count() {
          i = i + 1;
          print i;
        }
        return count;
      }
      var counter = makeCounter();
      counter();
      counter();
    )";
  }
}
