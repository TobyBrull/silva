#pragma once

#include "canopy/types.hpp"
#include "canopy/variant.hpp"

namespace silva::lox {
  struct test_error_t {
    vector_t<string_view_t> error_parts;
  };

  struct test_case_t {
    string_view_t lox_code;
    variant_t<string_view_t, test_error_t> expected;

    bool is_success_expected() const { return std::holds_alternative<string_view_t>(expected); }
  };

  struct test_chapter_t {
    string_view_t name;
    vector_t<test_case_t> test_cases;
  };

  vector_t<test_chapter_t> test_suite();
}
