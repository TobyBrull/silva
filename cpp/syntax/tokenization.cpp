#include "tokenization.hpp"

#include "canopy/filesystem.hpp"

namespace silva {
  using enum token_category_t;

  tokenization_t tokenization_t::copy() const
  {
    return tokenization_t{
        .context         = context,
        .filepath        = filepath,
        .token_locations = token_locations,
        .tokens          = tokens,
    };
  }

  const token_info_t* tokenization_t::token_info_get(const index_t token_index) const
  {
    return &context->token_infos[tokens[token_index]];
  }

  token_id_t token_catalog_get_token_id_from_info(token_catalog_t* tc,
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

  string_or_view_t to_string_impl(const token_position_t& self)
  {
    if (self.tp.is_nullptr()) {
      return string_or_view_t{string_view_t{"unknown token_position"}};
    }
    string_t retval;
    const auto [line, column] = self.tp->token_locations[self.token_index];
    const string_t filename   = self.tp->filepath.filename().string();
    return string_or_view_t{fmt::format("{}:{}:{}", filename, line + 1, column + 1)};
  }

  string_or_view_t to_string_impl(const token_range_t& self)
  {
    constexpr index_t max_num_tokens = 5;
    string_t retval;
    const auto print_tokens = [&retval, &self](const index_t begin, const index_t end) {
      for (index_t token_idx = begin; token_idx < end; ++token_idx) {
        retval += self.tp->token_info_get(token_idx)->str;
        if (token_idx + 1 < end) {
          retval += " ";
        }
      }
    };
    const index_t num_tokens = self.token_end - self.token_begin;
    if (num_tokens <= max_num_tokens) {
      print_tokens(self.token_begin, self.token_end);
    }
    else {
      print_tokens(self.token_begin, self.token_begin + max_num_tokens / 2);
      retval += " ... ";
      print_tokens(self.token_end - max_num_tokens / 2, self.token_end);
    }
    return string_or_view_t{std::move(retval)};
  }

  expected_t<unique_ptr_t<tokenization_t>> tokenize_load(token_catalog_ptr_t tcp,
                                                         filesystem_path_t filepath)
  {
    string_t text = SILVA_EXPECT_FWD(read_file(filepath));
    return tokenize(std::move(tcp), std::move(filepath), std::move(text));
  }

  expected_t<unique_ptr_t<tokenization_t>>
  tokenize(token_catalog_ptr_t tcp, filesystem_path_t filepath, string_view_t text)
  {
    auto retval        = std::make_unique<tokenization_t>();
    retval->filepath   = std::move(filepath);
    retval->context    = tcp;
    index_t text_index = 0;
    tokenization_t::location_t loc;
    while (text_index < text.size()) {
      const auto [tokenized_str, token_cat] = tokenize_one(text.substr(text_index));
      text_index += tokenized_str.size();
      const auto old_loc = loc;
      loc.column += tokenized_str.size();
      if (token_cat != NONE) {
        token_info_t ti{
            .category = token_cat,
            .str      = string_t{tokenized_str},
        };
        const token_id_t tii = token_catalog_get_token_id_from_info(tcp.get(), std::move(ti));
        retval->tokens.push_back(tii);
        retval->token_locations.push_back(old_loc);
      }
      else if (tokenized_str == "\n") {
        loc.line_num += 1;
        loc.column = 0;
      }
    }
    return retval;
  }

  string_or_view_t to_string_impl(const tokenization_t& self)
  {
    string_t retval;
    for (index_t token_index = 0; token_index < self.tokens.size(); ++token_index) {
      const token_id_t tii      = self.tokens[token_index];
      const token_info_t* info  = &self.context->token_infos[tii];
      const auto [line, column] = self.token_locations[token_index];
      retval += fmt::format("[{:3}] {:3}:{:<3} {}\n", token_index, line + 1, column + 1, info->str);
    }
    return string_or_view_t{std::move(retval)};
  }
}
