#include "var_context.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("var_context", "[var_context_t]")
  {
    {
      var_context_t outer;
      outer.variables["VAR1"_sov] = "truee"_sov;
      outer.variables["VAR2"_sov] = "def"_sov;

      CHECK(var_context_get("VAR1") == "truee");
      CHECK(var_context_get("VAR2") == "def");
      CHECK(var_context_get("VAR3").has_value() == false);
      {
        const auto result = var_context_get_as<bool>("VAR1");
        REQUIRE(!result);
        CHECK(result.error().level == error_level_t::MAJOR);
      }
      {
        const auto result = var_context_get_as<bool>("VAR100");
        REQUIRE(!result);
        CHECK(result.error().level == error_level_t::MINOR);
      }

      {
        var_context_t inner;
        inner.variables["VAR1"_sov] = "true"_sov;
        inner.variables["VAR3"_sov] = "xyz"_sov;

        CHECK(var_context_get("VAR1") == "true");
        CHECK(var_context_get("VAR2") == "def");
        CHECK(var_context_get("VAR3") == "xyz");
        {
          const auto result = var_context_get_as<bool>("VAR1");
          REQUIRE(result);
          CHECK(*result == true);
        }
      }

      CHECK(var_context_get("VAR1") == "truee");
      CHECK(var_context_get("VAR2") == "def");
      CHECK(var_context_get("VAR3").has_value() == false);
    }
  }
}
