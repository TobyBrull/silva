#include "convert.hpp"

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

  bool string_escape_is_trivial(const string_view_t unescaped_string)
  {
    for (const char c: unescaped_string) {
      switch (c) {
        case '\n':
          return false;
        case '\r':
          return false;
        case '\\':
          return false;
        case '\"':
          return false;
        default:
          break;
      }
    }
    return true;
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

  bool string_unescape_is_trivial(const string_view_t escaped_string)
  {
    for (size_t i = 0; i < escaped_string.size(); ++i) {
      if (escaped_string[i] == '\\' && i + 1 < escaped_string.size()) {
        switch (escaped_string[i + 1]) {
          case 'n':
            return false;
          case 'r':
            return false;
          case '\\':
            return false;
          case '\"':
            return false;
          default:
            break;
        }
      }
    }
    return true;
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

  string_or_view_t string_or_view_escaped(const string_view_t unescaped_string)
  {
    if (string_escape_is_trivial(unescaped_string)) {
      return string_or_view_t(unescaped_string);
    }
    else {
      return string_or_view_t(string_escaped(unescaped_string));
    }
  }

  string_or_view_t string_or_view_unescaped(const string_view_t escaped_string)
  {
    if (string_unescape_is_trivial(escaped_string)) {
      return string_or_view_t(escaped_string);
    }
    else {
      return string_or_view_t(string_unescaped(escaped_string));
    }
  }
}
