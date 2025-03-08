#pragma once

#include "canopy/expected.hpp"
#include "canopy/preprocessor.hpp"
#include "canopy/string_or_view.hpp"

namespace silva {

  enum class token_category_t {
    NONE = 0,
    IDENTIFIER,
    OPERATOR,
    STRING,
    NUMBER,
  };
  string_view_t to_string(token_category_t);

  // An index in the "token_infos" vector of "token_context_t". Equality of two tokens is then
  // equivalent to the equality of their token_info_index_t.
  using token_id_t = index_t;

  // Index in "qual_names
  using full_name_id_t = index_t;

  constexpr inline token_id_t token_id_none         = 0;
  constexpr inline full_name_id_t full_name_id_none = 0;

  struct token_info_t {
    string_t str;
    token_category_t category = token_category_t::NONE;

    expected_t<string_view_t> string_as_plain_contained() const;
    expected_t<double> number_as_double() const;

    friend auto operator<=>(const token_info_t&, const token_info_t&) = default;
  };

  struct full_name_info_t {
    full_name_id_t parent_name = full_name_id_none;
    token_id_t base_name       = token_id_none;

    friend auto operator<=>(const full_name_info_t&, const full_name_info_t&) = default;
    friend hash_value_t hash_impl(const full_name_info_t& x);
  };

  struct token_context_t : public menhir_t {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = true;

    vector_t<token_info_t> token_infos;
    hashmap_t<string_t, token_id_t> token_lookup;

    vector_t<full_name_info_t> full_name_infos;
    hashmap_t<full_name_info_t, full_name_id_t> full_name_lookup;

    token_context_t();

    token_id_t token_id(string_view_t);
    expected_t<token_id_t> token_id_unquoted(token_id_t);

    full_name_id_t full_name_id(full_name_id_t parent_name, token_id_t base_name);
    full_name_id_t full_name_id(span_t<const token_id_t>);
    bool full_name_id_is_parent(full_name_id_t parent_name, full_name_id_t child_name);
    string_t full_name_to_string(full_name_id_t, string_view_t separator = ".");

    template<typename... Ts>
    full_name_id_t full_name_id_of(Ts&&... xs);
  };
  using token_context_ptr_t = ptr_t<token_context_t>;

  struct tokenization_t : public sprite_t {
    token_context_ptr_t context;

    filesystem_path_t filepath;
    string_t text;
    vector_t<token_id_t> tokens;

    struct line_data_t {
      // The index in "tokens" of the first token in that line. If a line does not contain any
      // tokens, this is the index of any token in a following line. What if a token spans multiple
      // lines (could only happend for strings)?
      index_t token_index = 0;
      // Offset of the start of this line within "text".
      index_t source_code_offset = 0;

      friend auto operator<=>(const line_data_t&, const line_data_t&) = default;
    };
    // Contains one entry for each line in "text".
    vector_t<line_data_t> lines;

    tokenization_t copy() const;

    const token_info_t* token_info_get(index_t token_index) const;

    const line_data_t* binary_search_line(index_t token_index) const;

    tuple_t<index_t, index_t> compute_line_and_column(index_t token_index) const;

    string_t to_string() const;
  };

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
  template<typename... Ts>
  full_name_id_t token_context_t::full_name_id_of(Ts&&... xs)
  {
    vector_t<token_id_t> vec;
    ((vec.push_back(token_id(std::forward<Ts>(xs)))), ...);
    return full_name_id(vec);
  }

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
          const auto [line, column] = tp.tokenization->compute_line_and_column(tp.token_index);
          const string_t filename   = tp.tokenization->filepath.filename().string();
          return string_or_view_t{fmt::format("[{}:{}:{}]", filename, line + 1, column + 1)};
        });
  };
}
