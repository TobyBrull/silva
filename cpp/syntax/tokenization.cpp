#include "tokenization.hpp"

#include "canopy/filesystem.hpp"

#include <algorithm>

namespace silva {
  using enum token_category_t;

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
    void start_new_line(tokenization_t* tokenization, const index_t source_code_offset)
    {
      tokenization->lines.push_back(tokenization_t::line_data_t{
          .token_index        = static_cast<index_t>(tokenization->tokens.size()),
          .source_code_offset = source_code_offset,
      });
    }
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
      const auto [tokenized_str, token_cat] = tokenize_one(text.substr(text_index));
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
      const auto [tokenized_str, token_cat] = tokenize_one(rest.substr(column));
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
}
