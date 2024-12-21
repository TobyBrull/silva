#include "env_context.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("env_context", "[env_context_t]")
{
  {
    env_context_t outer;
    outer.variables["VAR1"_sov] = "truee"_sov;
    outer.variables["VAR2"_sov] = "def"_sov;

    CHECK(env_context_get("VAR1") == "truee");
    CHECK(env_context_get("VAR2") == "def");
    CHECK(env_context_get("VAR3").has_value() == false);
    {
      const auto result = env_context_get_as<bool>("VAR1");
      REQUIRE(!result);
      CHECK(result.error().level == error_level_t::MAJOR);
    }
    {
      const auto result = env_context_get_as<bool>("VAR100");
      REQUIRE(!result);
      CHECK(result.error().level == error_level_t::MINOR);
    }

    {
      env_context_t inner;
      inner.variables["VAR1"_sov] = "true"_sov;
      inner.variables["VAR3"_sov] = "xyz"_sov;

      CHECK(env_context_get("VAR1") == "true");
      CHECK(env_context_get("VAR2") == "def");
      CHECK(env_context_get("VAR3") == "xyz");
      {
        const auto result = env_context_get_as<bool>("VAR1");
        REQUIRE(result);
        CHECK(*result == true);
      }
    }

    CHECK(env_context_get("VAR1") == "truee");
    CHECK(env_context_get("VAR2") == "def");
    CHECK(env_context_get("VAR3").has_value() == false);
  }
}
