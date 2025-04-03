#include "tokenization.hpp"

#include "canopy/filesystem.hpp"

#include <algorithm>

namespace silva {
  using enum token_category_t;

  tokenization_t tokenization_t::copy() const
  {
    return tokenization_t{
        .context         = context,
        .filepath        = filepath,
        .text            = text,
        .tokens          = tokens,
        .token_locations = token_locations,
    };
  }

  const token_info_t* tokenization_t::token_info_get(const index_t token_index) const
  {
    return &context->token_infos[tokens[token_index]];
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
    auto retval        = std::make_unique<tokenization_t>();
    retval->filepath   = std::move(filepath);
    retval->text       = std::move(text_arg);
    retval->context    = tcp;
    index_t text_index = 0;
    tokenization_t::location_t loc;
    const string_view_t text = retval->text;
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
        const token_id_t tii = token_context_get_token_id_from_info(tcp.get(), std::move(ti));
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

  string_t tokenization_t::to_string() const
  {
    string_t retval;
    for (index_t token_index = 0; token_index < tokens.size(); ++token_index) {
      const token_id_t tii      = tokens[token_index];
      const token_info_t* info  = &context->token_infos[tii];
      const auto [line, column] = token_locations[token_index];
      retval += fmt::format("[{:3}] {:3}:{:<3} {}\n", token_index, line + 1, column + 1, info->str);
    }
    return retval;
  }
}
