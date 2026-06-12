#include "tokenization.hpp"

#include "seed.lexicon.hpp"
#include "syntax_farm.hpp"

namespace silva {
  index_t tokenization_t::size() const
  {
    return tokens.size();
  }

  file_location_t tokenization_t::location_at(const index_t idx) const
  {
    if (idx < size()) {
      const index_t frag_idx = tokens[idx].frag_idx_begin;
      return fs.fp->location_at(frag_idx);
    }
    return file_location_eof;
  }

  const token_info_t* tokenization_t::token_info_get(const index_t token_index) const
  {
    const token_id_t ti = tokens[token_index].token_id;
    return &sfp->token_infos[ti];
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
    silva::pretty_write(self.tp->location_at(self.token_index), stream);
  }

  index_t token_span_t::size() const
  {
    return end - begin;
  }

  token_span_t::operator span_t<const token_t>() const
  {
    return span_t<const token_t>(tp->tokens.data() + begin, end - begin);
  }

  void pretty_write_impl(const token_span_t& self, byte_sink_t* stream)
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
    const index_t num_tokens = self.end - self.begin;
    if (num_tokens <= max_num_tokens) {
      print_tokens(self.begin, self.end);
    }
    else {
      print_tokens(self.begin, self.begin + max_num_tokens / 2);
      retval += " ... ";
      print_tokens(self.end - max_num_tokens / 2, self.end);
    }
    stream->write_str(retval);
  }

  void pretty_write_impl(const tokenization_t& self, byte_sink_t* stream)
  {
    const auto& lexicon = self.sfp->get_lexicon<silva::seed::lexicon_t>();
    for (index_t token_index = 0; token_index < self.size(); ++token_index) {
      const token_t& token         = self.tokens[token_index];
      const token_info_t* tii_info = &self.sfp->token_infos[token.token_id];
      const string_t token_cat_str = lexicon.name_id_str(token.category);
      const auto [line, column, _] = self.location_at(token_index);
      stream->format("[{:3}] {:3}:{:<3} cat={:20} {}\n",
                     token_index,
                     line + 1,
                     column + 1,
                     token_cat_str,
                     tii_info->str);
    }
  }
}
