#include "misc.hpp"

namespace silva {
  void string_append_escaped(std::string& obuf, const std::string_view unescaped_string)
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

  void string_append_unescaped(std::string& obuf, const std::string_view escaped_string)
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

  std::string string_escaped(std::string_view unescaped_string)
  {
    std::string retval;
    string_append_escaped(retval, unescaped_string);
    return retval;
  }

  std::string string_unescaped(std::string_view escaped_string)
  {
    std::string retval;
    string_append_unescaped(retval, escaped_string);
    return retval;
  }
}
