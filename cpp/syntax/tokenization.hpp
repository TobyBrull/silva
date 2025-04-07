#pragma once

#include "canopy/preprocessor.hpp"
#include "canopy/string_or_view.hpp"

#include "token_context.hpp"

namespace silva {
  struct tokenization_t : public menhir_t {
    token_context_ptr_t context;

    filesystem_path_t filepath;

    struct location_t {
      index_t line_num = 0;
      index_t column   = 0;
    };
    vector_t<location_t> token_locations;
    vector_t<token_id_t> tokens;

    tokenization_t copy() const;

    const token_info_t* token_info_get(index_t token_index) const;

    string_t to_string() const;
  };
  using tokenization_ptr_t = ptr_t<tokenization_t>;

  struct token_position_t {
    const tokenization_t* tokenization{nullptr};
    index_t token_index = 0;

    string_t to_string() const;
  };

  struct token_range_t {
    const tokenization_t* tokenization{nullptr};
    index_t token_begin = 0;
    index_t token_end   = 0;

    string_t to_string(index_t max_num_tokens = 5) const;
  };

  expected_t<unique_ptr_t<tokenization_t>> tokenize_load(token_context_ptr_t, filesystem_path_t);
  expected_t<unique_ptr_t<tokenization_t>>
  tokenize(token_context_ptr_t, filesystem_path_t filepath, string_view_t text);
}

// IMPLEMENTATION

namespace silva {
  template<>
  struct memento_item_writer_t<token_position_t> {
    inline static memento_item_type_t memento_item_type = memento_item_type_custom();
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
          return tp.to_string();
        });
  };

  template<>
  struct memento_item_writer_t<token_range_t> {
    inline static memento_item_type_t memento_item_type = memento_item_type_custom();
    static memento_item_type_t write(string_t& buffer, const token_range_t& x)
    {
      bit_append<token_range_t>(buffer, x);
      return memento_item_type;
    }
    static inline SILVA_USED bool reg = memento_item_reader_t::register_reader(
        memento_item_type,
        [](const byte_t* ptr, const index_t size) -> string_or_view_t {
          SILVA_ASSERT(size == sizeof(token_range_t));
          const token_range_t tr = bit_cast_ptr<token_range_t>(ptr);
          return tr.to_string();
        });
  };
}
