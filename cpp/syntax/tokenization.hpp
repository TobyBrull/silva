#pragma once

#include "canopy/preprocessor.hpp"
#include "canopy/string_or_view.hpp"
#include "syntax/syntax_context.hpp"

namespace silva {

  struct token_context_t : public menhir_t {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = true;

    vector_t<token_info_t> token_infos;
    hashmap_t<string_t, token_id_t> token_lookup;

    vector_t<name_info_t> name_infos;
    hashmap_t<name_info_t, name_id_t> name_lookup;

    token_context_t();

    token_id_t token_id(string_view_t);
    expected_t<token_id_t> token_id_unquoted(token_id_t);

    name_id_t name_id(name_id_t parent_name, token_id_t base_name);
    name_id_t name_id_span(name_id_t parent_name, span_t<const token_id_t>);
    bool name_id_is_parent(name_id_t parent_name, name_id_t child_name) const;

    name_id_t name_id_lca(name_id_t, name_id_t) const;

    template<typename... Ts>
    name_id_t name_id_of(Ts&&... xs);
    template<typename... Ts>
    name_id_t name_id_of(name_id_t parent_name, Ts&&... xs);
  };
  using token_context_ptr_t = ptr_t<token_context_t>;

  struct name_id_style_t {
    token_context_ptr_t tcp;
    token_id_t root      = tcp->token_id("silva");
    token_id_t current   = tcp->token_id("x");
    token_id_t parent    = tcp->token_id("up");
    token_id_t separator = tcp->token_id(".");

    name_id_t from_token_span(name_id_t current, span_t<const token_id_t>) const;

    string_t absolute(name_id_t) const;
    string_t relative(name_id_t current, name_id_t) const;
    string_t readable(name_id_t current, name_id_t) const;
  };

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
  name_id_t token_context_t::name_id_of(Ts&&... xs)
  {
    vector_t<token_id_t> vec;
    ((vec.push_back(token_id(std::forward<Ts>(xs)))), ...);
    return name_id_span(name_id_root, vec);
  }

  template<typename... Ts>
  name_id_t token_context_t::name_id_of(name_id_t parent_name, Ts&&... xs)
  {
    vector_t<token_id_t> vec;
    ((vec.push_back(token_id(std::forward<Ts>(xs)))), ...);
    return name_id_span(parent_name, vec);
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
