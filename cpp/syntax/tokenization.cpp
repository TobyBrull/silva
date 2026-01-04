#include "tokenization.hpp"

#include "canopy/filesystem.hpp"

#include "syntax_ward.hpp"

namespace silva {
  using enum token_category_t;

  tokenization_t tokenization_t::copy() const
  {
    return tokenization_t{
        .swp       = swp,
        .filepath  = filepath,
        .tokens    = tokens,
        .locations = locations,
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

  void pretty_write_impl(const token_location_t& self, byte_sink_t* stream)
  {
    if (self.tp.is_nullptr()) {
      stream->write_str("unknown token-location");
      return;
    }
    stream->format("{}:", self.tp->filepath.filename().string());
    const file_location_t loc = [&] {
      if (self.token_index < self.tp->locations.size()) {
        return self.tp->locations[self.token_index];
      }
      else {
        return file_location_eof;
      }
    }();
    silva::pretty_write(loc, stream);
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
    string_t source_code  = SILVA_EXPECT_FWD(read_file(filepath));
    tokenization_ptr_t tp = SILVA_EXPECT_FWD_PLAIN(
        tokenize(std::move(swp), std::move(filepath), std::move(source_code)));
    return tp;
  }

  expected_t<tokenization_ptr_t>
  tokenize(syntax_ward_ptr_t swp, filesystem_path_t filepath, string_view_t source_code)
  {
    auto retval      = std::make_unique<tokenization_t>();
    retval->filepath = std::move(filepath);
    retval->swp      = swp;
    file_location_t loc;
    while (loc.byte_offset < source_code.size()) {
      const auto [tokenized_str, token_cat] = tokenize_one(source_code.substr(loc.byte_offset));
      SILVA_EXPECT(token_cat != INVALID,
                   MINOR,
                   "token {} at [{}] is invalid",
                   string_t{tokenized_str},
                   loc);
      loc.byte_offset += tokenized_str.size();
      const auto old_loc = loc;
      loc.column += tokenized_str.size();
      if (token_cat == WHITESPACE || token_cat == COMMENT) {
        if (tokenized_str == "\n") {
          loc.line_num += 1;
          loc.column = 0;
        }
      }
      else {
        token_info_t ti{
            .category = token_cat,
            .str      = string_t{tokenized_str},
        };
        const token_id_t tii = syntax_ward_get_token_id_from_info(*swp, std::move(ti));
        retval->tokens.push_back(tii);
        retval->locations.push_back(old_loc);
      }
    }
    return swp->add(std::move(retval));
  }

  void pretty_write_impl(const tokenization_t& self, byte_sink_t* stream)
  {
    for (index_t token_index = 0; token_index < self.tokens.size(); ++token_index) {
      const token_id_t tii         = self.tokens[token_index];
      const token_info_t* info     = &self.swp->token_infos[tii];
      const auto [line, column, _] = self.locations[token_index];
      stream->format("[{:3}] {:3}:{:<3} {}\n", token_index, line + 1, column + 1, info->str);
    }
  }
}
