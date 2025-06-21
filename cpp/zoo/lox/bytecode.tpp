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
    vm_t vm;

    const auto& test = [&](const string_view_t lox_code, const object_ref_t expected) {
      const auto tp = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.lox", lox_code));
      const auto pt = SILVA_EXPECT_REQUIRE(si->apply(tp, sw.name_id_of("Lox", "Expr")));
      INFO(SILVA_EXPECT_REQUIRE(pt->span().to_string()));
      const chunk_t chunk = SILVA_EXPECT_REQUIRE(compiler.compile(pt->span(), pool));
      INFO(SILVA_EXPECT_REQUIRE(chunk.to_string()));
      SILVA_EXPECT_REQUIRE(vm.run(chunk));
      REQUIRE(vm.stack.size() == 1);
      const auto result = vm.stack.back();
      INFO(result);
      INFO(expected);
      vm.stack.clear();
      CHECK(*result == *expected);
    };

    test(" 42.0 ", pool.make(42.0));
    test(" - 42.0 ", pool.make(-42.0));
    test(" 1 + 2 * 3 + 4 ", pool.make(11.0));
    test(" ! true ", pool.make(false));
    test(" ! ( 1 + 2 == 3 ) ", pool.make(false));
    test(" 1 + 2 != 4 ", pool.make(true));
    test(" 'hello' + ' world' ", pool.make("hello world"));
  }
}
