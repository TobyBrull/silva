#include "tokenization.hpp"

#include "canopy/convert.hpp"
#include "canopy/enum.hpp"
#include "canopy/filesystem.hpp"

namespace silva {
  using enum token_category_t;

  string_view_t to_string(const token_category_t x)
  {
    static const auto vals = enum_hashmap_to_string<token_category_t>();
    return string_view_t{vals.at(x)};
  }

  string_t token_info_t::as_string() const
  {
    if (category == IDENTIFIER) {
      return string_t{str};
    }
    else if (category == STRING) {
      SILVA_ASSERT(str.size() >= 2 && str.front() == '"' && str.back() == '"');
      return string_unescaped(str.substr(1, str.size() - 2));
    }
    else {
      SILVA_ASSERT(false);
    }
  }

  string_or_view_t token_info_t::as_string_or_view() const
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

  expected_t<double> token_info_t::as_double() const
  {
    SILVA_EXPECT(category == token_category_t::NUMBER, ASSERT);
    return convert_to<double>(str);
  }

  const token_info_t* tokenization_t::token_info_get(const index_t token_index) const
  {
    return &context->_token_infos[tokens[token_index]];
  }

  const tokenization_t::line_data_t*
  tokenization_t::binary_search_line(const index_t token_index) const
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

    token_info_t tokenize_one(const string_view_t text)
    {
      SILVA_ASSERT(!text.empty());
      const char c = text.front();
      if (is_one_of(c, whitespace_chars)) {
        const index_t len = find_whitespace_length(text);
        return token_info_t{
            .str      = text.substr(0, len),
            .category = token_category_t::INVALID,
        };
      }
      else if (std::isalpha(c) || is_one_of(c, identifier_chars)) {
        const index_t len = find_token_length(text, [](const char x) {
          return std::isalnum(x) || is_one_of(x, identifier_chars);
        });
        return token_info_t{
            .str      = text.substr(0, len),
            .category = token_category_t::IDENTIFIER,
        };
      }
      else if (is_one_of(c, oplet_chars)) {
        return token_info_t{
            .str      = text.substr(0, 1),
            .category = token_category_t::OPERATOR,
        };
      }
      else if (is_one_of(c, operator_chars)) {
        const index_t len =
            find_token_length(text, [](const char x) { return is_one_of(x, operator_chars); });
        return token_info_t{
            .str      = text.substr(0, len),
            .category = token_category_t::OPERATOR,
        };
      }
      else if (c == '"') {
        const std::optional<index_t> maybe_length = find_string_length(text);
        if (maybe_length.has_value()) {
          return token_info_t{
              .str      = text.substr(0, maybe_length.value()),
              .category = token_category_t::STRING,
          };
        }
        else {
          return token_info_t{
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

    void start_new_line(tokenization_t* tokenization, const index_t source_code_offset)
    {
      tokenization->lines.push_back(tokenization_t::line_data_t{
          .token_index        = static_cast<index_t>(tokenization->tokens.size()),
          .source_code_offset = source_code_offset,
      });
    }
  }

  token_info_index_t token_context_get_index(const string_view_t token_str)
  {
    const auto token_info = impl::tokenize_one(token_str);
    return token_context_get_index(token_info);
  }

  token_info_index_t token_context_get_index(const token_info_t& token_info)
  {
    token_context_t* tc = token_context_t::get();
    const auto it       = tc->_token_lookup.find(token_info.str);
    if (it != tc->_token_lookup.end()) {
      return it->second;
    }
    else {
      const token_info_index_t new_token_id = tc->_token_infos.size();
      tc->_token_infos.push_back(token_info);
      tc->_token_lookup.emplace(token_info.str, new_token_id);
      return new_token_id;
    }
  }

  const token_info_t* token_context_get_info(const token_info_index_t tii)
  {
    return &token_context_t::get()->_token_infos[tii];
  }

  expected_t<const tokenization_t*> token_context_load(filesystem_path_t filepath)
  {
    string_t text = SILVA_EXPECT_FWD(read_file(filepath));
    return token_context_make(filepath, string_or_view_t{std::move(text)});
  }

  expected_t<const tokenization_t*> token_context_make(filesystem_path_t filepath,
                                                       string_or_view_t text_arg)
  {
    token_context_t* context = token_context_t::get();
    auto tt                  = std::make_unique<tokenization_t>();
    tt->filepath             = std::move(filepath);
    tt->text                 = std::move(text_arg);
    tt->context              = context;
    impl::start_new_line(tt.get(), 0);
    index_t text_index       = 0;
    const string_view_t text = tt->text.get_view();
    while (text_index < text.size()) {
      const token_info_t token_info = impl::tokenize_one(text.substr(text_index));
      text_index += token_info.str.size();
      if (token_info.category != INVALID) {
        const token_info_index_t tii = token_context_get_index(token_info);
        tt->tokens.push_back(tii);
      }
      else if (token_info.str == "\n") {
        impl::start_new_line(tt.get(), text_index);
      }
    }
    const tokenization_t* retval = tt.get();
    context->_tokenizations.push_back(std::move(tt));
    return retval;
  }

  tuple_t<index_t, index_t> tokenization_t::compute_line_and_column(const index_t token_index) const
  {
    const auto* line_data          = binary_search_line(token_index);
    const string_view_t rest       = text.get_view().substr(line_data->source_code_offset);
    const index_t line_token_index = token_index - line_data->token_index;
    index_t seen_tokens            = 0;
    index_t column                 = 0;
    while (column < rest.size()) {
      const token_info_t info = impl::tokenize_one(rest.substr(column));
      if (info.category == token_category_t::INVALID) {
        column += info.str.size();
      }
      else {
        SILVA_ASSERT(info.str != "\n");
        seen_tokens += 1;
        if (seen_tokens > line_token_index) {
          break;
        }
        else {
          column += info.str.size();
        }
      }
    }
    const index_t line = line_data - lines.data();
    return {line, column};
  }

  string_t tokenization_t::to_string() const
  {
    string_t retval;
    for (index_t token_index = 0; token_index < tokens.size(); ++token_index) {
      const token_info_index_t tii = tokens[token_index];
      const token_info_t* info     = token_context_get_info(tii);
      const auto [line, column]    = compute_line_and_column(token_index);
      retval += fmt::format("[{:3}] {:3}:{:<3} {}\n", token_index, line + 1, column + 1, info->str);
    }
    return retval;
  }
}
