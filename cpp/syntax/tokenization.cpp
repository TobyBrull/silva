#include "tokenization.hpp"

#include "canopy/filesystem.hpp"

#include <algorithm>

namespace silva {
  using enum token_category_t;

  syntax_context_t::syntax_context_t()
  {
    token_infos.emplace_back();
    token_lookup[""] = token_id_none;
    const name_info_t fni{0, 0};
    name_infos.emplace_back(fni);
    name_lookup.emplace(fni, 0);
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

  expected_t<token_id_t> syntax_context_t::token_id(const string_view_t token_str)
  {
    const auto it = token_lookup.find(string_t{token_str});
    if (it != token_lookup.end()) {
      return it->second;
    }
    else {
      const auto [tokenized_str, token_cat] = impl::tokenize_one(token_str);
      SILVA_EXPECT(tokenized_str.size() == token_str.size(), MINOR);
      const token_id_t new_token_id = token_infos.size();
      token_infos.push_back(token_info_t{token_cat, string_t{tokenized_str}});
      token_lookup.emplace(tokenized_str, new_token_id);
      return new_token_id;
    }
  }

  expected_t<token_id_t> syntax_context_t::token_id_in_string(const token_id_t ti)
  {
    const string_t str{SILVA_EXPECT_FWD(token_infos[ti].string_as_plain_contained())};
    return token_id(str);
  }

  token_id_t token_context_get_token_id_from_info(syntax_context_t* tc,
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

  expected_t<unique_ptr_t<tokenization_t>> tokenize_load(syntax_context_ptr_t tcp,
                                                         filesystem_path_t filepath)
  {
    string_t text = SILVA_EXPECT_FWD(read_file(filepath));
    return tokenize(std::move(tcp), std::move(filepath), std::move(text));
  }

  expected_t<unique_ptr_t<tokenization_t>>
  tokenize(syntax_context_ptr_t tcp, filesystem_path_t filepath, string_t text_arg)
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
        const token_id_t tii =
            token_context_get_token_id_from_info(tcp.get(),
                                                 token_info_t{
                                                     .category = token_cat,
                                                     .str      = string_t{tokenized_str},
                                                 });
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

  name_id_t syntax_context_t::name_id(const name_id_t parent_name, const token_id_t base_name)
  {
    const name_info_t fni{parent_name, base_name};
    const auto [it, inserted] = name_lookup.emplace(fni, name_infos.size());
    if (inserted) {
      name_infos.push_back(fni);
    }
    return it->second;
  }

  name_id_t syntax_context_t::name_id_span(const name_id_t parent_name,
                                           const span_t<const token_id_t> token_ids)
  {
    name_id_t retval = parent_name;
    for (const token_id_t token_id: token_ids) {
      retval = name_id(retval, token_id);
    }
    return retval;
  }

  bool syntax_context_t::name_id_is_parent(const name_id_t parent_name, token_id_t child_name) const
  {
    while (true) {
      if (child_name == parent_name) {
        return true;
      }
      if (child_name == name_id_root) {
        return false;
      }
      child_name = name_infos[child_name].parent_name;
    }
  }

  name_id_t syntax_context_t::name_id_lca(const name_id_t lhs, const name_id_t rhs) const
  {
    // TODO: O(1) time, O(n) memory ?
    const auto fni_path = [this](name_id_t x) {
      vector_t<name_id_t> retval;
      while (true) {
        retval.push_back(x);
        if (x == name_id_root) {
          break;
        }
        x = name_infos[x].parent_name;
      }
      std::ranges::reverse(retval);
      return retval;
    };
    const auto lhs_path = fni_path(lhs);
    const auto rhs_path = fni_path(rhs);
    const index_t n     = std::min(lhs_path.size(), rhs_path.size());
    index_t common      = 0;
    while (common + 1 < n && lhs_path[common + 1] == rhs_path[common + 1]) {
      common += 1;
    }
    SILVA_ASSERT(lhs_path[common] == rhs_path[common]);
    return lhs_path[common];
  }

  string_t name_id_style_t::absolute(const name_id_t target_fni) const
  {
    if (target_fni == name_id_root) {
      return tcp->token_infos[root].str;
    }
    const name_info_t& fni = tcp->name_infos[target_fni];
    return absolute(fni.parent_name) + tcp->token_infos[separator].str +
        tcp->token_infos[fni.base_name].str;
  }

  string_t name_id_style_t::relative(const name_id_t current_fni, const name_id_t target_fni) const
  {
    const name_id_t lca = tcp->name_id_lca(current_fni, target_fni);

    string_t first_part;
    {
      name_id_t curr = current_fni;
      while (curr != lca) {
        if (!first_part.empty()) {
          first_part += tcp->token_infos[separator].str;
        }
        first_part += tcp->token_infos[parent].str;
        curr = tcp->name_infos[curr].parent_name;
      }
    }

    string_t second_part;
    {
      name_id_t curr = target_fni;
      while (curr != lca) {
        if (!second_part.empty()) {
          second_part = tcp->token_infos[separator].str + second_part;
        }
        const name_info_t* fni = &tcp->name_infos[curr];
        second_part            = tcp->token_infos[fni->base_name].str + second_part;
        curr                   = tcp->name_infos[curr].parent_name;
      }
    }
    if (!first_part.empty() && !second_part.empty()) {
      return first_part + tcp->token_infos[separator].str + second_part;
    }
    else if (first_part.empty() && second_part.empty()) {
      return tcp->token_infos[current].str;
    }
    else {
      return first_part + second_part;
    }
  }
}
