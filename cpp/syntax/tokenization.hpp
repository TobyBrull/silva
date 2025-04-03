#pragma once

#include "canopy/preprocessor.hpp"
#include "canopy/string_or_view.hpp"

#include "token_context.hpp"

namespace silva {
  struct tokenization_t : public menhir_t {
    token_context_ptr_t context;

    filesystem_path_t filepath;
    string_t text;
    vector_t<token_id_t> tokens;

    struct location_t {
      index_t line_num = 0;
      index_t column   = 0;
    };
    vector_t<location_t> token_locations;

    tokenization_t copy() const;

    const token_info_t* token_info_get(index_t token_index) const;

    string_t to_string() const;
  };
  using tokenization_ptr_t = ptr_t<tokenization_t>;

  struct token_position_t {
    const tokenization_t* tokenization{nullptr};
    index_t token_index = 0;
  };

  expected_t<unique_ptr_t<tokenization_t>> tokenize_load(token_context_ptr_t, filesystem_path_t);
  expected_t<unique_ptr_t<tokenization_t>>
  tokenize(token_context_ptr_t, filesystem_path_t filepath, string_t text);
}

// IMPLEMENTATION

namespace silva {
  template<>
  struct memento_item_writer_t<token_position_t> {
    constexpr inline static memento_item_type_t memento_item_type = memento_item_type_custom(1);
    static memento_item_type_t write(string_t& buffer, const token_position_t& x)
    {
      bit_append<token_position_t>(buffer, x);
      return memento_item_type;
    }
    static inline SILVA_USED bool reg = memento_item_reader_t::register_reader(
        memento_item_type,
        [](const byte_t* ptr, const index_t size) -> string_or_view_t {
          SILVA_ASSERT(size == sizeof(token_position_t));
          const token_position_t tp = bit_cast_ptr<token_position_t>(ptr);
          const auto [line, column] = tp.tokenization->token_locations[tp.token_index];
          const string_t filename   = tp.tokenization->filepath.filename().string();
          return string_or_view_t{fmt::format("[{}:{}:{}]", filename, line + 1, column + 1)};
        });
  };
}
