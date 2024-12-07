#include "misc.hpp"

namespace silva {
  void string_append_escaped(string_t& obuf, const string_view_t unescaped_string)
  {
    for (const char c: unescaped_string) {
      switch (c) {
        case '\n':
          obuf += "\\n";
          break;
        case '\r':
          obuf += "\\r";
          break;
        case '\\':
          obuf += "\\\\";
          break;
        case '\"':
          obuf += "\\\"";
          break;
        default:
          obuf += c;
          break;
      }
    }
  }

  void string_append_unescaped(string_t& obuf, const string_view_t escaped_string)
  {
    for (size_t i = 0; i < escaped_string.size(); ++i) {
      if (escaped_string[i] == '\\' && i + 1 < escaped_string.size()) {
        switch (escaped_string[i + 1]) {
          case 'n':
            obuf += '\n';
            ++i;
            break;
          case 'r':
            obuf += '\r';
            ++i;
            break;
          case '\\':
            obuf += '\\';
            ++i;
            break;
          case '\"':
            obuf += '\"';
            ++i;
            break;
          default:
            obuf += escaped_string[i];
            break;
        }
      }
      else {
        obuf += escaped_string[i];
      }
    }
  }

  string_t string_escaped(string_view_t unescaped_string)
  {
    string_t retval;
    string_append_escaped(retval, unescaped_string);
    return retval;
  }

  string_t string_unescaped(string_view_t escaped_string)
  {
    string_t retval;
    string_append_unescaped(retval, escaped_string);
    return retval;
  }
}
