#include "unicode.hpp"

namespace silva::unicode {
  void utf8_encode_one(string_t& output, const codepoint_t cp)
  {
    if (cp < 0x80) {
      output.push_back(cp);
    }
    else if (cp < 0x800) {
      output.push_back((cp >> 6) | 0xC0);
      output.push_back(((cp >> 0) & 0x3F) | 0x80);
    }
    else if (cp < 0x10000) {
      output.push_back((cp >> 12) | 0xE0);
      output.push_back(((cp >> 6) & 0x3F) | 0x80);
      output.push_back(((cp >> 0) & 0x3F) | 0x80);
    }
    else {
      output.push_back((cp >> 18) | 0xF0);
      output.push_back(((cp >> 12) & 0x3F) | 0x80);
      output.push_back(((cp >> 6) & 0x3F) | 0x80);
      output.push_back(((cp >> 0) & 0x3F) | 0x80);
    }
  }

  expected_t<tuple_t<codepoint_t, index_t>> utf8_decode_one(const string_view_t buffer)
  {
    SILVA_EXPECT(buffer.size() >= 1, MINOR, "can't get codepoint from empty string");
    const char lb      = buffer.front();
    codepoint_t retval = 0;
    index_t len        = 0;
    if ((lb & 0x80) == 0) {
      return {{codepoint_t(lb), 1}};
    }
    else if ((lb & 0xE0) == 0xC0) {
      SILVA_EXPECT(buffer.size() >= 2, MINOR, "expected at least 2 bytes in buffer");
      // clang-format off
      retval = ((buffer[0] & 0x1F) << 6)
             | ((buffer[1] & 0x3F) << 0);
      const bool cont_bytes_good =
                (buffer[1] & 0xC0) == 0x80;
      // clang-format on
      SILVA_EXPECT(cont_bytes_good, MINOR, "invalid leading bits in continuation bytes");
      SILVA_EXPECT(retval >= 0x80, MINOR, "non-canonical encoding of two byte codepoing");
      len = 2;
    }
    else if ((lb & 0xF0) == 0xE0) {
      SILVA_EXPECT(buffer.size() >= 3, MINOR, "expected at least 3 bytes in buffer");
      // clang-format off
      retval = ((buffer[0] & 0x0F) << 12)
             | ((buffer[1] & 0x3F) << 6)
             | ((buffer[2] & 0x3F) << 0);
      const bool cont_bytes_good =
                (buffer[1] & 0xC0) == 0x80 ||
                (buffer[2] & 0xC0) == 0x80;
      // clang-format on
      SILVA_EXPECT(cont_bytes_good, MINOR, "invalid leading bits in continuation bytes");
      SILVA_EXPECT(retval >= 0x800, MINOR, "non-canonical encoding of two byte codepoing");
      len = 3;
    }
    else if ((lb & 0xF8) == 0xF0) {
      SILVA_EXPECT(buffer.size() >= 4, MINOR, "expected at least 4 bytes in buffer");
      // clang-format off
      retval = ((buffer[0] & 0x0F) << 18)
             | ((buffer[1] & 0x3F) << 12)
             | ((buffer[2] & 0x3F) << 6)
             | ((buffer[3] & 0x3F) << 0);
      const bool cont_bytes_good =
                (buffer[1] & 0xC0) == 0x80 ||
                (buffer[2] & 0xC0) == 0x80 ||
                (buffer[3] & 0xC0) == 0x80;
      // clang-format on
      SILVA_EXPECT(cont_bytes_good, MINOR, "invalid leading bits in continuation bytes");
      SILVA_EXPECT(retval >= 0x10000, MINOR, "non-canonical encoding of two byte codepoing");
      len = 4;
    }
    SILVA_EXPECT(retval <= 0x10FFFF, MINOR, "codepoint above 0x10FFFF");
    SILVA_EXPECT(((retval >> 11) != 0x1B), MINOR, "surrogate half");
    return {{retval, len}};
  }
}
