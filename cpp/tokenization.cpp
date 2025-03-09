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

  expected_t<string_view_t> token_info_t::string_as_plain_contained() const
  {
    SILVA_EXPECT(category == STRING, MINOR);
    SILVA_EXPECT(str.size() >= 2, MINOR);
    return string_view_t{str}.substr(1, str.size() - 2);
  }

  expected_t<double> token_info_t::number_as_double() const
  {
    SILVA_EXPECT(category == NUMBER, MINOR);
    return convert_to<double>(str);
  }

  hash_value_t hash_impl(const full_name_info_t& x)
  {
    return hash(tuple_t<full_name_id_t, token_id_t>{x.parent_name, x.base_name});
  }

  token_context_t::token_context_t()
  {
    token_infos.emplace_back();
    token_lookup[""] = token_id_none;
    const full_name_info_t fni{0, 0};
    full_name_infos.emplace_back(fni);
    full_name_lookup.emplace(fni, 0);
  }

  tokenization_t tokenization_t::copy() const
  {
    return tokenization_t{
        .context  = context,
        .filepath = filepath,
        .text     = text,
        .tokens   = tokens,
        .lines    = lines,
    };
  }

  const token_info_t* tokenization_t::token_info_get(const index_t token_index) const
  {
    return &context->token_infos[tokens[token_index]];
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
        // other
        '~',
    };
    constexpr char operator_chars[] = {
        // punctuation
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
        '@',
        '!',
        '?',
        ';',
        '$',
        '`',
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
        if (rest[index] == '\'' && rest[index - 1] != '\\') {
          index += 1;
          break;
        }
        index += 1;
      }
      if (index < rest.size() && rest[index - 1] == '\'') {
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

    tuple_t<string_view_t, token_category_t> tokenize_one(const string_view_t text)
    {
      SILVA_ASSERT(!text.empty());
      const char c = text.front();
      if (is_one_of(c, whitespace_chars)) {
        const index_t len = find_whitespace_length(text);
        return {text.substr(0, len), NONE};
      }
      else if (std::isalpha(c) || is_one_of(c, identifier_chars)) {
        const index_t len = find_token_length(text, [](const char x) {
          return std::isalnum(x) || is_one_of(x, identifier_chars);
        });
        return {text.substr(0, len), IDENTIFIER};
      }
      else if (is_one_of(c, oplet_chars)) {
        return {text.substr(0, 1), OPERATOR};
      }
      else if (is_one_of(c, operator_chars)) {
        const index_t len =
            find_token_length(text, [](const char x) { return is_one_of(x, operator_chars); });
        return {text.substr(0, len), OPERATOR};
      }
      else if (c == '\'') {
        const std::optional<index_t> maybe_length = find_string_length(text);
        if (maybe_length.has_value()) {
          return {text.substr(0, maybe_length.value()), STRING};
        }
        else {
          return {text, NONE};
        }
      }
      else if (c == '#') {
        const index_t len = find_comment_length(text);
        return {text.substr(0, len), NONE};
      }
      else if (std::isdigit(c)) {
        const index_t len = find_token_length(text, [](const char x) {
          return std::isdigit(x) || is_one_of(x, number_chars);
        });
        return {text.substr(0, len), NUMBER};
      }
      else if (c == '\n') {
        return {text.substr(0, 1), NONE};
      }
      else {
        return {text.substr(0, 1), NONE};
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

  token_id_t token_context_t::token_id(const string_view_t token_str)
  {
    const auto it = token_lookup.find(string_t{token_str});
    if (it != token_lookup.end()) {
      return it->second;
    }
    else {
      const auto [tokenized_str, token_cat] = impl::tokenize_one(token_str);
      SILVA_ASSERT(tokenized_str == token_str);
      const token_id_t new_token_id = token_infos.size();
      token_infos.push_back(token_info_t{string_t{tokenized_str}, token_cat});
      token_lookup.emplace(tokenized_str, new_token_id);
      return new_token_id;
    }
  }

  expected_t<token_id_t> token_context_t::token_id_unquoted(const token_id_t ti)
  {
    const string_t str{SILVA_EXPECT_FWD(token_infos[ti].string_as_plain_contained(), MINOR)};
    return token_id(str);
  }

  token_id_t token_context_get_token_id_from_info(token_context_t* tc,
                                                  const token_info_t& token_info)
  {
    const auto it = tc->token_lookup.find(token_info.str);
    if (it != tc->token_lookup.end()) {
      return it->second;
    }
    else {
      const token_id_t new_token_id = tc->token_infos.size();
      tc->token_infos.push_back(token_info);
      tc->token_lookup.emplace(token_info.str, new_token_id);
      return new_token_id;
    }
  }

  expected_t<unique_ptr_t<tokenization_t>> tokenize_load(token_context_ptr_t tcp,
                                                         filesystem_path_t filepath)
  {
    string_t text = SILVA_EXPECT_FWD(read_file(filepath));
    return tokenize(std::move(tcp), std::move(filepath), std::move(text));
  }

  expected_t<unique_ptr_t<tokenization_t>>
  tokenize(token_context_ptr_t tcp, filesystem_path_t filepath, string_t text_arg)
  {
    auto retval      = std::make_unique<tokenization_t>();
    retval->filepath = std::move(filepath);
    retval->text     = std::move(text_arg);
    retval->context  = tcp;
    impl::start_new_line(retval.get(), 0);
    index_t text_index       = 0;
    const string_view_t text = retval->text;
    while (text_index < text.size()) {
      const auto [tokenized_str, token_cat] = impl::tokenize_one(text.substr(text_index));
      text_index += tokenized_str.size();
      if (token_cat != NONE) {
        const token_id_t tii = token_context_get_token_id_from_info(
            tcp.get(),
            token_info_t{.str = string_t{tokenized_str}, .category = token_cat});
        retval->tokens.push_back(tii);
      }
      else if (tokenized_str == "\n") {
        impl::start_new_line(retval.get(), text_index);
      }
    }
    return retval;
  }

  tuple_t<index_t, index_t> tokenization_t::compute_line_and_column(const index_t token_index) const
  {
    const auto* line_data          = binary_search_line(token_index);
    const string_view_t rest       = string_view_t{text}.substr(line_data->source_code_offset);
    const index_t line_token_index = token_index - line_data->token_index;
    index_t seen_tokens            = 0;
    index_t column                 = 0;
    while (column < rest.size()) {
      const auto [tokenized_str, token_cat] = impl::tokenize_one(rest.substr(column));
      if (token_cat == NONE) {
        column += tokenized_str.size();
      }
      else {
        SILVA_ASSERT(tokenized_str != "\n");
        seen_tokens += 1;
        if (seen_tokens > line_token_index) {
          break;
        }
        else {
          column += tokenized_str.size();
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
      const token_id_t tii      = tokens[token_index];
      const token_info_t* info  = &context->token_infos[tii];
      const auto [line, column] = compute_line_and_column(token_index);
      retval += fmt::format("[{:3}] {:3}:{:<3} {}\n", token_index, line + 1, column + 1, info->str);
    }
    return retval;
  }

  full_name_id_t token_context_t::full_name_id(const full_name_id_t parent_name,
                                               const token_id_t base_name)
  {
    const full_name_info_t fni{parent_name, base_name};
    const auto [it, inserted] = full_name_lookup.emplace(fni, full_name_infos.size());
    if (inserted) {
      full_name_infos.push_back(fni);
    }
    return it->second;
  }

  full_name_id_t token_context_t::full_name_id(const span_t<const token_id_t> token_ids)
  {
    full_name_id_t retval = full_name_id_none;
    for (const token_id_t token_id: token_ids) {
      retval = full_name_id(retval, token_id);
    }
    return retval;
  }

  bool token_context_t::full_name_id_is_parent(const full_name_id_t parent_name,
                                               token_id_t child_name)
  {
    while (true) {
      if (child_name == parent_name) {
        return true;
      }
      if (child_name == full_name_id_none) {
        return false;
      }
      child_name = full_name_infos[child_name].parent_name;
    }
  }

  string_t token_context_t::full_name_to_string(const full_name_id_t full_name_id,
                                                const string_view_t separator)
  {
    if (full_name_id == full_name_id_none) {
      return {};
    }
    const full_name_info_t* fni = &full_name_infos[full_name_id];
    const token_info_t* ti      = &token_infos[fni->base_name];
    return full_name_to_string(fni->parent_name, separator) + string_t{separator} + ti->str;
  }
}
