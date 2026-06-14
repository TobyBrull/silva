#pragma once

#include "canopy/expected.hpp"
#include "canopy/file_location.hpp"

#include "fragmentization_data.hpp"
#include "syntax_farm.hpp"

namespace silva {
  constexpr bool is_xid_start_generalized(codepoint_category_t);
  constexpr bool is_xid_continue_generalized(codepoint_category_t);

  enum class fragment_category_t {
    INVALID = 0,

    STRING,

    INDENT,
    DEDENT,
    NEWLINE,

    SPACE,
    LINEFEED,
    DIGIT,
    PARENTHESIS,
    OPERATOR,

    ID_LOWER,
    ID_UPPER,
    ID_START__NOT_ID_LOWER_AND_NOT_ID_UPPER,
    ID_CONTINUE__NOT_ID_START_AND_NOT_DIGIT,

    LANG_BEGIN,
    LANG_END,

    COMMENT,
    WHITESPACE,
  };

  // If a fragment corresponds to a unique codepoint.
  constexpr bool is_fragment_category_simple(fragment_category_t);

  constexpr bool is_fragment_category_id_start(fragment_category_t);
  constexpr bool is_fragment_category_id_continue(fragment_category_t);
  constexpr bool is_fragment_category_real(fragment_category_t);
  constexpr bool is_fragment_category_visible(fragment_category_t);

  token_id_t fragment_category_to_token_id(syntax_farm_t&, fragment_category_t);

  struct fragment_t {
    fragment_category_t category = fragment_category_t::INVALID;
    file_location_t location;

    friend auto operator<=>(const fragment_t&, const fragment_t&) = default;

    friend void pretty_write_impl(const fragment_t&, byte_sink_t*);
  };

  struct fragmentization_t : public menhir_t {
    syntax_farm_ptr_t sfp;
    filepath_t filepath;
    string_t source_code;

    array_t<fragment_t> fragments;

    index_t size() const;

    file_location_t location_at(index_t idx) const;

    string_view_t get_fragment_text(index_t frag_idx) const;
    index_t get_fragment_byte_offset(index_t frag_idx) const;
    expected_t<unicode::codepoint_t> get_unique_codepoint(index_t frag_idx) const;

    expected_t<index_t> advance_language(const index_t start) const;

    friend void pretty_write_impl(const fragmentization_t&, byte_sink_t*);
  };
  using fragmentization_ptr_t = ptr_t<const fragmentization_t>;

  struct fragment_location_t {
    fragmentization_ptr_t fp;
    index_t fragment_index = 0;

    friend void pretty_write_impl(const fragment_location_t&, byte_sink_t*);
  };

  struct fragment_span_t {
    fragmentization_ptr_t fp;
    index_t begin = 0;
    index_t end   = 0;

    fragment_span_t() = default;
    fragment_span_t(fragmentization_ptr_t);
    fragment_span_t(fragmentization_ptr_t, index_t begin, index_t end);

    operator span_t<const fragment_t>();
  };

  string_t escape_string(string_t);

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize_unique(filepath_t, string_t source_code);
  expected_t<fragmentization_ptr_t>
  fragmentize(syntax_farm_ptr_t, filepath_t, string_t source_code);
  expected_t<fragmentization_ptr_t> fragmentize_load(syntax_farm_ptr_t, filepath_t);

  struct fragmented_token_t {
    token_id_t token_id = token_id_none;
    name_id_t category  = name_id_none;
    struct item_t {
      fragment_category_t category   = fragment_category_t::INVALID;
      unicode::codepoint_t codepoint = unicode::codepoint_none;
    };
    array_t<item_t> items;
  };
  expected_t<fragmented_token_t> fragmented_token(syntax_farm_ptr_t, string_view_t);
}

// IMPLEMENTATION

namespace silva {
  constexpr bool is_fragment_category_simple(const fragment_category_t fc)
  {
    using enum fragment_category_t;
    return (fc == SPACE ||                                   //
            fc == LINEFEED ||                                //
            fc == DIGIT ||                                   //
            fc == PARENTHESIS ||                             //
            fc == OPERATOR ||                                //
            fc == ID_LOWER ||                                //
            fc == ID_UPPER ||                                //
            fc == ID_START__NOT_ID_LOWER_AND_NOT_ID_UPPER || //
            fc == ID_CONTINUE__NOT_ID_START_AND_NOT_DIGIT || //
            false);
  }
  constexpr bool is_xid_start_generalized(const codepoint_category_t cc)
  {
    using enum codepoint_category_t;
    return cc == XID_Start || cc == XID_Lowercase || cc == XID_Uppercase;
  }
  constexpr bool is_xid_continue_generalized(const codepoint_category_t cc)
  {
    return is_xid_start_generalized(cc) || cc == codepoint_category_t::XID_Continue;
  }

  constexpr bool is_fragment_category_id_start(const fragment_category_t fc)
  {
    using enum fragment_category_t;
    return (fc == ID_START__NOT_ID_LOWER_AND_NOT_ID_UPPER || fc == ID_LOWER || fc == ID_UPPER);
  }
  constexpr bool is_fragment_category_id_continue(const fragment_category_t fc)
  {
    using enum fragment_category_t;
    return is_fragment_category_id_start(fc) || fc == ID_CONTINUE__NOT_ID_START_AND_NOT_DIGIT ||
        fc == DIGIT;
  }
  constexpr bool is_fragment_category_real(const fragment_category_t fc)
  {
    using enum fragment_category_t;
    return (fc != WHITESPACE && fc != COMMENT);
  }
  constexpr bool is_fragment_category_visible(const fragment_category_t fc)
  {
    using enum fragment_category_t;
    return (fc != WHITESPACE && //
            fc != COMMENT &&    //
            fc != INDENT &&     //
            fc != DEDENT &&     //
            fc != NEWLINE &&    //
            true);
  }
}
