#include "bytecode.hpp"
#include "bytecode_compiler.hpp"
#include "bytecode_vm.hpp"

#include "lox.hpp"

#include <catch2/catch_all.hpp>

namespace silva::lox::test {
  TEST_CASE("lox::bytecode", "[lox][bytecode]")
  {
    const string_view_t lox_expr = R"'(
      1 + 2 * 3 + 4
    )'";

    syntax_ward_t sw;
    auto si = seed_interpreter(sw.ptr());
  }
}
