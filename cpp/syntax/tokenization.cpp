#include "tokenization.hpp"

#include "syntax_farm.hpp"

namespace silva {
  tokenization_t tokenization_t::copy() const
  {
    return tokenization_t{
        .sfp       = sfp,
        .filepath  = filepath,
        .tokens    = tokens,
        .locations = locations,
    };
  }

  const token_info_t* tokenization_t::token_info_get(const index_t token_index) const
  {
    return &sfp->token_infos[tokens[token_index]];
  }

  token_id_t syntax_farm_get_token_id_from_info(syntax_farm_t& sf, const token_info_t& token_info)
  {
    const auto it = sf.token_lookup.find(token_info.str);
    if (it != sf.token_lookup.end()) {
      return it->second;
    }
    else {
      const token_id_t new_token_id = sf.token_infos.size();
      sf.token_infos.push_back(token_info);
      sf.token_lookup.emplace(token_info.str, new_token_id);
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

  void pretty_write_impl(const tokenization_t& self, byte_sink_t* stream)
  {
    for (index_t token_index = 0; token_index < self.tokens.size(); ++token_index) {
      const token_id_t tii         = self.tokens[token_index];
      const token_id_t tic         = self.categories[token_index];
      const token_info_t* tii_info = &self.sfp->token_infos[tii];
      const token_info_t* tic_info = &self.sfp->token_infos[tic];
      const auto [line, column, _] = self.locations[token_index];
      stream->format("[{:3}] {:3}:{:<3} cat={:20} {}\n",
                     token_index,
                     line + 1,
                     column + 1,
                     tic_info->str,
                     tii_info->str);
    }
  }
}
