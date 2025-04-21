#include "error.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using enum error_level_t;

  TEST_CASE("error", "[error_t]")
  {
    error_context_t error_context;

    vector_t<silva::error_t> errors;

    silva::error_t final_error;
    {
      auto a_1 = make_error(MINOR, {}, "scope a 1");
      auto a_2 = make_error(MINOR, {}, "scope a 2");

      silva::error_t a_3;
      {
        auto b_1 = make_error(MINOR, {}, "scope b 1 i");

        errors.clear();
        errors.push_back(std::move(b_1));
        b_1 = make_error(MINOR, errors, "scope b 1 ii");

        errors.clear();
        errors.push_back(std::move(b_1));
        b_1 = make_error(MINOR, errors, "scope b 1 iii");

        auto b_2 = make_error(MINOR, {}, "scope b 2");
        errors.clear();
        errors.push_back(std::move(b_1));
        errors.push_back(std::move(b_2));
        a_3 = make_error(MAJOR, errors, "combined 1");

        errors.clear();
        errors.push_back(std::move(a_3));
        a_3 = make_error(MINOR, errors, "combined 2");
      }

      auto a_4 = make_error(MINOR, {}, "scope a 4");

      errors.clear();
      errors.push_back(std::move(a_1));
      errors.push_back(std::move(a_2));
      errors.push_back(std::move(a_3));
      errors.push_back(std::move(a_4));
      final_error = make_error(MINOR, errors, "scope final 1");

      errors.clear();
      errors.push_back(std::move(final_error));
      final_error = make_error(MINOR, errors, "scope final 2");

      errors.clear();
      errors.push_back(std::move(final_error));
      final_error = make_error(MINOR, errors, "scope final 3");
    }

    CHECK(error_context.tree.nodes.size() == 12);
    {
      const string_view_t expected = R"(
      scope a 1
      scope a 2
              scope b 1 i
            scope b 1 ii
          scope b 1 iii
          scope b 2
        combined 1
      combined 2
      scope a 4
    scope final 1
  scope final 2
scope final 3
)";
      const auto result            = final_error.to_string_plain();
      CHECK(result.as_string_view() == expected.substr(1));
    }
    {
      const string_view_t expected = R"(
┌─scope a 1
├─scope a 2
│   scope b 1 i
│   scope b 1 ii
│ ┌─scope b 1 iii
│ ├─scope b 2
│ combined 1
├─combined 2
├─scope a 4
scope final 1
scope final 2
scope final 3
)";
      const auto result            = final_error.to_string_structured();
      CHECK(result.as_string_view() == expected.substr(1));
    }
  }
}
