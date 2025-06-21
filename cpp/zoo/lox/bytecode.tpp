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

    const auto& eval = [&](const string_view_t lox_code) -> object_ref_t {
      const auto tp = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.lox", lox_code));
      const auto pt = SILVA_EXPECT_REQUIRE(si->apply(tp, sw.name_id_of("Lox", "Expr")));
      INFO(SILVA_EXPECT_REQUIRE(pt->span().to_string()));
      const chunk_t chunk = SILVA_EXPECT_REQUIRE(compiler.compile(pt->span(), pool));
      INFO(SILVA_EXPECT_REQUIRE(chunk.to_string()));
      SILVA_EXPECT_REQUIRE(vm.run(chunk));
      REQUIRE(vm.stack.size() == 1);
      return vm.stack.back();
    };

    CHECK(eval(" 42.0 ")->as_double() == 42.0);
    // CHECK(eval(" 1 + 2 * 3 + 4 ")->as_double() == 11.0);
  }
}
