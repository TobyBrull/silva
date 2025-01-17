#include "tokenization.hpp"

#include "canopy/assert.hpp"
#include "canopy/convert.hpp"
#include "canopy/enum.hpp"

#include <cctype>

namespace silva {
  string_view_t to_string(const token_category_t x)
  {
    static const auto vals = enum_hashmap_to_string<token_category_t>();
    return string_view_t{vals.at(x)};
  }

  string_t tokenization_t::token_data_t::as_string() const
  {
    if (category == token_category_t::IDENTIFIER) {
      return string_t{str};
    }
    else if (category == token_category_t::STRING) {
      SILVA_ASSERT(str.size() >= 2 && str.front() == '"' && str.back() == '"');
      return string_unescaped(str.substr(1, str.size() - 2));
    }
    else {
      SILVA_ASSERT(false);
    }
  }

  string_or_view_t tokenization_t::token_data_t::as_string_or_view() const
  {
    if (category == token_category_t::IDENTIFIER) {
      return string_or_view_t(str);
    }
    else if (category == token_category_t::STRING) {
      SILVA_ASSERT(str.size() >= 2 && str.front() == '"' && str.back() == '"');
      const string_view_t sub_str = str.substr(1, str.size() - 2);
      return string_or_view_unescaped(sub_str);
    }
    else {
      SILVA_ASSERT(false);
    }
  }

  expected_t<double> tokenization_t::token_data_t::as_double() const
  {
    SILVA_EXPECT(category == token_category_t::NUMBER, ASSERT);
    return convert_to<double>(str);
  }

  optional_t<token_id_t> tokenization_t::lookup_token(const string_view_t token_str) const
  {
    const auto it = token_data_lookup.find(token_str);
    if (it != token_data_lookup.end()) {
      return it->second;
    }
    else {
      return none;
    }
  }

  const tokenization_t::line_data_t*
  tokenization_t::binary_search_line(const token_index_t token_index) const
  {
    auto it =
        std::ranges::lower_bound(lines, token_index, std::less<>{}, [](const line_data_t& line) {
          return line.token_index;
        });
    SILVA_ASSERT(!lines.empty());
    if (it == lines.end() || it->token_index != token_index) {
      SILVA_ASSERT(it != lines.begin());
      --it;
    }
    while (true) {
      SILVA_ASSERT(it != lines.end());
      const auto next_it = std::next(it);
      if (next_it != lines.end() && next_it->token_index == token_index) {
        it = next_it;
      }
      else {
        break;
      }
    }
    return &(*it);
  }

  namespace impl {
    constexpr char whitespace_chars[] = {' '};
    constexpr char identifier_chars[] = {'_'};
    constexpr char oplet_chars[]      = {
        // parentheses
        '[',
        ']',
        '(',
        ')',
        '{',
        '}',
    };
    constexpr char operator_chars[] = {
        // punctuation
        '\'',
        ',',
        '.',
        ':',
        // comparison
        '<',
        '>',
        '=',
        // arithmetic
        '-',
        '+',
        '*',
        '/',
        '%',
        // logical
        '&',
        '|',
        // other
        '^',
        '~',
        '@',
        '!',
        '?',
        ';',
    };
    constexpr char number_chars[] = {'.', '`', 'e'};

    template<index_t Size>
    constexpr bool is_one_of(const char c, const char (&char_array)[Size])
    {
      for (index_t i = 0; i < Size; ++i) {
        if (c == char_array[i]) {
          return true;
        }
      }
      return false;
    }

    template<typename Predicate>
    index_t find_token_length(const string_view_t rest, Predicate predicate)
    {
      SILVA_ASSERT(rest.size() >= 1);
      index_t index = 1;
      while (index < rest.size()) {
        const char x = rest[index];
        if (!predicate(x)) {
          break;
        }
        index += 1;
      }
      return index;
    }

    index_t find_whitespace_length(const string_view_t rest)
    {
      return find_token_length(rest, [](const char x) { return is_one_of(x, whitespace_chars); });
    }

    optional_t<index_t> find_string_length(const string_view_t rest)
    {
      SILVA_ASSERT(rest.size() >= 1);
      index_t index = 1;
      while (index < rest.size()) {
        if (rest[index] == '"' && rest[index - 1] != '\\') {
          index += 1;
          break;
        }
        index += 1;
      }
      if (index < rest.size() && rest[index - 1] == '"') {
        return index;
      }
      else {
        return none;
      }
    }

    index_t find_comment_length(const string_view_t rest)
    {
      index_t index = 1;
      while (index < rest.size() && rest[index] != '\n') {
        index += 1;
      }
      return index;
    }

    tokenization_t::token_data_t tokenize_one(string_view_t text)
    {
      SILVA_ASSERT(!text.empty());
      const char c = text.front();
      if (is_one_of(c, whitespace_chars)) {
        const index_t len = find_whitespace_length(text);
        return tokenization_t::token_data_t{
            .str      = text.substr(0, len),
            .category = token_category_t::INVALID,
        };
      }
      else if (std::isalpha(c) || is_one_of(c, identifier_chars)) {
        const index_t len = find_token_length(text, [](const char x) {
          return std::isalnum(x) || is_one_of(x, identifier_chars);
        });
        return tokenization_t::token_data_t{
            .str      = text.substr(0, len),
            .category = token_category_t::IDENTIFIER,
        };
      }
      else if (is_one_of(c, oplet_chars)) {
        return tokenization_t::token_data_t{
            .str      = text.substr(0, 1),
            .category = token_category_t::OPERATOR,
        };
      }
      else if (is_one_of(c, operator_chars)) {
        const index_t len =
            find_token_length(text, [](const char x) { return is_one_of(x, operator_chars); });
        return tokenization_t::token_data_t{
            .str      = text.substr(0, len),
            .category = token_category_t::OPERATOR,
        };
      }
      else if (c == '"') {
        const std::optional<index_t> maybe_length = find_string_length(text);
        if (maybe_length.has_value()) {
          return tokenization_t::token_data_t{
              .str      = text.substr(0, maybe_length.value()),
              .category = token_category_t::STRING,
          };
        }
        else {
          return tokenization_t::token_data_t{
              .str      = text,
              .category = token_category_t::INVALID,
          };
        }
      }
      else if (c == '#') {
        const index_t len = find_comment_length(text);
        return {
            .str      = text.substr(0, len),
            .category = token_category_t::INVALID,
        };
      }
      else if (std::isdigit(c)) {
        const index_t len = find_token_length(text, [](const char x) {
          return std::isdigit(x) || is_one_of(x, number_chars);
        });
        return {
            .str      = text.substr(0, len),
            .category = token_category_t::NUMBER,
        };
      }
      else if (c == '\n') {
        return {
            .str      = text.substr(0, 1),
            .category = token_category_t::INVALID,
        };
      }
      else {
        return {
            .str      = text.substr(0, 1),
            .category = token_category_t::INVALID,
        };
      }
    }

    void append_token(tokenization_t* tokenization, const tokenization_t::token_data_t* td)
    {
      optional_t<token_id_t> x = tokenization->lookup_token(td->str);
      if (x.has_value()) {
        tokenization->tokens.push_back(x.value());
      }
      else {
        const index_t new_token_id = tokenization->token_datas.size();
        tokenization->token_datas.push_back(*td);
        tokenization->tokens.push_back(new_token_id);
        tokenization->token_data_lookup.emplace(td->str, new_token_id);
      }
    }

    void start_new_line(tokenization_t* tokenization, const index_t source_code_offset)
    {
      tokenization->lines.push_back(tokenization_t::line_data_t{
          .token_index        = static_cast<index_t>(tokenization->tokens.size()),
          .source_code_offset = source_code_offset,
      });
    }
  }

  expected_t<tokenization_t> tokenize(const_ptr_t<source_code_t> source_code)
  {
    tokenization_t retval{.source_code = std::move(source_code)};
    impl::start_new_line(&retval, 0);
    index_t text_index = 0;
    string_view_t text = retval.source_code->text;
    while (text_index < text.size()) {
      const tokenization_t::token_data_t td = impl::tokenize_one(text.substr(text_index));
      text_index += td.str.size();
      if (td.category != token_category_t::INVALID) {
        impl::append_token(&retval, &td);
      }
      else if (td.str == "\n") {
        impl::start_new_line(&retval, text_index);
      }
    }
    return {std::move(retval)};
  }

  source_code_location_t
  tokenization_t::compute_source_code_location(const token_index_t token_index) const
  {
    const auto* line_data = binary_search_line(token_index);

    const string_view_t rest =
        string_view_t{source_code->text}.substr(line_data->source_code_offset);
    const index_t line_token_index = token_index - line_data->token_index;

    index_t seen_tokens = 0;
    index_t column      = 0;
    while (column < rest.size()) {
      const tokenization_t::token_data_t td = impl::tokenize_one(rest.substr(column));
      if (td.category == token_category_t::INVALID) {
        column += td.str.size();
      }
      else {
        SILVA_ASSERT(td.str != "\n");
        seen_tokens += 1;
        if (seen_tokens > line_token_index) {
          break;
        }
        else {
          column += td.str.size();
        }
      }
    }

    source_code_location_t retval{.source_code = source_code.get()};
    retval.line   = line_data - lines.data();
    retval.column = column;
    return retval;
  }

  string_t tokenization_t::to_string() const
  {
    string_t retval;
    for (token_index_t index = 0; index < tokens.size(); ++index) {
      const token_id_t id              = tokens[index];
      const auto& td                   = token_datas[id];
      const source_code_location_t scl = compute_source_code_location(index);
      retval += fmt::format("[{:3}] {:3}:{:<3} {}\n", index, scl.line + 1, scl.column + 1, td.str);
    }
    return retval;
  }

  source_code_location_t token_position_t::compute_source_code_location() const
  {
    return tokenization->compute_source_code_location(index);
  }
}
