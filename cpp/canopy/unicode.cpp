#include "unicode.hpp"

namespace silva::unicode {
  using enum type_t;

  expected_t<type_t> codepoint_type(const codepoint_t cp)
  {
    if (cp == U' ') {
      return SPACE;
    }
    else if (cp == U'\n') {
      return NEWLINE;
    }
    else if (U'0' <= cp && cp <= U'9') {
      return DIGIT;
    }
    SILVA_EXPECT(false, MINOR, "unrecognized codepoint U+{} [{}]", static_cast<unsigned>(cp), cp);
  }
}
