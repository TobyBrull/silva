#include "tokenization.hpp"

#include "canopy/filesystem.hpp"

#include "syntax_ward.hpp"

namespace silva {
  using enum token_category_t;

  tokenization_t tokenization_t::copy() const
  {
    return tokenization_t{
        .swp             = swp,
        .filepath        = filepath,
        .token_locations = token_locations,
        .tokens          = tokens,
    };
  }

  const token_info_t* tokenization_t::token_info_get(const index_t token_index) const
  {
    return &swp->token_infos[tokens[token_index]];
  }

  token_id_t syntax_ward_get_token_id_from_info(syntax_ward_t& sw, const token_info_t& token_info)
  {
    const auto it = sw.token_lookup.find(token_info.str);
    if (it != sw.token_lookup.end()) {
      return it->second;
    }
    else {
      const token_id_t new_token_id = sw.token_infos.size();
      sw.token_infos.push_back(token_info);
      sw.token_lookup.emplace(token_info.str, new_token_id);
      return new_token_id;
    }
  }

  void pretty_write_impl(const token_position_t& self, byte_sink_t* stream)
  {
    if (self.tp.is_nullptr()) {
      stream->write_str("unknown token_position");
      return;
    }
    string_t retval;
    const string_t filename = self.tp->filepath.filename().string();
    if (self.token_index < self.tp->token_locations.size()) {
      const auto [line, column] = self.tp->token_locations[self.token_index];
      stream->format("{}:{}:{}", filename, line + 1, column + 1);
    }
    else {
      stream->format("{}:EOF", filename);
    }
  }

  void pretty_write_impl(const token_range_t& self, byte_sink_t* stream)
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
    stream->write_str(retval);
  }

  expected_t<tokenization_ptr_t> tokenize_load(syntax_ward_ptr_t swp, filesystem_path_t filepath)
  {
    string_t text = SILVA_EXPECT_FWD(read_file(filepath));
    tokenization_ptr_t tp =
        SILVA_EXPECT_FWD(tokenize(std::move(swp), std::move(filepath), std::move(text)));
    return tp;
  }

  expected_t<tokenization_ptr_t>
  tokenize(syntax_ward_ptr_t swp, filesystem_path_t filepath, string_view_t text)
  {
    auto retval        = std::make_unique<tokenization_t>();
    retval->filepath   = std::move(filepath);
    retval->swp        = swp;
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
        const token_id_t tii = syntax_ward_get_token_id_from_info(*swp, std::move(ti));
        retval->tokens.push_back(tii);
        retval->token_locations.push_back(old_loc);
      }
      else if (tokenized_str == "\n") {
        loc.line_num += 1;
        loc.column = 0;
      }
    }
    return swp->add(std::move(retval));
  }

  void pretty_write_impl(const tokenization_t& self, byte_sink_t* stream)
  {
    for (index_t token_index = 0; token_index < self.tokens.size(); ++token_index) {
      const token_id_t tii      = self.tokens[token_index];
      const token_info_t* info  = &self.swp->token_infos[tii];
      const auto [line, column] = self.token_locations[token_index];
      stream->format("[{:3}] {:3}:{:<3} {}\n", token_index, line + 1, column + 1, info->str);
    }
  }
}
